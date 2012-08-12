#ifndef _RAIN_GUI_CHATMAIN_H
#define _RAIN_GUI_CHATMAIN_H

#include <wx/wx.h>
#include <wx/notebook.h>

#include "Message.h"

enum
{
	ID_MENUEXIT = 201,
	ID_MENUCONNECT,
	ID_MENUDISCONNECT,
	ID_MENUFORCEMANIFEST,
	ID_MENUPREFS,
	
	ID_UPDATESTATS,
	ID_PINGPEER,
	ID_TRACEPEER,
	
	ID_DBGCONNECTION,
	ID_DBGWIZARD,
	ID_DBGADDFILES,
	
	ID_HELPABOUT,
};

namespace RAIN
{
	class SandboxApp;

	namespace GUI
	{
		class ConnectionsWidget;
		class PeersWidget;
		class QueueWidget;
		class FilesWidget;
		class NetworkMonitor;
		class Chat;

		class ChatMain : public wxFrame, public RAIN::MessageHandler
		{
		public:
			ChatMain(RAIN::SandboxApp *app);
			~ChatMain();

			void OnQuit(wxCommandEvent &e);

			void OnPrefs(wxCommandEvent &e);

			void OnConnect(wxCommandEvent &e);
			void OnDisconnect(wxCommandEvent &e);
			void OnManifestUpdate(wxCommandEvent &e);

			void OnDbgConnProps(wxCommandEvent &e);
			void OnDbgWizard(wxCommandEvent &e);
			void OnDbgAddFiles(wxCommandEvent &e);

			void OnAbout(wxCommandEvent &e);

			void UpdateStats(wxCommandEvent &e);
			void OnPingPeer(wxCommandEvent &e);
			void OnTracePeer(wxCommandEvent &e);

			void OnClose(wxCloseEvent &e);
			void OnChatTextEnter(wxCommandEvent &e);

			/* messagehandler impl */
			void HandleMessage(RAIN::Message *msg);
			int GetMessageHandlerID();
			void Tick();

			wxListBox*	ConversationText;
			NetworkMonitor* Network;
			RAIN::GUI::QueueWidget* Queue;
			
			DECLARE_EVENT_TABLE();
		private:
			RAIN::SandboxApp *app;

			void set_properties();
			void do_layout();

		protected:
			RAIN::GUI::ConnectionsWidget* Connections;
			RAIN::GUI::PeersWidget* Peers;
			RAIN::GUI::FilesWidget* Files;
			RAIN::GUI::Chat *Chat;
			wxButton* UpdateButton, *PingButton, *TracerouteButton;
			wxPanel* FilesPage;
			wxPanel* ChatPage;
			wxPanel* ConnectionsPage;
			wxPanel* QueuePage;
			wxPanel* LogPage;
			wxNotebook* Notebook;
			wxMenuBar* MenuBar;
		};
	};
};

#include "RainSandboxApp.h"

#endif//_RAIN_GUI_CHATMAIN_H
