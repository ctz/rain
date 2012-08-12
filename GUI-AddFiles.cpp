#include "GUI-AddFiles.h"
#include "GUI-BrowseDialog.h"
#include "ManifestService.h"
#include "PieceService.h"
#include "BValue.h"

using namespace RAIN;
using namespace RAIN::GUI;

BEGIN_EVENT_TABLE(AddFilesDialog, wxDialog)
	EVT_BUTTON(ID_AF_ADDFOLDER, AddFilesDialog::OnAddFolder)
	EVT_BUTTON(ID_AF_ADDFILES, AddFilesDialog::OnAddFiles)
	EVT_BUTTON(ID_AF_REMOVE, AddFilesDialog::OnRemove)
	EVT_BUTTON(ID_AF_PATHBROWSE, AddFilesDialog::OnPathBrowse)
	EVT_BUTTON(ID_AF_CANCEL, AddFilesDialog::OnCancel)
	EVT_BUTTON(ID_AF_ADD, AddFilesDialog::OnAdd)

	EVT_TEXT(ID_AF_PATH, AddFilesDialog::OnPathUpdate)

	EVT_CLOSE(AddFilesDialog::OnClose)
END_EVENT_TABLE()

AddFilesDialog::AddFilesDialog(wxWindow* parent)
	: wxDialog(parent, -1, wxT("Add Files to Network"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxTHICK_FRAME)
{
	DestinationSizer_staticbox = new wxStaticBox(this, -1, wxT(" Destination "));
	LongevitySizer_staticbox = new wxStaticBox(this, -1, wxT(" Piece Security/Longevity "));
	OptionsSizer_staticbox = new wxStaticBox(this, -1, wxT(" Options "));
	FilesSizer_staticbox = new wxStaticBox(this, -1, wxT(" Files "));
	FileList = new wxListCtrl(this, -1, wxDefaultPosition, wxDefaultSize, wxLC_REPORT|wxLC_VRULES|wxSUNKEN_BORDER|wxLC_NO_SORT_HEADER);
	AddFolderButton = new wxButton(this, ID_AF_ADDFOLDER, wxT("Add Folder..."));
	AddFilesButton = new wxButton(this, ID_AF_ADDFILES, wxT("Add Files..."));
	RemoveButton = new wxButton(this, ID_AF_REMOVE, wxT("Remove"));
	DestinationText = new wxTextCtrl(this, ID_AF_PATH, wxT("/"));
	DestinationBrowseButton = new wxButton(this, ID_AF_PATHBROWSE, wxT("Browse..."));
	const wxString LongevityCombobox_choices[] = {
		wxT("Normal"),
		wxT("Medium (x1.5)"),
		wxT("High (x2)"),
		wxT("Insane (x3)")
	};
	LongevityCombobox = new wxComboBox(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, 4, LongevityCombobox_choices, wxCB_DROPDOWN|wxCB_SIMPLE|wxCB_READONLY);
	PropogateCheckbox = new wxCheckBox(this, -1, wxT("Propogate Manifest?"));
	RemoveAfterPropCheckbox = new wxCheckBox(this, -1, wxT("Remove pieces from local store?"));
	ButtonSpacer = new wxPanel(this, -1);
	CancelButton = new wxButton(this, ID_AF_CANCEL, wxT("&Cancel"));
	AddButton = new wxButton(this, ID_AF_ADD, wxT("Add"));

	set_properties();
	do_layout();

	/* disable gui controls for unimplemented features */
	this->LongevityCombobox->Enable(false);
	this->PropogateCheckbox->Enable(false);
	this->RemoveAfterPropCheckbox->Enable(false);

	this->progress = NULL;

	this->FileList->ClearAll();
	this->FileList->InsertColumn(0, wxT("Local Path"));
	this->FileList->InsertColumn(1, wxT("Size (bytes)"));
	this->FileList->InsertColumn(2, wxT("Mesh Path"));
}

AddFilesDialog::~AddFilesDialog()
{
	for (size_t i = 0; i < this->files.size(); i++)
	{
		delete this->files.at(i);
	}

	this->files.erase(this->files.begin(), this->files.end());
}

void AddFilesDialog::set_properties()
{
	SetTitle(wxT("Add Files"));
	LongevityCombobox->SetSelection(0);
	PropogateCheckbox->SetValue(1);
	AddButton->SetDefault();
}

void AddFilesDialog::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* OptionsSizer = new wxStaticBoxSizer(OptionsSizer_staticbox, wxHORIZONTAL);
	wxStaticBoxSizer* LongevitySizer = new wxStaticBoxSizer(LongevitySizer_staticbox, wxHORIZONTAL);
	wxStaticBoxSizer* DestinationSizer = new wxStaticBoxSizer(DestinationSizer_staticbox, wxHORIZONTAL);
	wxStaticBoxSizer* FilesSizer = new wxStaticBoxSizer(FilesSizer_staticbox, wxHORIZONTAL);
	wxBoxSizer* FileButtonsSizer = new wxBoxSizer(wxVERTICAL);
	FilesSizer->Add(FileList, 1, wxALL|wxEXPAND, 2);
	FileButtonsSizer->Add(AddFolderButton, 0, wxLEFT|wxRIGHT|wxTOP|wxFIXED_MINSIZE, 5);
	FileButtonsSizer->Add(AddFilesButton, 0, wxLEFT|wxRIGHT|wxTOP|wxFIXED_MINSIZE, 5);
	FileButtonsSizer->Add(RemoveButton, 0, wxLEFT|wxRIGHT|wxTOP|wxFIXED_MINSIZE, 5);
	FilesSizer->Add(FileButtonsSizer, 0, wxEXPAND, 0);
	DestinationSizer->Add(DestinationText, 1, wxALL|wxFIXED_MINSIZE, 5);
	DestinationSizer->Add(DestinationBrowseButton, 0, wxRIGHT|wxTOP|wxBOTTOM|wxFIXED_MINSIZE, 5);
	MainSizer->Add(DestinationSizer, 0, wxALL|wxEXPAND, 5);
	MainSizer->Add(FilesSizer, 1, wxALL|wxEXPAND, 5);
	LongevitySizer->Add(LongevityCombobox, 1, wxALL|wxFIXED_MINSIZE, 5);
	MainSizer->Add(LongevitySizer, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	OptionsSizer->Add(PropogateCheckbox, 1, wxALL|wxFIXED_MINSIZE, 5);
	OptionsSizer->Add(RemoveAfterPropCheckbox, 1, wxALL|wxFIXED_MINSIZE, 5);
	MainSizer->Add(OptionsSizer, 0, wxALL|wxEXPAND, 5);
	ButtonSizer->Add(ButtonSpacer, 1, wxEXPAND, 0);
	ButtonSizer->Add(AddButton, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 5);
	ButtonSizer->Add(CancelButton, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 5);
	MainSizer->Add(ButtonSizer, 0, wxEXPAND, 0);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	Layout();
	this->SetSize((int) this->GetSize().x * 1.5, (int) this->GetSize().y * 1.5);
}

void AddFilesDialog::OnAddFolder(wxCommandEvent &e)
{
	wxDirDialog *dialog = new wxDirDialog(this, wxT("Select a directory to add"), wxEmptyString, 0);

	if (wxID_OK == dialog->ShowModal())
	{
		this->progress = new wxProgressDialog("Adding directory...", "Adding " + dialog->GetPath(), 100, this);
		this->AddDir(dialog->GetPath(), wxFileName());
		delete this->progress;
		this->progress = NULL;
		this->UpdateFiles();
	}

	dialog->Destroy();
}

void AddFilesDialog::AddDir(const wxString& path, const wxFileName& base_offset)
{
	if (::wxDirExists(path))
	{
		if (this->progress)
			this->progress->Update(0, "Adding " + path);

		::wxSafeYield();

		wxDir d(path);

		if (d.IsOpened() && d.HasFiles())
		{
			wxString file;
			bool cont = d.GetFirst(&file);

			while (cont)
			{
				wxFileName fn(path, file);
				
				if (fn.DirExists())
				{
					wxFileName base(base_offset);
					base.AppendDir(file);
					this->AddDir(fn.GetFullPath(), base);
				} else if (fn.FileExists()) {
					this->files.push_back(new ManifestCreationFile(base_offset.GetDirs(), fn));
				}

				cont = d.GetNext(&file);
			}
		}
	} else {
		// do nothing
	}
}

void AddFilesDialog::OnAddFiles(wxCommandEvent &e)
{
	wxFileDialog *dialog = new wxFileDialog(this, wxT("Select Files to Add"), wxEmptyString, wxEmptyString, wxT("All Files (*.*)|*.*"), wxOPEN|wxHIDE_READONLY|wxMULTIPLE|wxFILE_MUST_EXIST);
	
	if (wxID_OK == dialog->ShowModal())
	{
		wxArrayString files;
		dialog->GetPaths(files);

		for (int i = 0; i < files.Count(); i++)
		{
			this->files.push_back(new ManifestCreationFile(files[i]));
		}

		this->UpdateFiles();
	}

	dialog->Destroy();
}

void AddFilesDialog::UpdateFiles()
{
	this->FileList->ClearAll();
	this->FileList->InsertColumn(0, wxT("Local Path"));
	this->FileList->InsertColumn(1, wxT("Size (bytes)"));
	this->FileList->InsertColumn(2, wxT("Mesh Path"));

	if (this->files.size() == 0)
		return;

	for (size_t i = 0; i < this->files.size(); i++)
	{
		this->FileList->InsertItem(i, this->files.at(i)->filename.GetFullPath());
		this->FileList->SetItem(i, 1, wxString::Format("%d", this->files.at(i)->fileSize));
		this->FileList->SetItem(i, 2, this->files.at(i)->GetTranslatedPath(this->DestinationText->GetValue()));
	}

	this->FileList->SetColumnWidth(0, -1);
	this->FileList->SetColumnWidth(1, -1);
	this->FileList->SetColumnWidth(2, -1);
}

void AddFilesDialog::OnRemove(wxCommandEvent &e)
{
	std::vector<ManifestCreationFile*> toDelete;

	for (int i = 0; i < this->FileList->GetItemCount(); i++)
	{
		if (this->FileList->GetItemState(i, 0) & wxLIST_STATE_SELECTED)
		{
			toDelete.push_back(this->files.at(i));
		}
	}

	for (int i = 0; i < toDelete.size(); i++)
	{
		for (int n = 0; n < this->files.size(); n++)
		{
			if (toDelete.at(i) == this->files.at(n))
			{
				this->files.erase(this->files.begin() + n);
				break;
			}
		}
	}

	if (toDelete.size() > 0)
		this->UpdateFiles();
}

void AddFilesDialog::OnPathBrowse(wxCommandEvent &e)
{
	GUI::BrowseDialog *dialog = new GUI::BrowseDialog(this);

	dialog->HelpLabel->SetLabel(wxT("Select a location."));
	
	if (dialog->ShowModal())
	{
		this->DestinationText->SetValue(dialog->CurrentPath->GetValue());
	}

	dialog->Destroy();
}

void AddFilesDialog::OnPathUpdate(wxCommandEvent &e)
{
	this->UpdateFiles();
}

void AddFilesDialog::OnCancel(wxCommandEvent &e)
{
	this->EndModal(false);
}

void AddFilesDialog::OnAdd(wxCommandEvent &e)
{
	if (this->files.size() == 0)
	{
		wxMessageBox("No files to add!");
	}

	ManifestCreationResult result = ManifestService::Create(&(this->files), this->DestinationText->GetValue());
	
	if (result.errorCode == MCE_ERROR_OK)
	{
		wxMessageBox("Manifest and pieces created OK");
		this->EndModal(true);
	} else {
		wxMessageBox("Creation failed: " + result.error);
	}

	delete result.manifest;
}

void AddFilesDialog::OnClose(wxCloseEvent &e)
{
	this->EndModal(false);
}
