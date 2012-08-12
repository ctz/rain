#include "GUI-AboutDialog.h"
#include "Versions.h"

#include "rain-about.png.h"

#include <wx/mstream.h>

using namespace RAIN;

BEGIN_EVENT_TABLE(RAIN::GUI::AboutDialog, wxDialog)
	EVT_BUTTON(wxOK, RAIN::GUI::AboutDialog::OnOK)
END_EVENT_TABLE()

BEGIN_EVENT_TABLE(RAIN::GUI::StaticImage, wxWindow)
	EVT_PAINT(RAIN::GUI::StaticImage::OnPaint)
END_EVENT_TABLE()

GUI::AboutDialog::AboutDialog(wxWindow* parent):
	wxDialog(parent, -1, "About RAIN " __PRODUCTVERSION, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER|wxTHICK_FRAME)
{
	// begin wxGlade: AboutDialog::AboutDialog
	wxMemoryInputStream ins(rain_about_png, rain_about_png_len);
	RAINBitmap = new StaticImage(this, wxImage(ins));
	VersionLabel = new wxStaticText(this, -1, wxT("Version:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	VersionText = new wxStaticText(this, -1, wxT(__PRODUCTVERSION " build " __PRODUCTBUILD));
	
	DateLabel = new wxStaticText(this, -1, wxT("Build Date:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	DateText = new wxStaticText(this, -1, __PRODUCTDATE);

	AuthorLabel = new wxStaticText(this, -1, wxT("Copyright:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	AuthorText = new wxStaticText(this, -1, wxT("©2004/2005 Joseph Birr-Pixton"));

	OSLabel = new wxStaticText(this, -1, wxT("OS:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	OSText = new wxStaticText(this, -1, ::wxGetOsDescription());

	OpenSSLBitmap = new wxStaticBitmap(this, -1, wxBitmap("IDB_OSSLABOUT", wxBITMAP_TYPE_BMP_RESOURCE));
	WXBitmap = new wxStaticBitmap(this, -1, wxBitmap("IDB_WXABOUT", wxBITMAP_TYPE_BMP_RESOURCE));
	OKButton = new wxButton(this, wxOK, wxT("OK"));

	set_properties();
	do_layout();
	// end wxGlade
}

void GUI::AboutDialog::set_properties()
{
	// begin wxGlade: AboutDialog::set_properties
	// end wxGlade
}

void GUI::AboutDialog::do_layout()
{
	// begin wxGlade: AboutDialog::do_layout
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* ToolkitSizer = new wxBoxSizer(wxHORIZONTAL);
	wxFlexGridSizer* AboutSizer = new wxFlexGridSizer(3, 2, 0, 0);
	AboutSizer->AddGrowableCol(0, 1);
	AboutSizer->AddGrowableCol(1, 2);
	MainSizer->Add(RAINBitmap, 0, wxALL|wxFIXED_MINSIZE, 10);
	AboutSizer->Add(VersionLabel, 0, wxALL|wxALIGN_RIGHT|wxADJUST_MINSIZE, 5);
	AboutSizer->Add(VersionText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxEXPAND|wxADJUST_MINSIZE, 5);
	AboutSizer->Add(DateLabel, 0, wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 5);
	AboutSizer->Add(DateText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxEXPAND|wxADJUST_MINSIZE, 5);
	AboutSizer->Add(AuthorLabel, 0, wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 5);
	AboutSizer->Add(AuthorText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxEXPAND|wxADJUST_MINSIZE, 5);
	AboutSizer->Add(OSLabel, 0, wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 5);
	AboutSizer->Add(OSText, 0, wxRIGHT|wxTOP|wxBOTTOM|wxEXPAND|wxADJUST_MINSIZE, 5);
	MainSizer->Add(AboutSizer, 0, wxEXPAND, 0);
	ToolkitSizer->Add(OpenSSLBitmap, 0, wxLEFT|wxRIGHT|wxTOP|wxALIGN_RIGHT|wxADJUST_MINSIZE, 10);
	ToolkitSizer->Add(WXBitmap, 0, wxLEFT|wxRIGHT|wxTOP|wxALIGN_RIGHT|wxADJUST_MINSIZE, 10);
	MainSizer->Add(ToolkitSizer, 1, wxEXPAND, 0);
	MainSizer->Add(OKButton, 0, wxALL|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	Layout();
	// end wxGlade
}

void GUI::AboutDialog::OnOK(wxCommandEvent &e)
{
	this->EndModal(true);
}
