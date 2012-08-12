#include "GUI-Preferences.h"
#include "GUI-Helpers.h"

#include "MessageCore.h"
#include "RainSandboxApp.h"
#include "PieceManager.h"
#include "CredentialManager.h"

#include <wx/wx.h>

using namespace RAIN;
using namespace RAIN::GUI;

extern RAIN::MessageCore *globalMessageCore;

BEGIN_EVENT_TABLE(RAIN::GUI::Preferences, wxDialog)
	EVT_BUTTON(ID_PR_OK, RAIN::GUI::Preferences::OnOK)
	EVT_BUTTON(ID_PR_CANCEL, RAIN::GUI::Preferences::OnCancel)

	EVT_COMMAND_SCROLL(ID_PR_DISKSLIDER, RAIN::GUI::Preferences::OnSlide)

	EVT_BUTTON(ID_PR_CREDBROWSE, RAIN::GUI::Preferences::OnBrowse)
	EVT_BUTTON(ID_PR_MANIBROWSE, RAIN::GUI::Preferences::OnBrowse)
	EVT_BUTTON(ID_PR_PIECEBROWSE, RAIN::GUI::Preferences::OnBrowse)
	EVT_BUTTON(ID_PR_OUTPUTBROWSE, RAIN::GUI::Preferences::OnBrowse)

	EVT_BUTTON(ID_PR_CREDOPEN, RAIN::GUI::Preferences::OnOpen)
	EVT_BUTTON(ID_PR_MANIOPEN, RAIN::GUI::Preferences::OnOpen)
	EVT_BUTTON(ID_PR_PIECEOPEN, RAIN::GUI::Preferences::OnOpen)
	EVT_BUTTON(ID_PR_OUTPUTOPEN, RAIN::GUI::Preferences::OnOpen)
END_EVENT_TABLE()

