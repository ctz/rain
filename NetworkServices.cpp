#include "NetworkServices.h"

#include "BValue.h"
#include "CryptoHelper.h"
#include "CredentialManager.h"
#include "PeerID.h"
#include "MessageCore.h"
#include "SystemIDs.h"

using namespace RAIN;
extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;

NetworkServices::NetworkServices()
{ }

NetworkServices::~NetworkServices()
{ }
		
void NetworkServices::HandleMessage(RAIN::Message *msg)
{
	BENC_SAFE_CAST(msg->headers["Event"], msgevent, String);
	BENC_SAFE_CAST(msg->headers["From"], msgfromd, Dictionary);

	if (!msgfromd || !msgevent)
		return;

	BENC_SAFE_CAST(msgfromd->getDictValue("hash"), msgfrom, String);
	BENC_SAFE_CAST(msg->headers["Via"], msgvia, Dictionary);
	BENC_SAFE_CAST(msg->headers["Route-Preload"], msgroute, List);

	if (!msgevent || !msgfrom)
		return;

	if (msgevent->getString() == "Ping")
	{
		// got ping - send pong
		Message *m = new Message();
		m->status = MSG_STATUS_OUTGOING;
		m->subsystem = NETWORKSERV_ID;
		m->headers["To"] = msg->GetReply();
		m->headers["Destination"] = new BEnc::String("Peer");
		m->headers["Event"] = new BEnc::String("Pong");
		m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
		globalMessageCore->PushMessage(m);
	} else if (msgevent->getString() == "Pong") {
		// got pong
		::wxMessageBox(wxString::Format("Pong from %s", CryptoHelper::HashToHex(msgfrom->getString(), true).c_str()));
	} else if (msgevent->getString() == "T-Ping") {
		// got t-ping - send t-pong
		Message *m = new Message();
		m->status = MSG_STATUS_OUTGOING;
		m->subsystem = NETWORKSERV_ID;
		m->headers["To"] = msg->GetReply();
		m->headers["Destination"] = new BEnc::String("Peer");
		m->headers["Event"] = new BEnc::String("T-Pong");
		m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
		globalMessageCore->PushMessage(m);
	} else if (msgevent->getString() == "T-Pong") {
		// got t-pong
		wxString msg = (!msgroute) ? wxT("no route information - direct connection?") : wxString::Format("%d nodes in route", msgroute->list.size());

		::wxMessageBox(wxString::Format("Traceroute reply from %s: %s", CryptoHelper::HashToHex(msgfrom->getString(), true).c_str(), msg.c_str()));

		if (msgroute)
			msgroute->Debug();
	}
}

void NetworkServices::Tick()
{ }

int NetworkServices::GetMessageHandlerID()
{
	return NETWORKSERV_ID;
}

void NetworkServices::Ping(const wxString& hash)
{
	Message *m = new Message();
	m->status = MSG_STATUS_OUTGOING;
	m->subsystem = NETWORKSERV_ID;
	m->headers["Destination"] = new BEnc::String("Peer");
	m->headers["To"] = new BEnc::String(hash);
	m->headers["Event"] = new BEnc::String("Ping");
	m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
	globalMessageCore->PushMessage(m);
}

void NetworkServices::Traceroute(const wxString& hash)
{
	Message *m = new Message();
	m->status = MSG_STATUS_OUTGOING;
	m->subsystem = NETWORKSERV_ID;
	m->headers["Destination"] = new BEnc::String("Peer");
	m->headers["To"] = new BEnc::String(hash);
	m->headers["Event"] = new BEnc::String("T-Ping");
	m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
	globalMessageCore->PushMessage(m);
}
