#include "DebugHelper.h"
#include "BValue.h"
#include "Message.h"

#include "GUI-ChatMain.h"
#include "MeshCore.h"
#include "ManifestManager.h"
#include "SystemIDs.h"

using namespace RAIN;
DebugHelper *debug = NULL;
#define USE_DEBUG

RAIN::DebugHelper::DebugHelper()
{
#ifdef USE_DEBUG
	this->filename = wxString::Format("log.%ld.xml", wxDateTime::Now().GetTicks());
	this->doc = new wxXmlDocument();
	this->mEle = new wxXmlNode(wxXML_ELEMENT_NODE, "messages");

	wxXmlNode *e = new wxXmlNode(wxXML_ELEMENT_NODE, "rain-message-log");
	e->AddChild(this->mEle);
	this->doc->SetRoot(e);
#endif//USE_DEBUG
}

RAIN::DebugHelper::~DebugHelper()
{
#ifdef USE_DEBUG
	this->doc->Save(this->filename);	
	delete this->doc;
#endif//USE_DEBUG
}

void RAIN::DebugHelper::LogMessage(RAIN::Message *m)
{
#ifdef USE_DEBUG
	this->LogState(m);

	wxXmlNode *n = new wxXmlNode(wxXML_ELEMENT_NODE, "message");
	n->AddProperty("timestamp", wxString::Format("%ld", wxDateTime::Now().GetTicks()));

	wxString status;

	switch (m->status)
	{
		case MSG_STATUS_UNKNOWN:
			status = "unknown";
			break;
		case MSG_STATUS_INCOMING:
			status = "incoming";
			break;
		case MSG_STATUS_LOCAL:
			status = "local";
			break;
		case MSG_STATUS_OUTGOING:
			status = "outgoing";
			break;
	}

	n->AddProperty("status", status);
	n->AddProperty("subsystem", wxString::Format("%d", m->subsystem));

	HeaderHash::iterator i = m->headers.begin();

	while (i != m->headers.end())
	{
		wxXmlNode *h = new wxXmlNode(wxXML_ELEMENT_NODE, "param");
		h->AddProperty("key", i->first);

		BEnc::Value *value = i->second;

		try
		{
			value->toXML(h);
		} catch (...) {
			wxLogVerbose("Exception at %s(%d): 0x%08X is invalid ptr -> IsOk = %d", __FILE__, __LINE__, value, value->IsOk());
			m->Debug();
		}

		i++;

		n->AddChild(h);
	}

	this->mEle->AddChild(n);
#endif//USE_DEBUG
}

void RAIN::DebugHelper::LogState(RAIN::Message *m)
{
#ifdef USE_DEBUG
	wxString status, subsys, event;

	if (!m)
	{
		wxASSERT_MSG(0, "null message");
	}

	BENC_SAFE_CAST(m->headers["Event"], mevent, String);

	switch (m->status)
	{
		case MSG_STATUS_UNKNOWN:
			status = "- unknown -";
			break;
		case MSG_STATUS_INCOMING:
			status = "inc        ";
			break;
		case MSG_STATUS_LOCAL:
			status = "    loc    ";
			break;
		case MSG_STATUS_OUTGOING:
			status = "        out";
			break;
	}

	switch (m->subsystem)
	{
		case MESHCORE_ID:
			subsys = "Mesh";
			break;
		case CHATGUI_ID:
			subsys = "GUI";
			break;
		case MANIFEST_ID:
			subsys = "Manifest";
			break;
		case PIECEMAN_ID:
			subsys = "PieceManager";
			break;
		case PIECEQUEUE_ID:
			subsys = "PieceQueue";
			break;
		case NETWORKSERV_ID:
			subsys = "NetworkServices";
			break;
		case CHAT_ID:
			subsys = "Chat";
			break;
		case -1:
		default:
			BENC_SAFE_CAST(m->headers["Destination"], msgdest, String);
			if (msgdest && msgdest->getString().Cmp("Broadcast") == 0)
				subsys = "Broadcast";
			else
				subsys = "Unknown";
			break;
	}

	wxApp *a = (wxApp *)wxApp::GetInstance();
	RAIN::GUI::ChatMain *c = (RAIN::GUI::ChatMain *) a->GetTopWindow();

	if (!mevent)
		event = m->CanonicalForm();
	else {
		event = mevent->str;
	}

	c->ConversationText->Append(wxString::Format("[%s] %s: %s", status.c_str(), subsys.c_str(), event.c_str()));
#endif//USE_DEBUG
}
