#include "GUI-ChatMain.h"
#include "GUI-ConnectionProperties.h"
#include "GUI-ConnectionsWidget.h"
#include "GUI-PeersWidget.h"
#include "GUI-QueueWidget.h"
#include "GUI-NetworkMonitor.h"
#include "GUI-FilesWidget.h"
#include "GUI-CertificateWizard.h"
#include "GUI-Preferences.h"
#include "GUI-AddFiles.h"
#include "GUI-AboutDialog.h"
#include "GUI-Chat.h"
#include "BValue.h"
#include "PeerID.h"
#include "MeshCore.h"
#include "Message.h"
#include "MessageCore.h"
#include "CredentialManager.h"
#include "ConnectionPool.h"
#include "NetworkServices.h"
#include "SystemIDs.h"

#include "Versions.h"

#include <openssl/x509.h>
#include <fstream>
#include <wx/log.h>

BEGIN_EVENT_TABLE(RAIN::GUI::ChatMain, wxFrame)
	EVT_MENU(ID_MENUEXIT, RAIN::GUI::ChatMain::OnQuit)

	EVT_MENU(ID_MENUCONNECT, RAIN::GUI::ChatMain::OnConnect)
	EVT_MENU(ID_MENUDISCONNECT, RAIN::GUI::ChatMain::OnDisconnect)
	EVT_MENU(ID_MENUFORCEMANIFEST, RAIN::GUI::ChatMain::OnManifestUpdate)
	EVT_MENU(ID_MENUPREFS, RAIN::GUI::ChatMain::OnPrefs)

	EVT_MENU(ID_DBGCONNECTION, RAIN::GUI::ChatMain::OnDbgConnProps)
	EVT_MENU(ID_DBGWIZARD, RAIN::GUI::ChatMain::OnDbgWizard)
	EVT_MENU(ID_DBGADDFILES, RAIN::GUI::ChatMain::OnDbgAddFiles)

	EVT_MENU(ID_HELPABOUT, RAIN::GUI::ChatMain::OnAbout)
	EVT_BUTTON(ID_UPDATESTATS, RAIN::GUI::ChatMain::UpdateStats)
	EVT_BUTTON(ID_PINGPEER, RAIN::GUI::ChatMain::OnPingPeer)
	EVT_BUTTON(ID_TRACEPEER, RAIN::GUI::ChatMain::OnTracePeer)

	EVT_CLOSE(RAIN::GUI::ChatMain::OnClose)
END_EVENT_TABLE()

extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;