Preferences::Preferences(wxWindow *parent)
: wxDialog(parent, -1, wxT("RAIN Preferences"), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxTHICK_FRAME)
{
	LocationsSizer_staticbox = new wxStaticBox(this, -1, wxT(" File Locations "));
	OptionsSizer_staticbox = new wxStaticBox(this, -1, wxT(" Options "));
	DiskSizer_staticbox = new wxStaticBox(this, -1, wxT(" Piece Storage "));
	DiskLabel = new wxStaticText(this, -1, wxT("Disk Usage Limit:"));
	DiskLabel2 = new wxStaticText(this, -1, wxT("Currently used: lol"));
	DiskSlider = new wxSlider(this, ID_PR_DISKSLIDER, 0, 0, 10, wxDefaultPosition, wxDefaultSize, wxSL_HORIZONTAL|wxSL_TOP|wxSL_AUTOTICKS);

	CredLabel = new wxStaticText(this, -1, wxT("Credentials:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	CredText = new wxTextCtrl(this, -1, wxT(""));
	CredBrowse = new wxButton(this, ID_PR_CREDBROWSE, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	CredOpen = new wxButton(this, ID_PR_CREDOPEN, wxT("Open"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

	ManiLabel = new wxStaticText(this, -1, wxT("Manifest Cache:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	ManiText = new wxTextCtrl(this, -1, wxT(""));
	ManiBrowse = new wxButton(this, ID_PR_MANIBROWSE, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	ManiOpen = new wxButton(this, ID_PR_MANIOPEN, wxT("Open"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

	PieceLabel = new wxStaticText(this, -1, wxT("Piece Storage:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	PieceText = new wxTextCtrl(this, -1, wxT(""));
	PieceBrowse = new wxButton(this, ID_PR_PIECEBROWSE, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	PieceOpen = new wxButton(this, ID_PR_PIECEOPEN, wxT("Open"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

	OutputLabel = new wxStaticText(this, -1, wxT("Output Directory:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	OutputText = new wxTextCtrl(this, -1, wxT(""));
	OutputBrowse = new wxButton(this, ID_PR_OUTPUTBROWSE, wxT("..."), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);
	OutputOpen = new wxButton(this, ID_PR_OUTPUTOPEN, wxT("Open"), wxDefaultPosition, wxDefaultSize, wxBU_EXACTFIT);

	PieceQueueCheck = new wxCheckBox(this, ID_PR_PC_WAIT, wxT("Allow outgoing piece queue to run with less than 5 peers"));
	ConnCacheCheck = new wxCheckBox(this, ID_PR_CONNCACHE, wxT("Cache connections across sessions"));
	SmallIconsCheck = new wxCheckBox(this, ID_PR_SMALLICON, wxT("Use small icons in file browser"));

	ButtonSpacer = new wxPanel(this, -1);
	CancelButton = new wxButton(this, ID_PR_CANCEL, wxT("&Cancel"));
	OkButton = new wxButton(this, ID_PR_OK, wxT("&OK"));

	set_properties();
	do_layout();

	/* we import the settings, then update the slider, then import
	 * the settings once again to set the slider value. then
	 * we fake a slider event to have the label updated */
	ImportSettings();
	UpdateCacheSlider();
	ImportSettings();
	this->OnSlide(wxScrollEvent());
	UpdateCurrentlyUsed();
}

void Preferences::set_properties()
{
	CredLabel->SetSize(wxSize(100, -1));
	ManiLabel->SetSize(wxSize(100, -1));
	PieceLabel->SetSize(wxSize(100, -1));
	OutputLabel->SetSize(wxSize(100, -1));
	OkButton->SetDefault();
}

void Preferences::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ButtonSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* OptionsSizer = new wxStaticBoxSizer(OptionsSizer_staticbox, wxVERTICAL);
	wxStaticBoxSizer* LocationsSizer = new wxStaticBoxSizer(LocationsSizer_staticbox, wxVERTICAL);
	wxBoxSizer* OutputSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* PieceSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* ManiSizer = new wxBoxSizer(wxHORIZONTAL);
	wxBoxSizer* CredSizer = new wxBoxSizer(wxHORIZONTAL);
	wxStaticBoxSizer* DiskSizer = new wxStaticBoxSizer(DiskSizer_staticbox, wxVERTICAL);
	DiskSizer->Add(DiskLabel, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5);
	DiskSizer->Add(DiskSlider, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5);
	DiskSizer->Add(DiskLabel2, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 5);
	MainSizer->Add(DiskSizer, 0, wxALL|wxEXPAND, 10);
	CredSizer->Add(CredLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	CredSizer->Add(CredText, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	CredSizer->Add(CredBrowse, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
	CredSizer->Add(CredOpen, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);
	LocationsSizer->Add(CredSizer, 1, wxEXPAND, 0);
	ManiSizer->Add(ManiLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	ManiSizer->Add(ManiText, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	ManiSizer->Add(ManiBrowse, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
	ManiSizer->Add(ManiOpen, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);
	LocationsSizer->Add(ManiSizer, 1, wxEXPAND, 0);
	PieceSizer->Add(PieceLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	PieceSizer->Add(PieceText, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	PieceSizer->Add(PieceBrowse, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
	PieceSizer->Add(PieceOpen, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);
	LocationsSizer->Add(PieceSizer, 1, wxEXPAND, 0);
	OutputSizer->Add(OutputLabel, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	OutputSizer->Add(OutputText, 1, wxALL|wxALIGN_CENTER_VERTICAL|wxFIXED_MINSIZE, 5);
	OutputSizer->Add(OutputBrowse, 0, wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 0);
	OutputSizer->Add(OutputOpen, 0, wxALL|wxALIGN_CENTER_VERTICAL|wxADJUST_MINSIZE, 5);
	LocationsSizer->Add(OutputSizer, 1, wxEXPAND, 0);
	MainSizer->Add(LocationsSizer, 1, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 10);
	OptionsSizer->Add(PieceQueueCheck, 0, wxALL|wxFIXED_MINSIZE, 5);
	OptionsSizer->Add(ConnCacheCheck, 0, wxALL|wxFIXED_MINSIZE, 5);
	OptionsSizer->Add(SmallIconsCheck, 0, wxALL|wxFIXED_MINSIZE, 5);
	MainSizer->Add(OptionsSizer, 0, wxLEFT|wxRIGHT|wxEXPAND, 10);
	ButtonSizer->Add(ButtonSpacer, 1, wxEXPAND, 0);
	ButtonSizer->Add(OkButton, 0, wxLEFT|wxTOP|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	ButtonSizer->Add(CancelButton, 0, wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	MainSizer->Add(ButtonSizer, 0, wxEXPAND|wxALIGN_RIGHT, 0);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	Layout();
}

void Preferences::ImportSettings()
{
	wxConfig *cfg = (wxConfig *) wxConfig::Get();

	this->CredText->SetValue(CredentialManager::GetCredentialFilename(CREDENTIAL_BASE));
	this->PieceText->SetValue(CredentialManager::GetCredentialFilename(PIECE_DIR));
	this->OutputText->SetValue(CredentialManager::GetCredentialFilename(OUTPUT_DIR));
	this->ManiText->SetValue(CredentialManager::GetCredentialFilename(MANIFEST_DIR));

	long cachelim = cfg->Read("/PieceCacheLimit", (long) 1024);
	wxLogVerbose("Preferences::ImportSettings() set cachelimit to %dMB", cachelim);
	this->DiskSlider->SetValue(cachelim / 100);

	this->ConnCacheCheck->SetValue(cfg->Read("/UseConnectionCache", (long) 1) == 1);
	this->SmallIconsCheck->SetValue(cfg->Read("/SmallIcons", (long) 0) == 1);
	this->PieceQueueCheck->SetValue(cfg->Read("/RunQueueWithFewPeers", (long) 0) == 1);
}

void Preferences::ExportSettings()
{
	wxConfig *cfg = (wxConfig *) wxConfig::Get();

	wxLogVerbose("Preferences::ExportSettings() set cachelimit to %dMB", this->DiskSlider->GetValue() * 100);
	cfg->Write("/PieceCacheLimit", (long) this->DiskSlider->GetValue() * 100);

	cfg->Write("/Credentials", this->CredText->GetValue());
	cfg->Write("/ManifestCache", this->ManiText->GetValue());
	cfg->Write("/PieceCache", this->PieceText->GetValue());
	cfg->Write("/Output", this->OutputText->GetValue());

	cfg->Write("/UseConnectionCache", (long) this->ConnCacheCheck->GetValue());
	cfg->Write("/SmallIcons", (long) this->SmallIconsCheck->GetValue());
	cfg->Write("/RunQueueWithFewPeers", (long) this->PieceQueueCheck->GetValue());
}

void Preferences::UpdateCacheSlider()
{
	wxLongLong total, free, used;

	if (::wxGetDiskSpace(this->PieceText->GetValue(), &total, &free))
	{
		this->DiskSlider->Enable(true);
		used = total - free;

		used = used / (1024 * 1024);
		total = total / (1024 * 1024);
		free = free / (1024 * 1024);

        this->DiskSlider->SetRange(0, free.ToLong() / 100);
		currVolTotal = total.ToLong();
		currVolFree = free.ToLong();

		this->OnSlide(wxScrollEvent());
	} else {
		this->DiskSlider->Enable(false);
	}
}

void Preferences::UpdateCurrentlyUsed()
{
	unsigned long long used = globalMessageCore->app->pieces->GetCurrentUsage();
	DiskLabel2->SetLabel(wxString::Format(wxT("Currently used: %.02fMB"), used / (float) (1024 * 1024)));
}

void Preferences::OnOK(wxCommandEvent &e)
{
	ExportSettings();
	this->EndModal(true);
}

void Preferences::OnCancel(wxCommandEvent &e)
{
	this->EndModal(false);
}

void Preferences::OnSlide(wxScrollEvent &e)
{
	this->DiskLabel->SetLabel(wxString::Format(wxT("Disk Usage Limit: %.02fGB of %.02fGB free on this %.02fGB volume"), (float) (this->DiskSlider->GetValue() * 100) / (float)1024, currVolFree / (float)1024, currVolTotal / (float)1024));
}

void Preferences::OnBrowse(wxCommandEvent &e)
{
	wxTextCtrl *text = NULL;
	wxString help;

	switch (e.GetId())
	{
	case ID_PR_CREDBROWSE:
		help = wxT("Select the directory where your credentials (certificates and private key) are/are to be kept.");
		text = this->CredText;
		break;
	case ID_PR_MANIBROWSE:
		help = wxT("Select the directory where you wish to locally cache manifests.");
		text = this->ManiText;
		break;
	case ID_PR_PIECEBROWSE:
		help = wxT("Select the directory where you wish to store pieces.");
		text = this->PieceText;
		break;
	case ID_PR_OUTPUTBROWSE:
		help = wxT("Select the directory where you wish to downloaded files.");
		text = this->OutputText;
		break;
	}

	wxString newdir = ::wxDirSelector(help, text->GetValue(), wxDD_NEW_DIR_BUTTON);

	if (!newdir.IsEmpty())
	{
		text->SetValue(newdir);

		if (text == this->PieceText)
			UpdateCacheSlider();
	}
}

void Preferences::OnOpen(wxCommandEvent &e)
{
	wxString dir;

	switch (e.GetId())
	{
	case ID_PR_CREDOPEN:
		dir = this->CredText->GetValue();
		break;
	case ID_PR_MANIOPEN:
		dir = this->ManiText->GetValue();
		break;
	case ID_PR_PIECEOPEN:
		dir = this->PieceText->GetValue();
		break;
	case ID_PR_OUTPUTOPEN:
		dir = this->OutputText->GetValue();
		break;
	}

	if (!dir.IsEmpty())
	{
		GUI::OpenInShell(dir);
	}
}
