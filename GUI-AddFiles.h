#ifndef __GUI_ADDFILES_H
#define __GUI_ADDFILES_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/listctrl.h>
#include <wx/filedlg.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/dir.h>
#include <wx/progdlg.h>

#include <vector>

enum
{
	ID_AF_ADDFOLDER,
	ID_AF_ADDFILES,
	ID_AF_REMOVE,
	ID_AF_PATH,
	ID_AF_PATHBROWSE,
	ID_AF_CANCEL,
	ID_AF_ADD
};

namespace RAIN
{
	class ManifestCreationFile;

	namespace GUI
	{
		class AddFilesDialog : public wxDialog
		{
		public:
			AddFilesDialog(wxWindow* parent);
			~AddFilesDialog();

			void OnAddFolder(wxCommandEvent &e);
			void AddDir(const wxString& path, const wxFileName& base_offset);

			void OnAddFiles(wxCommandEvent &e);
			void OnRemove(wxCommandEvent &e);
			void OnPathBrowse(wxCommandEvent &e);
			void OnCancel(wxCommandEvent &e);
			void OnAdd(wxCommandEvent &e);

			void OnPathUpdate(wxCommandEvent &e);

			void OnClose(wxCloseEvent &e);
			
			DECLARE_EVENT_TABLE();

		private:
			void set_properties();
			void do_layout();

			wxProgressDialog *progress;

			std::vector<ManifestCreationFile*> files;

			void UpdateFiles();

		protected:
			wxStaticBox* OptionsSizer_staticbox;
			wxStaticBox* LongevitySizer_staticbox;
			wxStaticBox* DestinationSizer_staticbox;
			wxStaticBox* FilesSizer_staticbox;
			wxListCtrl* FileList;
			wxButton* AddFolderButton;
			wxButton* AddFilesButton;
			wxButton* RemoveButton;
			wxTextCtrl* DestinationText;
			wxButton* DestinationBrowseButton;
			wxComboBox* LongevityCombobox;
			wxCheckBox* PropogateCheckbox;
			wxCheckBox* RemoveAfterPropCheckbox;
			wxPanel* ButtonSpacer;
			wxButton* CancelButton;
			wxButton* AddButton;
		};
	}
}

#endif //__GUI_ADDFILES_H
