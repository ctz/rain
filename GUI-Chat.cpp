#include "GUI-Chat.h"
#include "SystemIDs.h"

#include "CredentialManager.h"
#include "MessageCore.h"
#include "Message.h"
#include "BValue.h"
#include "PeerID.h"
#include "MeshCore.h"

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>

using namespace RAIN;

extern CredentialManager *globalCredentialManager;
extern MessageCore *globalMessageCore;

enum
{
	ID_CT_CHANS,
	ID_CT_USERS,
	ID_CT_TABS,
	ID_CT_CLOSETAB,

	ID_DIALOG_JOIN,
	ID_DIALOG_NEW,
	ID_DIALOG_CANCEL,
	ID_DIALOG_LIST,

	ID_CTP_INPUT,
};

BEGIN_EVENT_TABLE(RAIN::GUI::Chat, wxPanel)
	EVT_BUTTON(ID_CT_CHANS, RAIN::GUI::Chat::OnChansClick)
	EVT_BUTTON(ID_CT_USERS, RAIN::GUI::Chat::OnUsersClick)

	EVT_MENU(ID_CT_CLOSETAB, RAIN::GUI::Chat::OnCloseTab)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(RAIN::GUI::ChatTabs, wxNotebook)
	EVT_RIGHT_DOWN(RAIN::GUI::ChatTabs::OnTabRightClick)
END_EVENT_TABLE()

GUI::Chat::Chat(wxWindow *parent, wxWindowID id)
: wxPanel(parent, id)
{
	this->tabs = new ChatTabs(this, ID_CT_TABS);
	this->chans = new wxButton(this, ID_CT_CHANS, wxT("&Channels..."));
	this->users = new wxButton(this, ID_CT_USERS, wxT("&Users..."));
	this->users->Enable(false);

	wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer *buttonsizer = new wxBoxSizer(wxHORIZONTAL);

	buttonsizer->Add(this->chans, 0, wxALL, 0);
	buttonsizer->Add(this->users, 0, wxLEFT, 5);

	mainsizer->Add(buttonsizer, 0, wxEXPAND|wxALL, 5);
	mainsizer->Add(this->tabs, 1, wxEXPAND|wxBOTTOM|wxLEFT|wxRIGHT, 5);
	
	this->SetAutoLayout(true);
	this->SetSizer(mainsizer);
	this->Layout();

	info = new BEnc::Dictionary();
	infotime = 0;

	globalMessageCore->RegisterMessageHandler(this);
}

void GUI::Chat::HandleMessage(RAIN::Message *msg)
{
	BENC_SAFE_CAST(msg->headers["Event"], eventstr, String);

	wxLogVerbose("GUI::Chat got %s", eventstr->getString().c_str());

	if (eventstr && eventstr->getString() == "Connection-Accepted")
	{
		/* emit chat-sync to newly connected peer */
		this->SendSync(msg->GetFromHash());
	} else if (eventstr && eventstr->getString() == "Chat-Sync") {
		BENC_SAFE_CAST(msg->headers["Chat-Info"], info, Dictionary);
		BENC_SAFE_CAST(msg->headers["Chat-Timestamp"], infotime, Integer);

		if (info && infotime)
		{
			/* if we have a later timestamp, send them our info. else
			 * replace ours with theirs */
			if (infotime->getNum() > this->infotime)
			{
				delete this->info;
				this->info = new BEnc::Dictionary(info);
				this->infotime = infotime->getNum();
			} else if (infotime->getNum() < this->infotime) {
				this->SendSync(msg->GetFromHash());
			}
		}
	} else if (eventstr && eventstr->getString() == "Join-Channel") {
		/* join channel and create channel are in fact the same thing, because 
		 * create joins the channel if it already exists and join creates the
		 * channel if it doesn't */
		BENC_SAFE_CAST(msg->headers["Channel"], channelname, String);
		BENC_SAFE_CAST(msg->headers["Who"], who, String);

		wxString channelname_lower = channelname->getString().Lower();

		BENC_SAFE_CAST(info->getDictValue(channelname_lower), channel, List);

		if (channel)
		{
			bool add = true;

			for (size_t = 0; i < channel->getListSize(); i++)
			{
				BENC_SAFE_CAST(channel->getListItem(i), listwho, String);

				if (listwho->getString() == who)
				{
					add = false;
					break;
				}
			}

			if (add)
			{
				channel->pushList(new BEnc::String(who));
				infotime = wxDateTime::Now().GetTicks();
			}
		} else {
			channel = new BEnc::List();
			info->hash[channelname_lower] = channel;
			channel->pushList(new BEnc::String(who));
			infotime = wxDateTime::Now().GetTicks();
			wxLogStatus("Chat channel %s created", channelname_lower.c_str());
		}

		/* if we have a tab open, update it */
		GUI::ChatPanel *p = this->GetPanel(channelname_lower);
		
		if (p)
		{
			p->OnJoin(who->getString());
			p->UpdateUserlist(channel);
		}
	} else if (eventstr && eventstr->getString() == "Part-Channel") {
		/* part channel gets the channel list, removes the person from it
		 * and destroys the channel entirely if that was the last person to
		 * leave */
		BENC_SAFE_CAST(msg->headers["Channel"], channelname, String);
		BENC_SAFE_CAST(msg->headers["Who"], who, String);

		wxString channelname_lower = channelname->getString().Lower();

		if (info->hasDictValue(channelname_lower))
		{
			BENC_SAFE_CAST(info->getDictValue(channelname_lower), channel, List);
			
			channel->removeByString(who->getString());
			
			/* if we have a tab open in this channel, notify the panel */
			GUI::ChatPanel *p = this->GetPanel(channelname_lower);
			if (p)
			{
				p->OnPart(who->getString());
				p->UpdateUserlist(channel);
			}

			/* if the channel is now empty, delete it */
			if (channel->list.size() == 0)
			{
				info->hash.erase(channelname_lower);
				delete channel;
			}

			infotime = wxDateTime::Now().GetTicks();
		} else {
			/* parting a non-existant channel? don't care */
		}
	} else if (eventstr && eventstr->getString() == "Channel-Message") {
		BENC_SAFE_CAST(msg->headers["Channel"], channelname, String);

		if (!channelname)
			return;

		/* if we have a tab open, add to it */
		GUI::ChatPanel *p = this->GetPanel(channelname->getString());
		if (p)
			p->ProcessMessage(msg);
	}
}

