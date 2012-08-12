#include "GUI-FilesWidget.h"
#include "GUI-Helpers.h"

#include "MessageCore.h"
#include "BValue.h"
#include "RainSandboxApp.h"
#include "ManifestManager.h"
#include "ManifestService.h"
#include "PieceManager.h"
#include "PieceQueue.h"
#include "PieceService.h"
#include "CredentialManager.h"

#include "_wxconfig.h"

#include <wx/listctrl.h>

using namespace RAIN;
extern RAIN::MessageCore *globalMessageCore;
extern RAIN::CredentialManager *globalCredentialManager;

BEGIN_EVENT_TABLE(RAIN::GUI::FilesWidget, wxPanel)
	EVT_TREE_SEL_CHANGED(ID_FW_DIRTREE, RAIN::GUI::FilesWidget::OnTreeSelect)
	EVT_LIST_ITEM_ACTIVATED(ID_FW_FILELIST, RAIN::GUI::FilesWidget::OnFileDoubleClick)
	EVT_LIST_ITEM_SELECTED(ID_FW_FILELIST, RAIN::GUI::FilesWidget::OnFileClick)
END_EVENT_TABLE()

GUI::FilesWidget::FilesWidget(wxWindow *parent, wxWindowID id, RAIN::SandboxApp *app)
	: wxPanel(parent, id)
{
	this->app = app;

	this->UpdateIconType();

	this->split = new wxSplitterWindow(this, -1, wxDefaultPosition, wxDefaultSize , wxSP_LIVE_UPDATE|wxSP_NO_XP_THEME);
	this->tree = new wxTreeCtrl(this->split, ID_FW_DIRTREE, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS|wxSUNKEN_BORDER);
	this->right = new wxPanel(this->split, -1);
	this->list = new wxListCtrl(this->right, ID_FW_FILELIST, wxDefaultPosition, wxDefaultSize, (this->useSmallIcons) ? wxLC_LIST|wxLC_AUTOARRANGE|wxLC_NO_SORT_HEADER : wxLC_ICON|wxLC_AUTOARRANGE|wxLC_NO_SORT_HEADER);
	this->info = new wxPanel(this->right, -1);

    this->SizeLabel = new wxStaticText(this->info, -1, wxT("Size:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->SizeText = new wxStaticText(this->info, -1, wxT("2.1MB (2,100,832 bytes)"));

	this->TotalLabel = new wxStaticText(this->info, -1, wxT("Total Piece Size:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->TotalText = new wxStaticText(this->info, -1, wxT("61.3MB (20 pieces)"));

	this->MimeLabel = new wxStaticText(this->info, -1, wxT("Mime type:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->MimeText = new wxStaticText(this->info, -1, wxT("audio/x-mp3"));

	this->NeededLabel = new wxStaticText(this->info, -1, wxT("Needed Piece Size:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->NeededText = new wxStaticText(this->info, -1, wxT("2.0MB (4 pieces)"));

	set_properties();
	do_layout();

	this->ImportManifests();

	wxImageList *im = new wxImageList(16, 16);
	this->listimages = new wxImageList(this->iconSize, this->iconSize);
	
#ifdef __WXMSW__
	this->tree_rooticon = im->Add(wxBitmap("IDB_RAINROOT", wxBITMAP_TYPE_BMP_RESOURCE), wxColour(0xFF, 0x00, 0xFF));
	this->tree_foldericon = im->Add(wxIcon(wxIconLocation("shell32.dll", 3)));

	this->list_foldericon = listimages->Add(wxIcon(wxIconLocation("shell32.dll", 3)));
#else
	this->tree_rooticon = -1;
	this->tree_foldericon = -1;
	
	this->list_foldericon = -1;
#endif//__WXMSW__

	this->list->AssignImageList(this->listimages, (this->useSmallIcons) ? wxIMAGE_LIST_SMALL : wxIMAGE_LIST_NORMAL);

	this->tree->AssignImageList(im);
	wxTreeItemId root = this->tree->AddRoot("Virtual Drive", this->tree_rooticon);
	this->FillTree(root);
	this->tree->Expand(root);

	this->FillInfo(NULL, "");
}

GUI::FilesWidget::~FilesWidget()
{
	while (this->entries.size() > 0)
	{
		delete this->entries[0];
		this->entries.erase(this->entries.begin());
	}
}

void GUI::FilesWidget::set_properties()
{
	this->split->SplitVertically(this->tree, this->right, 120);
	this->split->SetMinimumPaneSize(50);

	SizeLabel->SetSize(wxSize(100, -1));
	SizeLabel->SetFont(wxFont(-1, wxDEFAULT, wxNORMAL, wxBOLD));
	TotalLabel->SetSize(wxSize(100, -1));
	TotalLabel->SetFont(wxFont(-1, wxDEFAULT, wxNORMAL, wxBOLD));
	MimeLabel->SetSize(wxSize(100, -1));
	MimeLabel->SetFont(wxFont(-1, wxDEFAULT, wxNORMAL, wxBOLD));
	NeededLabel->SetSize(wxSize(100, -1));
	NeededLabel->SetFont(wxFont(-1, wxDEFAULT, wxNORMAL, wxBOLD));
}

void GUI::FilesWidget::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(this->split, 1, wxALL|wxEXPAND, 5);

	wxBoxSizer* RightSizer = new wxBoxSizer(wxVERTICAL);
	RightSizer->Add(this->list, 1, wxEXPAND, 0);
	RightSizer->Add(this->info, 0, wxEXPAND, 0);

	this->right->SetAutoLayout(true);
	this->right->SetSizer(RightSizer);
	RightSizer->Fit(this->right);
	RightSizer->SetSizeHints(this->right);

	wxFlexGridSizer* InfoGrid = new wxFlexGridSizer(2, 4, 0, 0);
	InfoGrid->Add(SizeLabel, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(SizeText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(TotalLabel, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(TotalText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(MimeLabel, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(MimeText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(NeededLabel, 0, wxALL|wxEXPAND|wxALIGN_CENTER_VERTICAL, 5);
	InfoGrid->Add(NeededText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
	this->info->SetAutoLayout(true);
	this->info->SetSizer(InfoGrid);
	InfoGrid->Fit(this->info);
	InfoGrid->SetSizeHints(this->info);
	InfoGrid->AddGrowableCol(1);
	InfoGrid->AddGrowableCol(3);

	this->SetAutoLayout(true);
	this->SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	this->Layout();
}

void GUI::FilesWidget::Update()
{
	this->ImportManifests();
	this->FillTree(this->tree->GetRootItem());
	this->tree->Expand(this->tree->GetRootItem());
	this->FillInfo(NULL, "");
}

void GUI::FilesWidget::UpdateIconType()
{
	wxConfig *cfg = (wxConfig *) wxConfig::Get();
	this->useSmallIcons = (cfg->Read("/SmallIcons", (long) 0) == 1);
	this->iconSize = (useSmallIcons) ? 16 : 32;
}

void GUI::FilesWidget::ImportManifests()
{
	while (this->entries.size() > 0)
	{
		delete this->entries[0];
		this->entries.erase(this->entries.begin());
	}

	globalMessageCore->app->manifest->GetAllManifests(this->entries);
}

void GUI::FilesWidget::FillTree(const wxTreeItemId &root)
{
	this->tree->DeleteChildren(root);

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
					currentNode = this->tree->InsertItem(parentNode, 0, token, this->tree_foldericon);
			}
		}
	}
}

void GUI::FilesWidget::FillList(const wxString& path)
{
	this->list->DeleteAllItems();
	this->list->Freeze();
	wxBusyCursor busy;

	for (size_t i = 0; i < entries.size(); i++)
	{
		BENC_SAFE_CAST(entries[i]->getFiles()->getDictValue("names"), filenames, List);

		for (size_t n = 0; n < filenames->getListSize(); n++)
		{
			BENC_SAFE_CAST(filenames->getListItem(n), listitem, String);
			wxString pathname = listitem->getString();

			wxString file;

			if (pathname.StartsWith(path.c_str(), &file))
			{
				if (file.GetChar(0) == '/')
					file = file.Mid(1);

				wxString extension = file.AfterLast('.');
				
				if (file.Freq('/') == 0)
				{
					this->list->InsertItem(0, file, this->ListImagesGetExtension(extension));
				} else {
					if (this->list->FindItem(-1, file.BeforeFirst('/')) == -1)
						this->list->InsertItem(0, file.BeforeFirst('/'), this->ListImagesGetExtension("folder"));
				}
			}
		}
	}

	this->list->Thaw();
}

void GUI::FilesWidget::FillInfo(RAIN::ManifestEntry *entry, const wxString &path)
{
	if (entry)
	{
		BEnc::Dictionary *files = entry->getFiles();
		BENC_SAFE_CAST(files->getDictValue("names"), filenames, List);
		BENC_SAFE_CAST(files->getDictValue("sizes"), filesizes, List);

		unsigned long totalSize = 0;

		for (size_t i = 0; i < filenames->getListSize(); i++)
		{
			BENC_SAFE_CAST(filenames->getListItem(i), filename, String);
			BENC_SAFE_CAST(filesizes->getListItem(i), filesize, Integer);
			totalSize += filesize->getNum();

			if (filename->getString() == path)
			{
				this->SizeText->SetLabel(GUI::FormatFileSize(filesize->getNum()));

				wxFileType *type = wxTheMimeTypesManager->GetFileTypeFromExtension(path.AfterLast('.'));

				if (type)
				{
					wxString mimetype;
					type->GetMimeType(&mimetype);
					this->MimeText->SetLabel(mimetype);

					delete type;
				} else {
					this->MimeText->SetLabel("unknown");
				}
			}
		}

		BEnc::Dictionary *pieces = entry->getPieces();
		BENC_SAFE_CAST(pieces->getDictValue("hashes"), piecehashes, List);
		BENC_SAFE_CAST(pieces->getDictValue("size"), piecesize, Integer);

		int needed = piecehashes->getListSize();

		for (size_t i = 0; i < piecehashes->getListSize(); i++)
		{
			BENC_SAFE_CAST(piecehashes->getListItem(i), piecehash, String);
			
			if (piecehash)
			{
				if (globalMessageCore->app->pieces->HasGotPiece(piecehash->getString()))
					needed--;
			}
		}

		long neededsize = needed * piecesize->getNum();

		if (needed == 0)
			this->NeededText->SetLabel(wxT("None - ready to unpack"));
		else
			this->NeededText->SetLabel(wxString::Format("%.02fMB (%d pieces)", neededsize / (float) (1024 * 1024), needed));
		this->TotalText->SetLabel(GUI::FormatFileSize(totalSize));
	} else {
		this->SizeText->SetLabel("");
		this->MimeText->SetLabel("");
		this->NeededText->SetLabel("");
		this->TotalText->SetLabel("");
	}
}

int GUI::FilesWidget::ListImagesGetExtension(const wxString& extension)
{
#ifdef __WXMSW__
	wxFileType *ft = wxTheMimeTypesManager->GetFileTypeFromExtension(extension);

	if (ft)
	{
		wxIconLocation icl;
		ft->GetIcon(&icl);

		delete ft;
		
		wxIcon icon;

		if (icl.GetFileName() == "%1")
			icon = wxIcon(wxIconLocation("shell32.dll", 2));
		
		if (icl.GetFileName() == "")
			icon = wxIcon(wxIconLocation("shell32.dll", 0));
			
		if (extension == "folder")
			return this->list_foldericon;
		
		if (!icon.Ok())
			icon = wxIcon(icl);

		if (icon.Ok())
			return this->listimages->Add(icon);
		else
			return this->listimages->Add(wxIcon(wxIconLocation("shell32.dll", 0)));
	} else {
		return this->listimages->Add(wxIcon(wxIconLocation("shell32.dll", 0)));
	}
#else
	return -1;
#endif//__WXMSW__	
}

/** iterates through <b>node</b>'s children, attempting to match against <b>name</b>
  * returns the wxTreeItemId of the found node, or an invalid wxTreeItemId otherwise */
wxTreeItemId GUI::FilesWidget::GetNodesChild(const wxTreeItemId& node, const wxString& name)
{
	wxTreeItemIdValue cookie;
	wxTreeItemId my = this->tree->GetFirstChild(node, cookie);

	while (my.IsOk())
	{
		if (this->tree->GetItemText(my) == name)
			return my;

		my = this->tree->GetNextChild(node, cookie);
	}

	return my;
}

void GUI::FilesWidget::OnTreeSelect(wxTreeEvent &e)
{
	wxString path = this->GetPath(e.GetItem());
	if (path.Length() != 1)
		path.Append("/");
	this->FillList(path);
}

void GUI::FilesWidget::OnFileClick(wxListEvent &e)
{
	wxListItem item;
    item.SetMask(wxLIST_MASK_TEXT|wxLIST_MASK_IMAGE);
	item.SetId(e.GetIndex());
	this->list->GetItem(item);

	wxString path = this->GetPath(this->tree->GetSelection());
	wxString file = this->list->GetItemText(e.GetIndex());

	if (path.Length() != 1)
		path.Append("/");
	path.Append(file);

	if (item.GetImage() == this->list_foldericon)
	{
		this->FillInfo(NULL, "");
		return;
	}

	// we clicked a file, so first get all our manifests and find which it belongs to
	for (size_t i = 0; i < entries.size(); i++)
	{
		BENC_SAFE_CAST(entries[i]->getFiles()->getDictValue("names"), filenames, List);

		for (size_t n = 0; n < filenames->getListSize(); n++)
		{
			BENC_SAFE_CAST(filenames->getListItem(n), listitem, String);
			
			if (listitem->getString() == path)
			{
				this->FillInfo(entries[i], listitem->getString());
				return;
			}
		}
	}
}

void GUI::FilesWidget::OnFileDoubleClick(wxListEvent &e)
{
	wxListItem item;
    item.SetMask(wxLIST_MASK_TEXT|wxLIST_MASK_IMAGE);
	item.SetId(e.GetIndex());
	this->list->GetItem(item);

	wxLogVerbose("double clicked %d", e.GetIndex());

	wxString path = this->GetPath(this->tree->GetSelection());
	wxString file = this->list->GetItemText(e.GetIndex());

	if (path.Length() != 1) 
		path.Append("/");
	path.Append(file);

	wxLogVerbose("clicked %s", path.c_str());

	wxLogVerbose("image = %d", item.GetImage());

	if (item.GetImage() == this->list_foldericon)
	{
		wxTreeItemId root = this->tree->GetSelection();

		wxTreeItemIdValue cookie;
		this->tree->Expand(root);
		wxTreeItemId item = this->tree->GetFirstChild(root, cookie);

		while (item.IsOk())
		{
			if (this->tree->GetItemText(item) == file)
			{
				this->tree->SelectItem(item);
				return;
			}

			item = this->tree->GetNextChild(root, cookie);
		}
	}

	// we clicked a file, so first get all our manifests and find which it belongs to
	for (size_t i = 0; i < entries.size(); i++)
	{
		BENC_SAFE_CAST(entries[i]->getFiles()->getDictValue("names"), filenames, List);

		for (size_t n = 0; n < filenames->getListSize(); n++)
		{
			BENC_SAFE_CAST(filenames->getListItem(n), listitem, String);
			wxString mpath = listitem->getString();
			wxLogVerbose("mpath = %s", mpath.c_str());
			
			if (mpath == path)
			{
				if (PieceService::HasAllPieces(entries[i]))
				{
					if (wxOK == ::wxMessageBox("Really unpack this manifest?", "Unpack?", wxICON_QUESTION|wxOK|wxCANCEL))
					{
						int ret = PieceService::Unpack(entries[i]);

						if (ret == PUR_OK)
						{
							wxLogVerbose("PieceService::Unpack said %d", ret);
							GUI::OpenInShell(CredentialManager::GetCredentialFilename(RAIN::OUTPUT_DIR));
						} else if (ret == PUR_MISSING_PIECES) {
							::wxMessageBox("Missing pieces");
						} else if (ret == PUR_BAD_MANIFEST) {
							::wxMessageBox("Invalid or damaged manifest");
						}
					}
				} else {
					if (wxOK == ::wxMessageBox("Add missing pieces to the queue for download?", "Download?", wxICON_QUESTION|wxOK|wxCANCEL))
					{
						int added = globalMessageCore->app->pieceQueue->QueueMissingPieces(entries[i]);

						if (added == 0)
							::wxMessageBox("Error: PieceService::HasAllPieces reports we do not have all the pieces, however PieceQueue::QueueMissingPieces reports it did not add any new queues!\n\nPerhaps you already have the pieces for this manifest queued.", "Error", wxOK|wxICON_WARNING);
						else
							::wxMessageBox(wxString::Format("Added %d pieces to the queue for download. Start the queue to begin downloading.", added), "Queued for download", wxOK|wxICON_INFORMATION);
					}
				}

				return;
			}
		}
	}
}

wxString GUI::FilesWidget::GetPath(const wxTreeItemId& node)
{
	if (node == this->tree->GetRootItem())
	{
		return "/";
	}

	wxTreeItemId child = node;
	wxString path;

	while (child != this->tree->GetRootItem())
	{
		path = "/" + this->tree->GetItemText(child) + path;
		child = this->tree->GetItemParent(child);
	}

	return path;
}

