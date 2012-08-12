#include "GUI-BrowseDialog.h"
#include "GUI-ChatMain.h"
#include "MessageCore.h"
#include "BValue.h"
#include "RainSandboxApp.h"
#include "ManifestManager.h"
#include "ManifestService.h"

using namespace RAIN;
using namespace RAIN::GUI;
extern RAIN::MessageCore *globalMessageCore;

BEGIN_EVENT_TABLE(RAIN::GUI::BrowseDialog, wxDialog)
	EVT_TREE_SEL_CHANGED(ID_GBD_TREE, RAIN::GUI::BrowseDialog::OnSelect)
	EVT_BUTTON(ID_GBD_OK, RAIN::GUI::BrowseDialog::OnOk)
	EVT_BUTTON(ID_GBD_CANCEL, RAIN::GUI::BrowseDialog::OnCancel)
END_EVENT_TABLE()

enum
{
	IM_IDX_RAINROOT,
	IM_IDX_FOLDER
};

/** browsedialog is a windows-esque directory browser which instead browses the mesh
  * content.  this is always used as a modal dialog! */
BrowseDialog::BrowseDialog(wxWindow* parent)
	: wxDialog(parent, -1, wxT("Browse Virtual Drive"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxTHICK_FRAME)
{
	HelpLabel = new wxStaticText(this, -1, wxT("<instruction text here>"));
	Tree = new wxTreeCtrl(this, ID_GBD_TREE, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxSUNKEN_BORDER);
	CurrentPath = new wxTextCtrl(this, -1, wxT(""));
	Spacer = new wxPanel(this, -1);
	CancelButton = new wxButton(this, ID_GBD_CANCEL, wxT("&Cancel"));
	OkButton = new wxButton(this, ID_GBD_OK, wxT("&OK"));

	set_properties();
	do_layout();

	wxImageList *im = new wxImageList(16, 16);
	im->Add(wxBitmap("IDB_RAINROOT", wxBITMAP_TYPE_BMP_RESOURCE), wxColour(0xFF, 0x00, 0xFF));

#ifdef __WXMSW__
	im->Add(wxIcon(wxIconLocation("shell32.dll", 3)));
#endif//__WXMSW__

	this->Tree->AssignImageList(im);
	wxTreeItemId root = this->Tree->AddRoot("Virtual Drive", IM_IDX_RAINROOT);
	this->FillTree(root);
	this->Tree->Expand(root);
}

void BrowseDialog::set_properties()
{
	SetTitle(wxT("Browse Virtual Drive"));
	OkButton->SetDefault();
}

void BrowseDialog::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	MainSizer->Add(HelpLabel, 0, wxLEFT|wxRIGHT|wxTOP|wxFIXED_MINSIZE, 10);
	MainSizer->Add(Tree, 1, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 10);
	MainSizer->Add(CurrentPath, 0, wxLEFT|wxRIGHT|wxTOP|wxEXPAND|wxFIXED_MINSIZE, 10);
	ButtonSizer->Add(Spacer, 1, wxEXPAND, 0);
	ButtonSizer->Add(CancelButton, 0, wxLEFT|wxTOP|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ButtonSizer->Add(OkButton, 0, wxALL|wxFIXED_MINSIZE, 10);
	MainSizer->Add(ButtonSizer, 0, wxEXPAND, 0);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	Layout();
	this->SetSize(this->GetSize().x * 1.5, this->GetSize().y * 1.5); 
}

/** empties the treectrl, then refills it using the manifests it gets from the
  * manifestmanager */
void BrowseDialog::FillTree(const wxTreeItemId& root)
{
	std::vector<ManifestEntry *> entries;
	globalMessageCore->app->manifest->GetAllManifests(entries);
	this->Tree->DeleteChildren(root);

	for (size_t i = 0; i < entries.size(); i++)
	{
		BENC_SAFE_CAST(entries[i]->getFiles()->getDictValue("names"), filenames, List);

		for (size_t n = 0; n < filenames->getListSize(); n++)
		{
			BENC_SAFE_CAST(filenames->getListItem(n), listitem, String);
			wxString filename = listitem->getString();
			wxStringTokenizer tok(filename, "/");
			wxTreeItemId currentNode, parentNode = root;

			while (tok.HasMoreTokens())
			{
				wxString token = tok.GetNextToken();

				if (token.Length() == 0)
					continue;

				if (currentNode.IsOk())
					parentNode = currentNode;

				currentNode = this->GetNodesChild(parentNode, token);

				if (!currentNode.IsOk() && tok.HasMoreTokens())
					currentNode = this->Tree->InsertItem(parentNode, 0, token, IM_IDX_FOLDER);
			}
		}
	}

	while (entries.size() > 0)
	{
		delete entries[0];
		entries.erase(entries.begin());
	}
}

/** iterates through <b>node</b>'s children, attempting to match against <b>name</b>
  * returns the wxTreeItemId of the found node, or an invalid wxTreeItemId otherwise */
wxTreeItemId BrowseDialog::GetNodesChild(const wxTreeItemId& node, const wxString& name)
{
	wxTreeItemIdValue cookie;
	wxTreeItemId my = this->Tree->GetFirstChild(node, cookie);

	while (my.IsOk())
	{
		if (this->Tree->GetItemText(my) == name)
			return my;

		my = this->Tree->GetNextChild(node, cookie);
	}

	return my;
}

/** single left click handler for the treectrl - decends the tree and builds a pathname 
  * into CurrentPath */
void BrowseDialog::OnSelect(wxTreeEvent &e)
{
	if (e.GetItem() == this->Tree->GetRootItem())
	{
		this->CurrentPath->SetValue("/");
		return;
	}

	this->CurrentPath->SetValue("");

	wxTreeItemId id = e.GetItem();
	while (id != this->Tree->GetRootItem())
	{
		this->CurrentPath->SetValue("/" + this->Tree->GetItemText(id) + this->CurrentPath->GetValue());
		id = this->Tree->GetItemParent(id);
	}

	this->CurrentPath->SetValue(this->CurrentPath->GetValue() + "/" );
}

/** click handler for ok button */
void BrowseDialog::OnOk(wxCommandEvent &e)
{
	this->EndModal(true);
}

/** click handler for cancel button */
void BrowseDialog::OnCancel(wxCommandEvent &e)
{
	this->EndModal(false);
}