void GUI::Chat::SendSync(const wxString &hash)
{
	/* emitting a null chat info dictionary is an error, so if we
	 * have nothing we create an default dictionary */
	if (!this->info)
	{
		this->info = new BEnc::Dictionary();
	}

	Message *m = new Message();
	m->status = MSG_STATUS_OUTGOING;
	m->subsystem = CHAT_ID;
	m->headers["Event"] = new BEnc::String("Chat-Sync");
	m->headers["Destination"] = new BEnc::String("Peer");
	m->headers["To"] = new BEnc::String(hash);
	m->headers["Chat-Info"] = new BEnc::Dictionary(this->info);
	m->headers["Chat-Timestamp"] = new BEnc::Integer(this->infotime);
	globalMessageCore->PushMessage(m);
}

RAIN::GUI::ChatPanel * GUI::Chat::GetPanel(const wxString& name)
{
	for (size_t i = 0; i < tabs->GetPageCount(); i++)
	{
		if (name.CmpNoCase(tabs->GetPageText(i)) == 0)
		{
			return (GUI::ChatPanel *) tabs->GetPage(i);
		}
	}
	
	return NULL;
}

int GUI::Chat::GetMessageHandlerID()
{
	return CHAT_ID;
}

void GUI::Chat::Tick()
{
}

void GUI::Chat::OnChansClick(wxCommandEvent &e)
{
	GUI::ChatChannelsDialog *channels = new GUI::ChatChannelsDialog(this);
	
	if (channels->ShowModal() && !channels->channel.IsEmpty())
	{
		ChatPanel *p = new ChatPanel(this->tabs, -1, channels->channel);
		this->tabs->AddPage(p, wxT(channels->channel), true);
		p->DoJoin(this->info);

	}

	channels->Destroy();
}

void GUI::Chat::OnUsersClick(wxCommandEvent &e)
{
	GUI::ChatUsersDialog *users = new GUI::ChatUsersDialog(this);
	users->ShowModal();
	users->Destroy();
}

void GUI::Chat::OnCloseTab(wxCommandEvent &e)
{
	GUI::ChatPanel *panel = (GUI::ChatPanel *) this->tabs->GetPage(this->tabs->lasttab-1);
	panel->DoPart(this->info);

	this->tabs->DeletePage(this->tabs->lasttab-1);
}

