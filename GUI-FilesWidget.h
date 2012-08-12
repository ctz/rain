#ifndef _RAIN_GUI_FILESWIDGET_H
#define _RAIN_GUI_FILESWIDGET_H

#include <wx/wx.h>

#include <wx/treectrl.h>
#include <wx/listctrl.h>
#include <wx/splitter.h>
#include <wx/mimetype.h>
#include <wx/imaglist.h>
#include <wx/tokenzr.h>
#include <wx/bitmap.h>

#include <vector>

enum
{
	ID_FW_DIRTREE,
	ID_FW_FILELIST
};

namespace RAIN
{
	class SandboxApp;
	class ManifestEntry;

	namespace GUI
	{
		class FilesWidget : public wxPanel
		{
		public:
			FilesWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app);
			~FilesWidget();

			void Update();
			void UpdateIconType();
			void ImportManifests();

			void FillTree(const wxTreeItemId& root);
			wxTreeItemId GetNodesChild(const wxTreeItemId& node, const wxString& name);

			void FillList(const wxString& path);
			int ListImagesGetExtension(const wxString& extension);

			void FillInfo(RAIN::ManifestEntry *entry, const wxString &path);

			void OnTreeSelect(wxTreeEvent &e);
			void OnFileClick(wxListEvent &e);
			void OnFileDoubleClick(wxListEvent &e);

			wxString GetPath(const wxTreeItemId& node);
			
			DECLARE_EVENT_TABLE();

		private:
			void set_properties();
			void do_layout();

			int iconSize;
			bool useSmallIcons;

			RAIN::SandboxApp *app;

			std::vector<ManifestEntry *> entries;
			int tree_rooticon, tree_foldericon, list_foldericon;

			wxTreeCtrl *tree;
			wxPanel *right;
			wxListCtrl *list;
			wxPanel *info;
			wxStaticText* SizeLabel;
			wxStaticText* SizeText;
			wxStaticText* TotalLabel;
			wxStaticText* TotalText;
			wxStaticText* MimeLabel;
			wxStaticText* MimeText;
			wxStaticText* NeededLabel;
			wxStaticText* NeededText;
			wxImageList *listimages;
			wxSplitterWindow *split;
		};
	}
}

#endif//_RAIN_GUI_FILESWIDGET_H
