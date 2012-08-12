#ifndef __GUI_CHAT_H
#define __GUI_CHAT_H

#include <wx/wx.h>
#include <wx/tabctrl.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>

#include "Message.h"

namespace RAIN
{
	namespace GUI
	{
		class ChatMain;
		class ChatPanel;
		class Chat;

		class ChatUsersDialog : public wxDialog
		{
		public:
			ChatUsersDialog(Chat* parent);

			void OnJoinClick(wxCommandEvent &e);
			void OnCancelClick(wxCommandEvent &e);
			void OnSelectClick(wxCommandEvent &e);

			wxString user;

			DECLARE_EVENT_TABLE();

		private:
			void set_properties();
			void do_layout();

			wxCheckBox *TypeBox;
			wxListBox *UserList;
			wxPanel *ButtonSpacer;
			wxButton *CancelButton;
			wxButton *ChatButton;
		};

		class ChatChannelsDialog : public wxDialog
		{
		public:
			ChatChannelsDialog(Chat* parent);

			void OnJoinClick(wxCommandEvent &e);
			void OnCancelClick(wxCommandEvent &e);
			void OnNewClick(wxCommandEvent &e);
			void OnSelectClick(wxCommandEvent &e);

			wxString channel;

			DECLARE_EVENT_TABLE();

		private:
			void set_properties();
			void do_layout();

			wxListBox *ChannelBox;
			wxButton *NewButton;
			wxPanel *ButtonSpacer;
			wxButton *CancelButton;
			wxButton *JoinButton;
		};

		class ChatTabs : public wxNotebook
		{
		public:
			ChatTabs(wxWindow *parent, wxWindowID id) : wxNotebook(parent, id) {};
			void OnTabRightClick(wxMouseEvent &e);
			int lasttab;

			DECLARE_EVENT_TABLE();
		};

		class Chat : public wxPanel, public RAIN::MessageHandler
		{
		public:
			Chat(wxWindow *parent, wxWindowID id);

			void HandleMessage(RAIN::Message *msg);
			int GetMessageHandlerID();
			void Tick();

			void OnChansClick(wxCommandEvent &e);
			void OnUsersClick(wxCommandEvent &e);
			void OnCloseTab(wxCommandEvent &e);

			RAIN::GUI::ChatPanel *GetPanel(const wxString& name);

			void SendSync(const wxString &hash);
			bool MergeInfo(BEnc::Dictionary *i);

			BEnc::Dictionary *info;
			time_t infotime;

			DECLARE_EVENT_TABLE();

		private:
			ChatTabs *tabs;
			wxButton *chans, *users;
		};

		class ChatPanel : public wxPanel
		{
		public:
			ChatPanel(wxWindow *parent, wxWindowID id, const wxString& channel);

			void OnTextEnter(wxCommandEvent &e);

			void DoJoin(BEnc::Dictionary *d);
			void DoPart(BEnc::Dictionary *d);

			void OnPart(const wxString &hash);
			void OnJoin(const wxString &hash);

			void UpdateUserlist(BEnc::List *users);
			void ProcessMessage(RAIN::Message *m);

			DECLARE_EVENT_TABLE();
			
		private:
			wxString channel;
			wxTextCtrl *text, *input;
			wxSplitterWindow *splitter;
			wxListBox *users;
		};
	}
}

#endif//__GUI_CHAT_H
