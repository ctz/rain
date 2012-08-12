#include "GUI-CertificateWizard.h"

#include <wx/utils.h>

#include "_wxconfig.h"

#include <openssl/asn1.h>
#include <openssl/pem.h>
#include <openssl/conf.h>
#include <openssl/rsa.h>
#include <openssl/x509v3.h>

#include "CredentialManager.h"

using namespace RAIN;
using namespace RAIN::GUI;

#define DEFAULT_CLIENT_TIMESPAN 31556926

/*
 * Wizard page for selection of a peer or CA certificate type
 */
CertificateCreateType::CertificateCreateType(CertificateWizard *wizard)
: wxWizardPage()
{
	this->wizard = wizard;
	this->Create(this->wizard, wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE));
	
	this->TitleLabel = new wxStaticText(this, -1, wxT("Certificate Type"));
	this->TypeHelp = new wxStaticText(this, -1, wxT("A RAIN certificate can be created for one of two reasons:\n\n    a) To be the root certificate for a new network, or\n    b) To allow you to connect to an existing network.\n"));
	
	const wxString TypeChoice_choices[] = {
		wxT("Join an existing RAIN network"),
		wxT("Create a new RAIN network"),
		wxT("Sign a client certificate")
	};

	this->TypeChoice = new wxRadioBox(this, -1, wxT(" Certificate Use "), wxDefaultPosition, wxDefaultSize, 3, TypeChoice_choices, 1, wxRA_SPECIFY_COLS);

	this->set_properties();
	this->do_layout();
}

bool CertificateCreateType::TransferDataFromWindow()
{
	switch (this->TypeChoice->GetSelection())
	{
	case 0:		this->wizard->mode = CERT_CREATECLIENT;
				this->wizard->key->UpdateDynamics();
				break;
	case 1:		this->wizard->mode = CERT_CREATECA;
				this->wizard->key->UpdateDynamics();
				break;
	case 2:		this->wizard->mode = CERT_SIGNCLIENT;
				this->wizard->key->UpdateDynamics();
				this->wizard->worker->SetPrev(this->wizard->signconfirm);
				break;
	default:	wxASSERT(0);
	}
	
	return true;
}

/* wxWizardPage overrides */
wxWizardPage * CertificateCreateType::GetPrev() const
{
	wizard->SetSize(600, 0);
	return NULL;
}

wxWizardPage * CertificateCreateType::GetNext() const
{
	switch (this->TypeChoice->GetSelection())
	{
	case 0:		//vvv
	case 1:		return this->wizard->subject;	
	case 2:		return this->wizard->signsel;
	default:	return NULL;
	}
}

void CertificateCreateType::set_properties()
{
	this->TitleLabel->SetFont(wxFont(14, wxMODERN, wxNORMAL, wxNORMAL, 0, "Trebuchet MS"));
	this->TypeHelp->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->TypeChoice->SetSelection(0);
}

void CertificateCreateType::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(TitleLabel, 0, wxALL, 10);
	MainSizer->Add(TypeHelp, 0, wxLEFT|wxRIGHT|wxEXPAND, 10);
	MainSizer->Add(TypeChoice, 1, wxALL|wxEXPAND, 10);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
}

/*
 * Wizard page for selection/input of crl
 */