void GUI::ChatTabs::OnTabRightClick(wxMouseEvent &e)
{
	if (int tab = this->HitTest(e.GetPosition(), NULL) != wxNOT_FOUND)
	{
		this->lasttab = tab;
		wxMenu *menu = new wxMenu();
		menu->Append(ID_CT_CLOSETAB, wxT("&Close"));
		this->PopupMenu(menu, e.GetPosition());
		delete menu;
	}
}

/* ================================================================== */

BEGIN_EVENT_TABLE(RAIN::GUI::ChatChannelsDialog, wxDialog)
	EVT_BUTTON(ID_DIALOG_CANCEL, RAIN::GUI::ChatChannelsDialog::OnCancelClick)
	EVT_BUTTON(ID_DIALOG_JOIN, RAIN::GUI::ChatChannelsDialog::OnJoinClick)
	EVT_BUTTON(ID_DIALOG_NEW, RAIN::GUI::ChatChannelsDialog::OnNewClick)
	EVT_LISTBOX(ID_DIALOG_LIST, RAIN::GUI::ChatChannelsDialog::OnSelectClick)
END_EVENT_TABLE()

GUI::ChatChannelsDialog::ChatChannelsDialog(GUI::Chat *parent)
: wxDialog((wxWindow *) parent, -1, wxT("Chat Channels"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxTHICK_FRAME|wxCLIP_CHILDREN)
{
	this->ChannelBox = new wxListBox(this, ID_DIALOG_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL|wxLB_SORT);
	this->NewButton = new wxButton(this, ID_DIALOG_NEW, wxT("&New..."));
	this->ButtonSpacer = new wxPanel(this, -1);
	this->CancelButton = new wxButton(this, ID_DIALOG_CANCEL, wxT("&Cancel"));
	this->JoinButton = new wxButton(this, ID_DIALOG_JOIN, wxT("&Join..."));

	if (parent->info)
		this->ChannelBox->Append(parent->info->getArrayString());

	this->set_properties();
	this->do_layout();
}
void GUI::ChatChannelsDialog::set_properties()
{
	JoinButton->SetDefault();
	JoinButton->Enable(false);
}

void GUI::ChatChannelsDialog::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* TopSizer = new wxBoxSizer(wxHORIZONTAL);
	TopSizer->Add(ChannelBox, 1, wxLEFT|wxTOP|wxEXPAND|wxFIXED_MINSIZE, 10);
	TopSizer->Add(NewButton, 0, wxALL|wxFIXED_MINSIZE, 10);
	MainSizer->Add(TopSizer, 1, wxEXPAND, 0);
	ButtonSizer->Add(ButtonSpacer, 1, wxEXPAND, 0);
	ButtonSizer->Add(CancelButton, 0, wxLEFT|wxTOP|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ButtonSizer->Add(JoinButton, 0, wxALL|wxFIXED_MINSIZE, 10);
	MainSizer->Add(ButtonSizer, 0, wxEXPAND, 0);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	Layout();
}

void GUI::ChatChannelsDialog::OnJoinClick(wxCommandEvent &e)
{
	this->channel = ChannelBox->GetStringSelection();
	this->EndModal(true);
}

void GUI::ChatChannelsDialog::OnCancelClick(wxCommandEvent &e)
{
	this->channel = "";
	this->EndModal(false);
}

void GUI::ChatChannelsDialog::OnNewClick(wxCommandEvent &e)
{
	wxString newchannel = ::wxGetTextFromUser("Enter the new channel's name:", "New Channel", "#", this);

	if (!newchannel.IsEmpty() && newchannel != "#")
	{
		if (newchannel.GetChar(0) != '#')
		{
			newchannel = "#" + newchannel;
		}

		ChannelBox->Append(newchannel);
	}
}

void GUI::ChatChannelsDialog::OnSelectClick(wxCommandEvent &e)
{
	this->JoinButton->Enable(true);
}

/* ================================================================== */

BEGIN_EVENT_TABLE(RAIN::GUI::ChatUsersDialog, wxDialog)
	EVT_BUTTON(ID_DIALOG_CANCEL, RAIN::GUI::ChatUsersDialog::OnCancelClick)
	EVT_BUTTON(ID_DIALOG_JOIN, RAIN::GUI::ChatUsersDialog::OnJoinClick)
	EVT_LISTBOX(ID_DIALOG_LIST, RAIN::GUI::ChatUsersDialog::OnSelectClick)
END_EVENT_TABLE()

GUI::ChatUsersDialog::ChatUsersDialog(GUI::Chat *parent)
: wxDialog((wxWindow *) parent, -1, wxT("Chat with Users"), wxDefaultPosition, wxSize(400, 300), wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxTHICK_FRAME|wxCLIP_CHILDREN)
{
	this->TypeBox = new wxCheckBox(this, -1, wxT("Only Users in a Channel"), wxDefaultPosition, wxDefaultSize);
	this->UserList = new wxListBox(this, ID_DIALOG_LIST, wxDefaultPosition, wxDefaultSize, 0, NULL, wxLB_SINGLE|wxLB_HSCROLL|wxLB_SORT);
	this->ButtonSpacer = new wxPanel(this, -1);
	this->CancelButton = new wxButton(this, ID_DIALOG_CANCEL, wxT("&Cancel"));
	this->ChatButton = new wxButton(this, ID_DIALOG_JOIN, wxT("&Send Message..."));

	this->set_properties();
	this->do_layout();
}

void GUI::ChatUsersDialog::set_properties()
{
	this->ChatButton->Enable(false);
}

void GUI::ChatUsersDialog::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	MainSizer->Add(TypeBox, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 10);
	MainSizer->Add(UserList, 1, wxLEFT|wxRIGHT|wxEXPAND|wxFIXED_MINSIZE, 10);
	ButtonSizer->Add(ButtonSpacer, 1, wxEXPAND, 0);
	ButtonSizer->Add(CancelButton, 0, wxLEFT|wxTOP|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ButtonSizer->Add(ChatButton, 0, wxALL|wxFIXED_MINSIZE, 10);
	MainSizer->Add(ButtonSizer, 0, wxEXPAND, 0);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	Layout();
}

void GUI::ChatUsersDialog::OnJoinClick(wxCommandEvent &e)
{
	this->user = UserList->GetStringSelection();
	this->EndModal(true);
}

void GUI::ChatUsersDialog::OnCancelClick(wxCommandEvent &e)
{
	this->user = "";
	this->EndModal(false);
}

void GUI::ChatUsersDialog::OnSelectClick(wxCommandEvent &e)
{
	this->ChatButton->Enable(true);
}

/* ================================================================== */

BEGIN_EVENT_TABLE(RAIN::GUI::ChatPanel, wxPanel)
	EVT_TEXT_ENTER(ID_CTP_INPUT, RAIN::GUI::ChatPanel::OnTextEnter)
END_EVENT_TABLE()

GUI::ChatPanel::ChatPanel(wxWindow *parent, wxWindowID id, const wxString& channel)
 : wxPanel(parent, id)
{
	splitter = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize, wxSP_NO_XP_THEME|wxSP_LIVE_UPDATE);
	text = new wxTextCtrl(splitter, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_RICH|wxTE_RICH2|wxTE_AUTO_URL|wxTE_MULTILINE|wxTE_READONLY);
	users = new wxListBox(splitter, -1, wxDefaultPosition, wxDefaultSize, NULL, 0, wxLB_EXTENDED|wxLB_HSCROLL|wxLB_NEEDED_SB|wxLB_SORT);
	splitter->SplitVertically(text, users, -150);
	splitter->SetMinimumPaneSize(100);

	text->SetFont(wxFont(-1, wxMODERN, wxNORMAL, wxNORMAL));

	input = new wxTextCtrl(this, ID_CTP_INPUT, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	
	wxBoxSizer *mainsizer = new wxBoxSizer(wxVERTICAL);

	mainsizer->Add(splitter, 1, wxEXPAND|wxALL, 5);
	mainsizer->Add(input, 0, wxEXPAND|wxBOTTOM|wxLEFT|wxRIGHT, 5);
	
	this->SetAutoLayout(true);
	this->SetSizer(mainsizer);
	this->Layout();

	this->channel = channel;
}

void GUI::ChatPanel::OnTextEnter(wxCommandEvent &e)
{
	Message *m = new Message();
	m->status = MSG_STATUS_OUTGOING;
	m->subsystem = CHAT_ID;

	if (channel.GetChar(0) == '#')
	{
		m->PrepareBroadcast();
		m->headers["Event"] = new BEnc::String("Channel-Message");
		m->headers["Channel"] = new BEnc::String(channel);
		m->headers["Message"] = new BEnc::String(this->input->GetValue());
	} else {
		m->headers["Event"] = new BEnc::String("Private-Message");
		m->headers["Destination"] = new BEnc::String("Peer");
		m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
		PeerID *pid = globalMessageCore->app->mesh->GetPeerByCommonName(channel);

		if (pid)
		{
			m->headers["To"] = new BEnc::String(pid->hash);
		} else {
			wxLogWarning("No peer known by %s", channel.c_str());
			delete m;
			return;
		}

		m->headers["Message"] = new BEnc::String(this->input->GetValue());
	}
	
	globalMessageCore->PushMessage(m);
	this->ProcessMessage(m);

	this->input->SetValue("");
}

void GUI::ChatPanel::DoJoin(BEnc::Dictionary *d)
{
	if (channel.GetChar(0) == '#')
	{
		BENC_SAFE_CAST(d->getDictValue(channel), chan, List);

		if (chan)
		{
			chan->pushList(new BEnc::String(globalCredentialManager->GetMyPeerID()->hash));
		} else {
			chan = new BEnc::List();
			d->hash[channel] = chan;
			chan->pushList(new BEnc::String(globalCredentialManager->GetMyPeerID()->hash));
		}

		this->UpdateUserlist(chan);

		Message *join = new Message();
		join->status = MSG_STATUS_OUTGOING;
		join->subsystem = CHAT_ID;
		join->PrepareBroadcast();
		join->headers["Event"] = new BEnc::String("Join-Channel");
		join->headers["Who"] = new BEnc::String(globalCredentialManager->GetMyPeerID()->hash);
		join->headers["Channel"] = new BEnc::String(channel);

		globalMessageCore->PushMessage(join);
	}
}

void GUI::ChatPanel::DoPart(BEnc::Dictionary *d)
{
	if (channel.GetChar(0) == '#')
	{
		if (d->hasDictValue(channel))
		{
			BENC_SAFE_CAST(d->getDictValue(channel), chan, List);
			chan->removeByString(globalCredentialManager->GetMyPeerID()->hash);
		}
	
		Message *part = new Message();
		part->status = MSG_STATUS_OUTGOING;
		part->subsystem = CHAT_ID;
		part->PrepareBroadcast();
		part->headers["Event"] = new BEnc::String("Part-Channel");
		part->headers["Who"] = new BEnc::String(globalCredentialManager->GetMyPeerID()->hash);
		part->headers["Channel"] = new BEnc::String(channel);

		globalMessageCore->PushMessage(part);
	}
}

void GUI::ChatPanel::UpdateUserlist(BEnc::List *users)
{
	this->users->Clear();

	for (size_t i = 0; i < users->getListSize(); i++)
	{
		BENC_SAFE_CAST(users->getListItem(i), userhash, String);

		if (userhash)
		{
			PeerID *pid = globalMessageCore->app->mesh->GetPeer(userhash->getString());
			this->users->Append(wxString::Format("%s (%s)", pid->GetCertificateField(NID_commonName).c_str(), CryptoHelper::HashToHex(userhash->getString(), true)));
		}
	}
}

void GUI::ChatPanel::ProcessMessage(RAIN::Message *m)
{
	/* this message is certain to be for us, so just display
	 * it in a pretty way */
	BENC_SAFE_CAST(m->headers["Message"], messagestr, String);
	wxString from = m->GetFromHash();

	PeerID *pid = globalMessageCore->app->mesh->GetPeer(from);
	
	this->text->SetDefaultStyle(wxTextAttr(*wxLIGHT_GREY));
	this->text->AppendText(wxDateTime::Now().FormatTime());
	this->text->SetDefaultStyle(wxTextAttr(*wxBLACK));
	this->text->AppendText(" <");
	this->text->SetDefaultStyle(wxTextAttr(*wxBLUE));
	this->text->AppendText(pid->GetCertificateField(NID_commonName));
	this->text->SetDefaultStyle(wxTextAttr(*wxBLACK));
	this->text->AppendText("> ");
	this->text->AppendText(messagestr->getString());
	this->text->AppendText("\n");
}

void GUI::ChatPanel::OnPart(const wxString &hash)
{
	PeerID *pid = globalMessageCore->app->mesh->GetPeer(hash);

	if (!pid)
		return;
	
	this->text->SetDefaultStyle(wxTextAttr(*wxLIGHT_GREY));
	this->text->AppendText(" * " + pid->GetCertificateField(NID_commonName) + " left " + this->channel + "\n");
}

void GUI::ChatPanel::OnJoin(const wxString &hash)
{
	PeerID *pid = globalMessageCore->app->mesh->GetPeer(hash);

	if (!pid)
		return;
	
	this->text->SetDefaultStyle(wxTextAttr(*wxLIGHT_GREY));
	this->text->AppendText(" * " + pid->GetCertificateField(NID_commonName) + " joined " + this->channel + "\n");
}