RAIN::GUI::ChatMain::ChatMain(RAIN::SandboxApp *app)
	: wxFrame(NULL, -1, wxT("RAIN " __PRODUCTRELEASESTATUS " " __PRODUCTNICEVERSION), wxDefaultPosition, wxDefaultSize, wxDEFAULT_FRAME_STYLE|wxCLIP_CHILDREN)
{
	this->app = app;

	// notebook
	this->Notebook = new wxNotebook(this, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN);
	this->LogPage = new wxPanel(Notebook, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN);
	this->ConnectionsPage = new wxPanel(Notebook, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN);
	this->QueuePage = new wxPanel(Notebook, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN);
	this->FilesPage = new wxPanel(Notebook, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN);
	this->ChatPage = new wxPanel(Notebook, -1, wxDefaultPosition, wxDefaultSize, wxNO_FULL_REPAINT_ON_RESIZE|wxCLIP_CHILDREN);
	this->Connections = new RAIN::GUI::ConnectionsWidget(this->ConnectionsPage, -1, this->app);
	this->Peers = new RAIN::GUI::PeersWidget(this->ConnectionsPage, -1, this->app);
	this->Queue = new RAIN::GUI::QueueWidget(this->QueuePage, -1, this->app);
	this->Files = new RAIN::GUI::FilesWidget(this->FilesPage, -1, this->app);
	this->Chat = new RAIN::GUI::Chat(this->ChatPage, -1);
	this->Network = new RAIN::GUI::NetworkMonitor(this->LogPage, -1);
	this->ConversationText = new wxListBox(LogPage, -1, wxDefaultPosition, wxDefaultSize, 0, NULL);

	this->UpdateButton = new wxButton(this->ConnectionsPage, ID_UPDATESTATS, wxT(" Update "));
	this->PingButton = new wxButton(this->ConnectionsPage, ID_PINGPEER, wxT(" Ping "));
	this->TracerouteButton = new wxButton(this->ConnectionsPage, ID_TRACEPEER, wxT(" Traceroute "));
	
	// statusbar
	this->CreateStatusBar(1, wxST_SIZEGRIP);

	// menus
	this->MenuBar = new wxMenuBar();
	this->SetMenuBar(MenuBar);

	wxMenu* FileMenu = new wxMenu();
	FileMenu->Append(ID_DBGWIZARD, wxT("&Certificate Wizard..."));
	FileMenu->Append(ID_MENUPREFS, wxT("&Preferences..."));
	FileMenu->Append(ID_MENUEXIT, wxT("E&xit"));
	this->MenuBar->Append(FileMenu, wxT("&File"));

	wxMenu* MeshMenu = new wxMenu();
	MeshMenu->Append(ID_MENUCONNECT, wxT("&Connect"));
	MeshMenu->Append(ID_MENUDISCONNECT, wxT("&Disconnect"));
	MeshMenu->Append(ID_MENUFORCEMANIFEST, wxT("&Force Manifest Synch"));
	this->MenuBar->Append(MeshMenu, wxT("&Mesh"));

	wxMenu* DebugMenu = new wxMenu();
	DebugMenu->Append(ID_DBGCONNECTION, wxT("&First Connection Properties..."));
	DebugMenu->Append(ID_DBGADDFILES, wxT("&Add Files..."));
	this->MenuBar->Append(DebugMenu, wxT("&Debug"));

	wxMenu* HelpMenu = new wxMenu();
	HelpMenu->Append(ID_HELPABOUT, wxT("&About..."));
	this->MenuBar->Append(HelpMenu, wxT("&Help"));

	this->set_properties();
	this->do_layout();

	/*wxLogWindow *win = new wxLogWindow(this, "RAIN Debugging Log", true, false);
	win->GetFrame()->SetIcon(wxIcon("IDI_AAA_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
	wxLog::SetActiveTarget(win);*/

	this->Show();
}

RAIN::GUI::ChatMain::~ChatMain()
{
}

void RAIN::GUI::ChatMain::set_properties()
{
#ifdef __WXMSW__
	this->SetIcon(wxIcon("IDI_AAA_ICON", wxBITMAP_TYPE_ICO_RESOURCE));
#endif

	this->SetSize(wxSize(700, 500));
	this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->ConversationText->SetFont(wxFont(10, wxTELETYPE, wxNORMAL, wxNORMAL));
}