CertificateSignSelect::CertificateSignSelect(CertificateWizard *wizard)
: wxWizardPageSimple()
{
	this->wizard = wizard;
	this->Create(this->wizard, NULL, NULL, wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE));

	this->TitleLabel = new wxStaticText(this, -1, wxT("Certificate Signing"));

	this->CertReqLocationLabel = new wxStaticText(this, -1, wxT("Load Certificate Request:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->CertReqLocation = new wxTextCtrl(this, -1, wxT(""));
	this->CertReqLocationChooser = new wxButton(this, -1, wxT("..."));

	this->KeySpacer1 = new wxPanel(this, -1);
	this->OrLabel = new wxStaticText(this, -1, wxT("or"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->KeySpacer2 = new wxPanel(this, -1);

	this->PEMPasteLabel = new wxStaticText(this, -1, wxT("PEM-encoded Certificate Request:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->PEMPasteText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
	this->KeySpacer3 = new wxPanel(this, -1);

	this->set_properties();
	this->do_layout();
}

void CertificateSignSelect::set_properties()
{
	this->TitleLabel->SetFont(wxFont(14, wxMODERN, wxNORMAL, wxNORMAL, 0, "Trebuchet MS"));
	this->CertReqLocationChooser->SetSize(wxSize(25, -1));
}

void CertificateSignSelect::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* FieldSizer = new wxFlexGridSizer(3, 3, 5, 5);

	MainSizer->Add(this->TitleLabel, 0, wxALL, 10);

	FieldSizer->Add(this->CertReqLocationLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer->Add(this->CertReqLocation, 2, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer->Add(this->CertReqLocationChooser, 0, wxRIGHT|wxBOTTOM, 5);

	FieldSizer->Add(this->KeySpacer1, 1, wxEXPAND, 0);
	FieldSizer->Add(this->OrLabel, 2, wxBOTTOM|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer->Add(this->KeySpacer2, 1, wxEXPAND, 0);

	FieldSizer->Add(this->PEMPasteLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer->Add(this->PEMPasteText, 2, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer->Add(this->KeySpacer3, 1, wxEXPAND, 0);

	FieldSizer->AddGrowableCol(1);
	MainSizer->Add(FieldSizer, 1, wxALL|wxEXPAND, 10);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
}

bool CertificateSignSelect::TransferDataFromWindow()
{
	if ((this->PEMPasteText->GetValue().Length() == 0 && this->CertReqLocation->GetValue().Length() == 0) ||
		(this->PEMPasteText->GetValue().Length() != 0 && this->CertReqLocation->GetValue().Length() != 0))
	{
		wxMessageBox("You must enter either a filename or a PEM-encoded certificate request.", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	}

	this->wizard->crqfilename = this->CertReqLocation->GetValue();
	this->wizard->crqstring = this->PEMPasteText->GetValue();

	return this->wizard->signconfirm->UpdateDynamics();
}

/*
 * Wizard page for crl details confirmation
 */
CertificateSignConfirm::CertificateSignConfirm(CertificateWizard *wizard)
: wxWizardPageSimple()
{
	this->wizard = wizard;
	this->Create(this->wizard, NULL, NULL, wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE));

	this->TitleLabel = new wxStaticText(this, -1, wxT("Sign this Certificate?"));

	this->NameLabel = new wxStaticText(this, -1, wxT("Name:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->NameText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	
	this->OrganisationLabel = new wxStaticText(this, -1, wxT("Organisation:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->OrganisationText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	
	this->OrganisationalUnitLabel = new wxStaticText(this, -1, wxT("Department:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->OrganisationalUnitText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	
	this->EmailLabel = new wxStaticText(this, -1, wxT("Email Address:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->EmailText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	
	this->LocalityLabel = new wxStaticText(this, -1, wxT("Locality:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->LocalityText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	
	this->StateProvinceLabel = new wxStaticText(this, -1, wxT("State/Province:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->StateProvinceText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	
	this->CountryLabel = new wxStaticText(this, -1, wxT("Country:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->CountryText = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_READONLY);

	this->ConfirmSpacer = new wxPanel(this, -1);
	this->ConfirmBox = new wxCheckBox(this, -1, wxT("Really sign this certificate?"));

	this->set_properties();
	this->do_layout();
}

void CertificateSignConfirm::set_properties()
{
	this->TitleLabel->SetFont(wxFont(14, wxMODERN, wxNORMAL, wxNORMAL, 0, "Trebuchet MS"));

	this->NameText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->OrganisationText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->OrganisationalUnitText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->EmailText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->LocalityText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->StateProvinceText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->CountryText->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
}

void CertificateSignConfirm::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* FieldsSizer = new wxFlexGridSizer(7, 2, 5, 5);
	MainSizer->Add(this->TitleLabel, 0, wxALL, 10);
	FieldsSizer->Add(this->NameLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->NameText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->OrganisationLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->OrganisationText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->OrganisationalUnitLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->OrganisationalUnitText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->EmailLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->EmailText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->LocalityLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->LocalityText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->StateProvinceLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->StateProvinceText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->CountryLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->CountryText, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->ConfirmSpacer, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->ConfirmBox, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->AddGrowableCol(1);
	MainSizer->Add(FieldsSizer, 1, wxALL|wxEXPAND, 10);
	this->SetAutoLayout(true);
	this->SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
}

bool CertificateSignConfirm::UpdateDynamics()
{
	X509_REQ *req = NULL;

	if (this->wizard->crqstring.Length() > 0)
	{
		wxString crq = wxString(this->wizard->crqstring);
		const char *crqstr = crq.GetData();

		BIO *bs = BIO_new_mem_buf((void*)crqstr, crq.Length());
        req = PEM_read_bio_X509_REQ(bs, NULL, NULL, NULL);

		if (req == NULL)
		{
			wxMessageBox("PEM decoding error", "Error", wxOK|wxICON_ERROR, this->wizard);
			return false;
		}
	} else {
		BIO *bf = BIO_new_file(this->wizard->crqfilename.c_str(), "rb");

		if (!bf)
		{
			wxMessageBox(wxString::Format("Error opening '%s'", this->wizard->crqfilename.c_str()), "Error", wxOK|wxICON_ERROR, this->wizard);
			return false;
		}

		req = PEM_read_bio_X509_REQ(bf, NULL, NULL, NULL);
		
		if (req == NULL)
		{
			wxMessageBox(wxString::Format("Error decoding PEM data in '%s'", this->wizard->crqfilename.c_str()), "Error", wxOK|wxICON_ERROR, this->wizard);
			return false;
		}
	}

	/* req is now a good x509_req object, but we need to verify the signing */
	EVP_PKEY *pkey = X509_REQ_get_pubkey(req);

	if (!pkey)
	{
		wxMessageBox("Error extracting public key from certificate request.", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	}

	if (X509_REQ_verify(req, pkey) != 1)
	{
		wxMessageBox("Certificate request is invalid, corrupt, or has been tampered with.", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	}

	/* req is now a *valid* x509_req object
	 * so now we fill out values */
	X509_NAME		*name = X509_REQ_get_subject_name(req);
	X509_NAME_ENTRY	*nval = NULL;

	for (int n = 0; n < sk_X509_NAME_ENTRY_num(name->entries); n++)
	{
		nval = sk_X509_NAME_ENTRY_value(name->entries, n);

		switch (OBJ_obj2nid(nval->object))
		{
			/* individual */
		case NID_commonName:
			this->NameText->SetValue(wxString(nval->value->data));
			break;
		case NID_organizationName:
			this->OrganisationText->SetValue(wxString(nval->value->data));
			break;
		case NID_organizationalUnitName:
			this->OrganisationalUnitText->SetValue(wxString(nval->value->data));
			break;
			
			/* location */
		case NID_localityName:
			this->LocalityText->SetValue(wxString(nval->value->data));
			break;
		case NID_stateOrProvinceName:
			this->StateProvinceText->SetValue(wxString(nval->value->data));
			break;
		case NID_countryName:
			this->CountryText->SetValue(wxString(nval->value->data));
			break;
		}
	}

#include "iso3166_sn.h"
#include "iso3166_ln.h"

	wxString cn = this->CountryText->GetValue();
	// convert iso3166 sn to ln
	for (int i = 0; i < countries_ln_c; i++)
	{
		if (cn == countries_sn[i])
		{
			this->CountryText->SetValue(countries_ln[i]);
			break;
		}
	}

	// get email from x509v3 extension
	STACK_OF(X509_EXTENSION)	*req_exts;
	X509_EXTENSION				*subjAltName;
	X509V3_EXT_METHOD			*method;
	void						*emailraw;
	wxString					 email;

	req_exts = X509_REQ_get_extensions(req);
	if (!req_exts)
	{
		email = "no x509v3 extensions containing email address";
		this->EmailText->SetValue(email);
		this->EmailText->Enable(false);
		return true;
	}

	subjAltName = X509v3_get_ext(req_exts, X509v3_get_ext_by_NID(req_exts, OBJ_sn2nid("subjectAltName"), -1));
	method = X509V3_EXT_get(subjAltName);

	if (method->it)
		emailraw = ASN1_item_d2i(NULL, &subjAltName->value->data, subjAltName->value->length, ASN1_ITEM_ptr(method->it));
	else
		emailraw = method->d2i(NULL, &subjAltName->value->data, subjAltName->value->length);

	if (method->i2s)
	{
		// extension is a single string
		email = wxString(method->i2s(method, emailraw));
	} else if (method->i2v) {
		// extension is a hash
		STACK_OF(CONF_VALUE) *nval = NULL;
		nval = method->i2v(method, emailraw, NULL);

		for (int i = 0; i < sk_CONF_VALUE_num(nval); i++)
		{
			CONF_VALUE *v = sk_CONF_VALUE_value(nval, i);
			wxString key(v->name), val(v->value);

			if (key == "email")
			{
				email = val;
				break;
			}
		}
	}

	this->EmailText->SetValue(email);

	this->wizard->req = req;

	return true;
}

bool CertificateSignConfirm::TransferDataFromWindow()
{
	if (this->ConfirmBox->IsChecked())
	{
		this->wizard->pkeypass = RAIN::CredentialManager::GetPrivateKeyPassword();
		this->wizard->worker->GetThread()->Run();
		return true;
	} else {
		return false;
	}
}

/*
 * Wizard page for input of subject info
 */
CertificateSubject::CertificateSubject(CertificateWizard *wizard)
: wxWizardPageSimple()
{
	this->wizard = wizard;
	this->Create(this->wizard, NULL, NULL, wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE));

	this->TitleLabel = new wxStaticText(this, -1, wxT("Certificate Subject"));
	this->SubjectHelp = new wxStaticText(this, -1, wxT("Enter the subject data for this certificate."));
	
	this->NameLabel = new wxStaticText(this, -1, wxT("Name:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->NameText = new wxTextCtrl(this, -1, wxT(""));
	
	this->OrganisationLabel = new wxStaticText(this, -1, wxT("Organisation:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->OrganisationText = new wxTextCtrl(this, -1, wxT(""));
	
	this->OrganisationalUnitLabel = new wxStaticText(this, -1, wxT("Department:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->OrganisationalUnitText = new wxTextCtrl(this, -1, wxT(""));
	
	this->EmailLabel = new wxStaticText(this, -1, wxT("Email Address:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->EmailText = new wxTextCtrl(this, -1, wxT(""));
	
	this->LocalityLabel = new wxStaticText(this, -1, wxT("Locality:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->LocalityText = new wxTextCtrl(this, -1, wxT(""));
	
	this->StateProvinceLabel = new wxStaticText(this, -1, wxT("State/Province:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->StateProvinceText = new wxTextCtrl(this, -1, wxT(""));
	
	this->CountryLabel = new wxStaticText(this, -1, wxT("Country:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	
#include "iso3166_ln.h"
	
	this->CountryChooser = new wxComboBox(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, countries_ln_c, countries_ln, wxCB_DROPDOWN|wxCB_READONLY);

	this->set_properties();
	this->do_layout();
}

bool CertificateSubject::TransferDataFromWindow()
{
	if (this->NameText->GetValue().Length() == 0)
	{
		wxMessageBox("You must enter your name", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	} else {
		this->wizard->name = this->NameText->GetValue();
	}

	if (this->EmailText->GetValue().Length() == 0 || !this->EmailText->GetValue().Contains("@"))
	{
		wxMessageBox("You must enter your email address", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	} else {
		this->wizard->email = this->EmailText->GetValue();
	}

	this->wizard->org = this->OrganisationText->GetValue();
	this->wizard->orgunit = this->OrganisationalUnitText->GetValue();
	this->wizard->locality = this->LocalityText->GetValue();
	this->wizard->state = this->StateProvinceText->GetValue();

#include "iso3166_sn.h"

	this->wizard->country = countries_sn[this->CountryChooser->GetSelection()];

	return true;
}

void CertificateSubject::set_properties()
{
	this->TitleLabel->SetFont(wxFont(14, wxMODERN, wxNORMAL, wxNORMAL, 0, "Trebuchet MS"));
	this->SubjectHelp->SetSize(wxSize(-1, 21));
	this->SubjectHelp->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->CountryChooser->SetSelection(0);
}

void CertificateSubject::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* FieldsSizer = new wxFlexGridSizer(7, 2, 5, 5);
	MainSizer->Add(this->TitleLabel, 0, wxALL, 10);
	MainSizer->Add(this->SubjectHelp, 0, wxLEFT|wxRIGHT|wxEXPAND, 10);
	FieldsSizer->Add(this->NameLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->NameText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->OrganisationLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->OrganisationText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->OrganisationalUnitLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->OrganisationalUnitText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->EmailLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->EmailText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->LocalityLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->LocalityText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->StateProvinceLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->StateProvinceText, 3, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->Add(this->CountryLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldsSizer->Add(this->CountryChooser, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldsSizer->AddGrowableCol(1);
	MainSizer->Add(FieldsSizer, 1, wxALL|wxEXPAND, 10);
	this->SetAutoLayout(true);
	this->SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
}

/*
 * Wizard page for setting certificate info
 */
CertificateKey::CertificateKey(RAIN::GUI::CertificateWizard *wizard)
: wxWizardPageSimple()
{
	this->wizard = wizard;
	this->Create(this->wizard, NULL, NULL, wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE));

	this->TitleLabel = new wxStaticText(this, -1, wxT("Certificate Request Generation"));
	this->KeyHelp1 = new wxStaticText(this, -1, wxT("Select the parameters for your new encryption keys and the location to save your certificate request.\n\n"));
	this->KeySizeLabel = new wxStaticText(this, -1, wxT("Key Length:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	
	const wxString KeySizeChooser_choices[] = {
		wxT("768 bit"),
		wxT("1024 bit"),
		wxT("2048 bit (recommended)"),
		wxT("4096 bit")
	};

	this->KeySizeChooser = new wxComboBox(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, 4, KeySizeChooser_choices, wxCB_DROPDOWN|wxCB_READONLY);
	this->KeySizeChooser->SetValue(KeySizeChooser_choices[2]);
	this->KeySpacer1 = new wxPanel(this, -1);
	
	this->CertReqLocationLabel = new wxStaticText(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->CertReqLocation = new wxTextCtrl(this, -1, wxT(""));
	this->CertReqLocation->Enable(false);
	this->CertReqLocationChooser = new wxButton(this, -1, wxT("..."), wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxBU_EXACTFIT);
	this->CertReqLocationChooser->Enable(false);
	
	this->KeyHelp2 = new wxStaticText(this, -1, wxT("Enter a secure password which will be used to protect your private key, and a safe location to save your private key.\n\n"));
	
	this->PrivateKeyPass1Label = new wxStaticText(this, -1, wxT("Password:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->PrivateKeyPassword1 = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
	this->KeySpacer2 = new wxPanel(this, -1);
	
	this->PrivateKeyPass2Label = new wxStaticText(this, -1, wxT("Again:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->PrivateKeyPassword2 = new wxTextCtrl(this, -1, wxT(""), wxDefaultPosition, wxDefaultSize, wxTE_PASSWORD);
	this->KeySpacer3 = new wxPanel(this, -1);
	
	this->PrivateKeyLocationLabel = new wxStaticText(this, -1, wxT("Save Keys As:"), wxDefaultPosition, wxDefaultSize, wxALIGN_RIGHT);
	this->PrivateKeyLocation = new wxTextCtrl(this, -1, wxT(""));
	this->PrivateKeyLocation->Enable(false);
	this->PrivateKeyLocationChooser = new wxButton(this, -1, wxT("..."), wxDefaultPosition, wxDefaultSize, wxNO_BORDER|wxBU_EXACTFIT);
	this->PrivateKeyLocationChooser->Enable(false);

	this->set_properties();
	this->do_layout();
}
void CertificateKey::UpdateDynamics()
{
	if (this->wizard->mode == CERT_CREATECA)
	{
		this->CertReqLocation->SetValue(RAIN::CredentialManager::GetCredentialFilename(RAIN::CA_CERTIFICATE));
		this->PrivateKeyLocation->SetValue(RAIN::CredentialManager::GetCredentialFilename(RAIN::CA_PRIVATEKEY));

		this->CertReqLocationLabel->SetLabel(wxT("Save Certificate As:"));
	} else if (this->wizard->mode == CERT_CREATECLIENT) {
		this->CertReqLocation->SetValue(RAIN::CredentialManager::GetCredentialFilename(RAIN::CL_CERTREQ));
		this->PrivateKeyLocation->SetValue(RAIN::CredentialManager::GetCredentialFilename(RAIN::CL_PRIVATEKEY));

		this->CertReqLocationLabel->SetLabel(wxT("Save Certificate Request As:"));
	}
}

bool CertificateKey::TransferDataFromWindow()
{
	if (this->CertReqLocation->GetValue().Length() == 0)
	{
		wxMessageBox("You must select a valid location to save your certificate request.", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	} else {
		this->wizard->crlocation = this->CertReqLocation->GetValue();
	}

	if (this->PrivateKeyLocation->GetValue().Length() == 0)
	{
		wxMessageBox("You must select a valid location to save your keys.", "Error", wxOK|wxICON_ERROR, this->wizard);
		return false;
	} else {
		this->wizard->pkeylocation = this->PrivateKeyLocation->GetValue();
	}

	if (this->PrivateKeyPassword1->GetValue() != this->PrivateKeyPassword2->GetValue())
	{
		wxMessageBox("The passwords you entered do not match.", "Error", wxOK|wxICON_ERROR, this->wizard);
		this->PrivateKeyPassword1->SetValue(wxEmptyString);
		this->PrivateKeyPassword2->SetValue(wxEmptyString);
		return false;
	}

	if (this->PrivateKeyPassword1->GetValue().Length() <= 5)
	{
		wxMessageBox("You must enter a password longer than 5 characters.", "Error", wxOK|wxICON_ERROR, this->wizard);
		this->PrivateKeyPassword1->SetValue(wxEmptyString);
		this->PrivateKeyPassword2->SetValue(wxEmptyString);
		return false;
	} else {
		this->wizard->pkeypass = this->PrivateKeyPassword1->GetValue();
	}

	const int keysizes[] = {
		768, 1024, 2048, 4096
	};

	this->wizard->keysize = keysizes[this->KeySizeChooser->GetSelection()];

	this->wizard->worker->GetThread()->Run();
	return true;
}

void CertificateKey::set_properties()
{
	this->TitleLabel->SetFont(wxFont(14, wxMODERN, wxNORMAL, wxNORMAL, 0, "Trebuchet MS"));
	this->KeyHelp1->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->CertReqLocationChooser->SetSize(wxSize(25, 10));
	this->KeyHelp2->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->PrivateKeyLocationChooser->SetSize(wxSize(25, 10));
}

void CertificateKey::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxFlexGridSizer* FieldSizer1 = new wxFlexGridSizer(2, 3, 5, 5);
	wxFlexGridSizer* FieldSizer2 = new wxFlexGridSizer(3, 3, 5, 5);
	FieldSizer1->Add(this->KeySizeLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer1->Add(this->KeySizeChooser, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer1->Add(this->KeySpacer1, 1, wxEXPAND, 0);
	FieldSizer1->Add(this->CertReqLocationLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer1->Add(this->CertReqLocation, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer1->Add(this->CertReqLocationChooser, 0, wxRIGHT|wxBOTTOM, 5);
	FieldSizer1->AddGrowableCol(1);
	FieldSizer2->Add(this->PrivateKeyPass1Label, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer2->Add(this->PrivateKeyPassword1, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer2->Add(this->KeySpacer2, 1, wxEXPAND, 0);
	FieldSizer2->Add(this->PrivateKeyPass2Label, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer2->Add(this->PrivateKeyPassword2, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer2->Add(this->KeySpacer3, 1, wxEXPAND, 0);
	FieldSizer2->Add(this->PrivateKeyLocationLabel, 1, wxALL|wxEXPAND|wxALIGN_CENTER_HORIZONTAL|wxALIGN_CENTER_VERTICAL, 5);
	FieldSizer2->Add(this->PrivateKeyLocation, 1, wxRIGHT|wxBOTTOM|wxEXPAND, 5);
	FieldSizer2->Add(this->PrivateKeyLocationChooser, 0, wxRIGHT|wxBOTTOM, 5);
	FieldSizer2->AddGrowableCol(1);
	MainSizer->Add(this->TitleLabel, 0, wxALL, 10);
	MainSizer->Add(this->KeyHelp1, 0, wxLEFT|wxRIGHT|wxEXPAND, 10);
	MainSizer->Add(FieldSizer1, 0, wxALL|wxEXPAND, 10);
	MainSizer->Add(this->KeyHelp2, 0, wxLEFT|wxRIGHT|wxEXPAND, 10);
	MainSizer->Add(FieldSizer2, 0, wxALL|wxEXPAND, 10);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
}

/*
 * Wizard page for doing the actual work
 */
CertificateWorker::CertificateWorker(RAIN::GUI::CertificateWizard *wizard)
: wxWizardPageSimple(wizard, NULL, NULL, wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE)), wxThreadHelper()
{
	this->wizard = wizard;

	this->TitleLabel = new wxStaticText(this, -1, wxT("Working..."));
	this->WorkGauge = new wxGauge(this, -1, 1000, wxDefaultPosition, wxDefaultSize, wxGA_HORIZONTAL|wxGA_PROGRESSBAR|wxGA_SMOOTH);
	this->DoneLabel = new wxStaticText(this, -1, wxT("Working..."));

	this->done = false;

	this->set_properties();
	this->do_layout();

	((wxThreadHelper *)this)->Create();
}

void CertificateWorker::set_properties()
{
	this->TitleLabel->SetFont(wxFont(14, wxMODERN, wxNORMAL, wxNORMAL, 0, "Trebuchet MS"));
	this->WorkGauge->SetSize(wxSize(-1, 16));
}

void CertificateWorker::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	MainSizer->Add(this->TitleLabel, 0, wxALL, 10);
	MainSizer->Add(this->WorkGauge, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxEXPAND, 10);
	MainSizer->Add(this->DoneLabel, 1, wxLEFT|wxRIGHT|wxBOTTOM, 10);
	SetAutoLayout(true);
	SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
}

bool CertificateWorker::TransferDataFromWindow()
{
	return this->done;
}

void CertificateWorker::RSAGenerateCallback(int a, int b, void *args)
{
	CertificateWorker *that = (CertificateWorker *)args;
	::wxMutexGuiEnter();
	that->WorkGauge->SetValue(that->WorkGauge->GetValue() + 1);
	::wxMutexGuiLeave();
}

void CertificateWorker::WorkUpdate(int value, const wxString& message)
{
	::wxMutexGuiEnter();
	this->WorkGauge->SetValue(value);
	this->DoneLabel->SetLabel(message);
	::wxMutexGuiLeave();
}

void * CertificateWorker::Entry()
{
	if (this->wizard->mode == CERT_SIGNCLIENT)
	{
		X509 *x = NULL;
		X509_NAME *name = NULL;
		X509_REQ *req = this->wizard->req;
		STACK_OF(X509_EXTENSION) *req_exts = NULL;

		if (req == NULL)
			return NULL;

		EVP_PKEY *capk = NULL;
		X509 *ca = NULL;
	
		// new empty cert
		x = X509_new();
		X509_set_version(x, 2L);

		// set serial
		long serial = wxConfigBase::Get()->Read("X509Serial", 0L);
		serial++;
		wxConfigBase::Get()->Write("X509Serial", serial);
		ASN1_INTEGER_set(X509_get_serialNumber(x), serial);

		this->WorkUpdate(0, wxT("Opening CA certificate and private key..."));
		
		// load ca pkey
		wxString pkfile = RAIN::CredentialManager::GetCredentialFilename(RAIN::CA_PRIVATEKEY);
		BIO *pkbio = BIO_new(BIO_s_file());
		BIO_read_filename(pkbio, pkfile.c_str());

		wxLogVerbose("opening pem ca pk from %s: %s", pkfile.c_str(), (pkbio) ? wxT("ok") : wxT("failed!"));

		wxString pkpass = this->wizard->pkeypass;

 		if (pkbio)
			capk = PEM_read_bio_PrivateKey(pkbio, NULL, NULL, (void*) pkpass.c_str());

		if (!pkbio || !capk)
		{
			wxMessageBox("Cannot load CA private key " + pkfile, "Error", wxOK|wxICON_ERROR, this->wizard);
			
			if (pkbio)
				BIO_free(pkbio);

			if (x)
				X509_free(x);

			return NULL;
		} else {
			BIO_free(pkbio);
		}

		// load ca cert
		BIO *cabio = BIO_new_file(RAIN::CredentialManager::GetCredentialFilename(RAIN::CA_CERTIFICATE).c_str(), "rb");

		if (cabio)
			ca = PEM_read_bio_X509(cabio, NULL, NULL, NULL);

		if (!cabio || !ca)
		{
			wxMessageBox("Cannot load CA certificate.", "Error", wxOK|wxICON_ERROR, this->wizard);
			
			if (cabio)
				BIO_free(cabio);

			if (x)
				X509_free(x);

			return NULL;
		} else {
			BIO_free(cabio);
		}

		this->WorkUpdate(500, wxT("Creating new x509 certificate..."));

		const int ext_c = 4;
		int   ext_key[] = {NID_basic_constraints, NID_subject_key_identifier, NID_authority_key_identifier, NID_key_usage};
		char *ext_val[] = {"CA:FALSE", "hash", "keyid,issuer:always", "nonrepudiation,digitalSignature,keyEncipherment"};

		// transfer credentials from certreq to cert
		name = X509_REQ_get_subject_name(req);
		wxASSERT(name);
		X509_set_subject_name(x, name);

		name = X509_get_subject_name(ca);
		wxASSERT(name);
		X509_set_issuer_name(x, name);

		X509_set_pubkey(x, X509_REQ_get_pubkey(req));
		X509_gmtime_adj(X509_get_notBefore(x), 0);
		X509_gmtime_adj(X509_get_notAfter(x), DEFAULT_CLIENT_TIMESPAN);

		// add extensions
		//TODO: fix this
		/*X509V3_CTX ctx;
		X509V3_set_ctx(&ctx, ca, x, NULL, NULL, 0);
		for (int i = 0; i < ext_c; i++)
		{
			X509_EXTENSION *ext;

			ext = X509V3_EXT_conf_nid(NULL, NULL, ext_key[i], ext_val[i]);
			X509_add_ext(x, ext, -1);
			X509_EXTENSION_free(ext);
		}*/

		// copy subjectAltName
		req_exts = X509_REQ_get_extensions(req);
		int subjAltName_pos = X509v3_get_ext_by_NID(req_exts, NID_subject_alt_name, -1);
		X509_add_ext(x, X509v3_get_ext(req_exts, subjAltName_pos), -1);

		this->WorkUpdate(750, wxT("Signing new x509 certificate..."));

		// sign new cert
		if (!X509_sign(x, capk, EVP_sha1()))
		{
			this->WorkUpdate(1000, wxT("Error signing certificate"));
		}

		this->WorkUpdate(900, wxT("Saving new x509 certificate..."));
		
		// save
		BIO *obio = BIO_new_file(RAIN::CredentialManager::GetCredentialFilename(RAIN::CL_CERTIFICATE).c_str(), "w");
		if (!obio)
		{
			wxMessageBox("Cannot save certificate.", "Error", wxOK|wxICON_ERROR, this->wizard);
			
			// tidy
			if (x)
				X509_free(x);

			if (capk)
				EVP_PKEY_free(capk);

			if (req)
				X509_REQ_free(req);

			return NULL;
		}

		PEM_write_bio_X509(obio, x);
		BIO_free(obio);

		if (x)
			X509_free(x);

		if (req)
			X509_REQ_free(req);

		if (capk)
			EVP_PKEY_free(capk);

		this->WorkUpdate(1000, wxT("Finished."));
	} else {
		// making a ca cert and making a req are similar
		bool makingCA = (this->wizard->mode == CERT_CREATECA);

		X509_REQ *x = NULL;
		X509 *xca = NULL;
		EVP_PKEY *pk = NULL;
		RSA *rsa = NULL;
		X509_NAME *name = NULL;
		STACK_OF(X509_EXTENSION) *exts = NULL;
		X509_EXTENSION *ex = NULL;

		pk = EVP_PKEY_new();

		if (makingCA)
			xca = X509_new();
		else
			x = X509_REQ_new();

		this->WorkUpdate(0, wxT("Generating RSA keys..."));
		
		// create rsa keys
		rsa = RSA_generate_key(this->wizard->keysize, RSA_F4, CertificateWorker::RSAGenerateCallback, this);

		// assign rsa keys to a evp_pkey
		EVP_PKEY_assign_RSA(pk, rsa);

		if (makingCA)
			X509_set_pubkey(xca, pk);
		else
			X509_REQ_set_pubkey(x, pk);

		this->WorkUpdate(500, (makingCA) ? wxT("Generating root certificate...") : wxT("Generating certificate request..."));

		// fill x509_req
		if (makingCA)
			name = X509_get_subject_name(xca);
		else
			name = X509_REQ_get_subject_name(x);

		//TODO: check return
		X509_NAME_add_entry_by_txt(name, SN_countryName, MBSTRING_ASC, (unsigned char*)this->wizard->country.c_str(), -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, SN_commonName, MBSTRING_ASC, (unsigned char*)this->wizard->name.c_str(), -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, SN_localityName, MBSTRING_ASC, (unsigned char*)this->wizard->locality.c_str(), -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, SN_stateOrProvinceName, MBSTRING_ASC, (unsigned char*)this->wizard->state.c_str(), -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, SN_organizationName, MBSTRING_ASC, (unsigned char*)this->wizard->org.c_str(), -1, -1, 0);
		X509_NAME_add_entry_by_txt(name, SN_organizationalUnitName, MBSTRING_ASC, (unsigned char*)this->wizard->orgunit.c_str(), -1, -1, 0);

		if (makingCA)
			X509_set_issuer_name(xca, name);

		// add extensions
		wxString email = wxString::Format(wxT("email:%s"), this->wizard->email.c_str());

		if (makingCA)
		{
			ex = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, (char *)email.c_str());
		} else {
			exts = sk_X509_EXTENSION_new_null();

			ex = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name, (char *)email.c_str());
			sk_X509_EXTENSION_push(exts, ex);
		}

		if (makingCA)
			X509_add_ext(xca, ex, -1);
		else
			X509_REQ_add_extensions(x, exts);

		sk_X509_EXTENSION_pop_free(exts, X509_EXTENSION_free);

		this->WorkUpdate(750, (makingCA) ? wxT("Signing root certificate...") : wxT("Signing certificate request..."));

		// fill out rest of x509 stuff
		if (makingCA)
		{
			X509_gmtime_adj(X509_get_notBefore(xca), 0);
			X509_gmtime_adj(X509_get_notAfter(xca), DEFAULT_CLIENT_TIMESPAN);

			long serial = wxConfigBase::Get()->Read("X509Serial", 0L);
			serial++;
			wxConfigBase::Get()->Write("X509Serial", serial);

			X509_set_version(xca, 2L);
			ASN1_INTEGER_set(X509_get_serialNumber(xca), serial);
		}

		//TODO: check return
		if (makingCA)
			X509_sign(xca, pk, EVP_sha1());
		else
			X509_REQ_sign(x, pk, EVP_sha1());

		this->WorkUpdate(900, (makingCA) ? wxT("Saving root certificate...") : wxT("Saving certificate request..."));

		wxString basepath = wxPathOnly(this->wizard->crlocation);
		if (!wxDirExists(basepath))
		{
			wxFileName::Mkdir(basepath, 0755, wxPATH_MKDIR_FULL);
		}

		BIO *crbio = BIO_new_file(this->wizard->crlocation.c_str(), "wb");
		if (crbio)
		{
			if (makingCA)
 				PEM_write_bio_X509(crbio, xca);
			else
 				PEM_write_bio_X509_REQ(crbio, x);
		} else {
			this->WorkUpdate(0, wxString::Format(wxT("Failed: Cannot write to '%s'"), this->wizard->crlocation.c_str()));
			return NULL;
		}
		BIO_free(crbio);

		this->WorkUpdate(950, wxT("Saving keys..."));

		basepath = wxPathOnly(this->wizard->pkeylocation);
		if (!wxDirExists(basepath))
		{
			wxFileName::Mkdir(basepath, 0755, wxPATH_MKDIR_FULL);
		}

		BIO *pkbio = BIO_new_file(this->wizard->pkeylocation.c_str(), "wb");
		if (pkbio)
		{
			PEM_write_bio_RSAPrivateKey(pkbio, rsa, EVP_desx_cbc(), NULL, 0, NULL, (void *)this->wizard->pkeypass.c_str());
			PEM_write_bio_RSAPublicKey(pkbio, rsa);
		} else {
			this->WorkUpdate(0, wxString::Format(wxT("Failed: Cannot write to '%s'"), this->wizard->pkeylocation.c_str()));
			return NULL;
		}
		BIO_free(pkbio);

		this->WorkUpdate(1000, wxT("Finished."));
		
		if (makingCA)
			X509_free(xca);
		else
			X509_REQ_free(x);

		EVP_PKEY_free(pk);
	}

	this->done = true;

	return NULL;
}


/** Wizard for creation of an X509 certificate */
RAIN::GUI::CertificateWizard::CertificateWizard(wxWindow *parent, RAIN::SandboxApp *app)
: wxWizard()
{
	this->Create(parent, -1, wxT("Certificate Wizard"), wxBitmap("IDB_CERTWIZARD_SIDEBAR", wxBITMAP_TYPE_BMP_RESOURCE), wxDefaultPosition, wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER);
	this->app = app;

	this->req = NULL;

	this->type = new RAIN::GUI::CertificateCreateType(this);
	this->signsel = new RAIN::GUI::CertificateSignSelect(this);
	this->signconfirm = new RAIN::GUI::CertificateSignConfirm(this);
	this->subject = new RAIN::GUI::CertificateSubject(this);
	this->key = new RAIN::GUI::CertificateKey(this);
	this->worker = new RAIN::GUI::CertificateWorker(this);

	this->signsel->SetPrev(this->type);
	this->signsel->SetNext(this->signconfirm);
	this->signconfirm->SetPrev(this->signsel);
	this->signconfirm->SetNext(this->worker);

	this->subject->SetPrev(this->type);
	this->subject->SetNext(this->key);
	this->key->SetPrev(this->subject);
	this->key->SetNext(this->worker);
	this->worker->SetPrev(this->key);
	this->worker->SetNext(NULL);
}

bool RAIN::GUI::CertificateWizard::RunWizard()
{
	wxWizard *wiz = (wxWizard *)this;
	return wiz->RunWizard(this->type);
}
