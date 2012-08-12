#ifndef __GUI_BROWSEDIALOG_H
#define __GUI_BROWSEDIALOG_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/tokenzr.h>

enum
{
	ID_GBD_TREE,
	ID_GBD_OK,
	ID_GBD_CANCEL
};

namespace RAIN
{
	namespace GUI
	{
		class BrowseDialog: public wxDialog
		{
		public:
			BrowseDialog(wxWindow* parent);

			void FillTree(const wxTreeItemId& root);
			wxTreeItemId GetNodesChild(const wxTreeItemId& node, const wxString& name);

			void OnSelect(wxTreeEvent &e);
			void OnOk(wxCommandEvent &e);
			void OnCancel(wxCommandEvent &e);

			wxStaticText* HelpLabel;
			wxTextCtrl* CurrentPath;

			DECLARE_EVENT_TABLE();

		private:
			void set_properties();
			void do_layout();

		protected:
			wxTreeCtrl* Tree;
			wxPanel* Spacer;
			wxButton* CancelButton;
			wxButton* OkButton;
		};
	}
}

#endif // __GUI_BROWSEDIALOG_H