void RAIN::GUI::ChatMain::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* LogSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ConnectionsSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* MeshButtons = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* FilesSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ChatSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* QueueSizer = new wxBoxSizer(wxVERTICAL);
	
	FilesSizer->Add(this->Files, 1, wxEXPAND, 0);
	FilesPage->SetAutoLayout(true);
	FilesPage->SetSizer(FilesSizer);
	FilesSizer->Fit(this->FilesPage);
	FilesSizer->SetSizeHints(this->FilesPage);
	
	ChatSizer->Add(this->Chat, 1, wxEXPAND, 0);
	ChatPage->SetAutoLayout(true);
	ChatPage->SetSizer(ChatSizer);
	ChatSizer->Fit(this->Chat);
	ChatSizer->SetSizeHints(this->ChatPage);

	QueueSizer->Add(this->Queue, 1, wxALL|wxEXPAND, 5);
	QueuePage->SetAutoLayout(true);
	QueuePage->SetSizer(QueueSizer);
	QueueSizer->Fit(this->QueuePage);
	QueueSizer->SetSizeHints(this->QueuePage);

	MeshButtons->Add(this->UpdateButton, 0, wxLEFT|wxRIGHT|wxBOTTOM, 5);
	MeshButtons->Add(this->PingButton, 0, wxRIGHT|wxBOTTOM, 5);
	MeshButtons->Add(this->TracerouteButton, 0, wxRIGHT|wxBOTTOM, 5);

	ConnectionsSizer->Add(new wxStaticText(this->ConnectionsPage, -1, wxT("Connections:")), 0, wxALL, 5);
	ConnectionsSizer->Add(this->Connections, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	ConnectionsSizer->Add(new wxStaticText(this->ConnectionsPage, -1, wxT("Peers:")), 0, wxLEFT|wxRIGHT|wxBOTTOM, 5);
	ConnectionsSizer->Add(this->Peers, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	ConnectionsSizer->Add(MeshButtons, 0, wxEXPAND, 0);
	ConnectionsPage->SetAutoLayout(true);
	ConnectionsPage->SetSizer(ConnectionsSizer);
	ConnectionsSizer->Fit(this->ConnectionsPage);
	ConnectionsSizer->SetSizeHints(this->ConnectionsPage);
	
	LogSizer->Add(this->Network, 1, wxALL|wxEXPAND, 5);
	LogSizer->Add(this->ConversationText, 1, wxALL|wxEXPAND, 5);
	LogPage->SetAutoLayout(true);
	LogPage->SetSizer(LogSizer);
	LogSizer->Fit(this->LogPage);
	LogSizer->SetSizeHints(this->LogPage);

#ifdef __WXMSW__
	wxImageList *img = new wxImageList(16, 16);
	img->Add(wxIcon("IDI_BROWSE_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));
	img->Add(wxIcon("IDI_CONNECTIONS_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));
	img->Add(wxIcon("IDI_QUEUE_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));
	img->Add(wxIcon("IDI_STATS_ICON", wxBITMAP_TYPE_ICO_RESOURCE, 16, 16));
	Notebook->AssignImageList(img);

	Notebook->AddPage(this->FilesPage, wxT(" Browse "), true, 0);
	Notebook->AddPage(this->ChatPage, wxT(" Chat "), false, 2);
	Notebook->AddPage(this->ConnectionsPage, wxT(" Connections "), false, 1);
	Notebook->AddPage(this->QueuePage, wxT(" Queue "), false, 2);
	Notebook->AddPage(this->LogPage, wxT(" Statistics "), false, 3);
#else
	Notebook->AddPage(this->FilesPage, wxT(" Browse "), true);
	Notebook->AddPage(this->ChatPage, wxT(" Connections "), false);
	Notebook->AddPage(this->ConnectionsPage, wxT(" Connections "), false);
	Notebook->AddPage(this->QueuePage, wxT(" Queue "), false);
	Notebook->AddPage(this->LogPage, wxT(" Statistics "), false);
#endif

	MainSizer->Add(new wxNotebookSizer(Notebook), 1, wxALL|wxEXPAND, 5);
	this->SetAutoLayout(true);
	this->SetSizer(MainSizer);
	this->Layout();
}

void RAIN::GUI::ChatMain::OnClose(wxCloseEvent &e)
{
	this->Destroy();
}

void RAIN::GUI::ChatMain::OnQuit(wxCommandEvent &e)
{
	this->Destroy();
}

void RAIN::GUI::ChatMain::OnPrefs(wxCommandEvent &e)
{
	GUI::Preferences *prefs = new GUI::Preferences(this);
	prefs->ShowModal();
	prefs->Destroy();
}

void RAIN::GUI::ChatMain::OnConnect(wxCommandEvent &e)
{
	wxString addr = wxGetTextFromUser(wxT("Enter address:"), wxT("Connect Peer"));

	if (addr == wxEmptyString)
		return;

	if (!addr.Contains(wxT(":")))
	{
		addr.Append(wxT(":1322"));
	}

	this->app->connections->ConnectPeer(addr);
}

void RAIN::GUI::ChatMain::OnDisconnect(wxCommandEvent &e)
{
	this->app->connections->DisconnectAll();
}

void RAIN::GUI::ChatMain::OnAbout(wxCommandEvent &e)
{
	GUI::AboutDialog *about = new GUI::AboutDialog(this);
	about->ShowModal();
	about->Destroy();
}

void RAIN::GUI::ChatMain::OnManifestUpdate(wxCommandEvent &e)
{
	RAIN::Message *m = new RAIN::Message();
	m->subsystem = MANIFEST_ID;
	m->status = MSG_STATUS_LOCAL;
	m->headers["Event"] = new BEnc::String("Force-Update");

	globalMessageCore->PushMessage(m);
}

void RAIN::GUI::ChatMain::UpdateStats(wxCommandEvent &e)
{
	this->Connections->Update();
	this->Peers->Update();
	this->Queue->Update();
}

void RAIN::GUI::ChatMain::OnPingPeer(wxCommandEvent &e)
{
	if (this->Peers->GetFirstSelected() < 0)
		return;

	RAIN::PeerID *pid = this->app->mesh->GetPeer(this->Peers->GetFirstSelected());

	if (pid)
		this->app->networkServices->Ping(pid->hash);
}

void RAIN::GUI::ChatMain::OnTracePeer(wxCommandEvent &e)
{
	if (this->Peers->GetFirstSelected() < 0)
		return;

	RAIN::PeerID *pid = this->app->mesh->GetPeer(this->Peers->GetFirstSelected());

	if (pid)
		this->app->networkServices->Traceroute(pid->hash);
}

void RAIN::GUI::ChatMain::OnDbgConnProps(wxCommandEvent &e)
{
	if (this->app->connections->AnyConnections())
	{
		RAIN::GUI::ConnectionProperties *c = new RAIN::GUI::ConnectionProperties(this, this->app->connections->Get(0));
		c->ShowModal();
		c->Destroy();
	}
}

void RAIN::GUI::ChatMain::OnDbgWizard(wxCommandEvent &e)
{
	RAIN::GUI::CertificateWizard *wizard = new RAIN::GUI::CertificateWizard(this, this->app);
	wizard->RunWizard();
}

void RAIN::GUI::ChatMain::OnDbgAddFiles(wxCommandEvent &e)
{
	RAIN::GUI::AddFilesDialog *f = new RAIN::GUI::AddFilesDialog(this);
	f->ShowModal();
	f->Destroy();
}

/*void RAIN::GUI::ChatMain::OnChatTextEnter(wxCommandEvent &e)
{
	RAIN::Message *chat = new RAIN::Message();
	chat->status = MSG_STATUS_OUTGOING;
	chat->subsystem = CHATGUI_ID;
	chat->headers["Event"] = new RAIN::BEnc::String("Message");
	chat->headers["Destination"] = new RAIN::BEnc::String("Broadcast");
	chat->headers["From"] = new RAIN::BEnc::String(globalCredentialManager->GetMyPeerID()->hash);
	RAIN::BEnc::Dictionary *viadict = new RAIN::BEnc::Dictionary();
	chat->headers["Via"] = viadict; 
	viadict->hash[globalCredentialManager->GetMyPeerID()->hash] = new BEnc::Dictionary();
	chat->headers["Message"] = new RAIN::BEnc::String(this->ChatInput->GetValue());
	globalMessageCore->PushMessage(chat);

	this->ChatInput->SetValue("");

	this->HandleMessage(chat);
}*/

void RAIN::GUI::ChatMain::Tick()
{

}

void RAIN::GUI::ChatMain::HandleMessage(RAIN::Message *msg)
{
	this->Connections->Update();
	this->Peers->Update();
	this->Queue->Update();
	this->Files->Update();

	/*BENC_SAFE_CAST(msg->headers["Event"], msgevent, String);

	if (msg->subsystem == CHATGUI_ID && msgevent && msgevent->getString().Cmp("Message") == 0)
	{
		BENC_SAFE_CAST(msg->headers["Message"], msgmessage, String);
		BENC_SAFE_CAST(msg->headers["From"], msgfrom, String);
		wxString nick = "";

		PeerID *pid = this->app->mesh->GetPeer(msgfrom->getString());

		if (pid)
		{		
			nick = pid->GetCertificateField(NID_commonName);
		} else {
			nick = CryptoHelper::HashToHex(msgfrom->getString(), true);
		}

		this->ChatOutput->SetDefaultStyle(wxColour(0x00, 0x66, 0x99));
		this->ChatOutput->AppendText(nick);
		this->ChatOutput->AppendText(": ");
		this->ChatOutput->SetDefaultStyle(wxColour(0x33, 0x44, 0x55));
		this->ChatOutput->AppendText(msgmessage->getString());
		this->ChatOutput->AppendText("\n");
	}*/
}

int RAIN::GUI::ChatMain::GetMessageHandlerID()
{
	return CHATGUI_ID;
}
