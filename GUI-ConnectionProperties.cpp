#include "GUI-ConnectionProperties.h"
#include "Connection.h"
#include "PeerID.h"

#include <wx/wx.h>

BEGIN_EVENT_TABLE(RAIN::GUI::ConnectionProperties, wxDialog)
	EVT_BUTTON(ID_GUI_CP_OK, RAIN::GUI::ConnectionProperties::OKClick)
END_EVENT_TABLE()

RAIN::GUI::ConnectionProperties::ConnectionProperties(wxWindow *parent, RAIN::Connection *conn)
	: wxDialog()
{
	this->Create(parent, (wxWindowID)-1, wxT("Connection Properties"), wxDefaultPosition, wxDefaultSize);
	this->Notebook = new wxNotebook(this, -1, wxDefaultPosition, wxDefaultSize, 0);

	this->IdentityPage = new wxPanel(this->Notebook, -1);
	this->ConnectionPage = new wxPanel(this->Notebook, -1);

	this->TopSpacer = new wxPanel(this->ConnectionPage, -1);
	this->MainText = new wxStaticText(this->ConnectionPage, -1, wxT("<empty>                                           "));
	this->DataInLabel = new wxStaticText(this->ConnectionPage, -1, wxT("Data In:"));
	this->DataInText = new wxStaticText(this->ConnectionPage, -1, wxT("<empty>"));
	this->DataOutLabel = new wxStaticText(this->ConnectionPage, -1, wxT("Data Out:"));
	this->DataOutText = new wxStaticText(this->ConnectionPage, -1, wxT("<empty>"));
	this->ConnectedSinceLabel = new wxStaticText(this->ConnectionPage, -1, wxT("Connected Since:"));
	this->ConnectedSinceText = new wxStaticText(this->ConnectionPage, -1, wxT("<empty>"));
	this->CipherLabel = new wxStaticText(this->ConnectionPage, -1, wxT("Protocol/Cipher:"));
	this->CipherText = new wxStaticText(this->ConnectionPage, -1, wxT("<empty>"));
	this->KeySizeLabel = new wxStaticText(this->ConnectionPage, -1, wxT("Public Key Type/Size:"));
	this->KeySizeText = new wxStaticText(this->ConnectionPage, -1, wxT("<empty>"));

	this->NameLabel = new wxStaticText(this->IdentityPage, -1, wxT("Name:"));
	this->NameText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));
	this->EmailLabel = new wxStaticText(this->IdentityPage, -1, wxT("Email:"));
	this->EmailText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));
	this->OrgLabel = new wxStaticText(this->IdentityPage, -1, wxT("Organisation:"));
	this->OrgText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));
	this->OrgUnitLabel = new wxStaticText(this->IdentityPage, -1, wxT("Organisational Unit:"));
	this->OrgUnitText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));
	this->LocaleLabel = new wxStaticText(this->IdentityPage, -1, wxT("Locality:"));
	this->LocaleText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));
	this->StateLabel = new wxStaticText(this->IdentityPage, -1, wxT("State/Region:"));
	this->StateText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));
	this->CountryLabel = new wxStaticText(this->IdentityPage, -1, wxT("Country:"));
	this->CountryText = new wxStaticText(this->IdentityPage, -1, wxT("<empty>"));

	this->BottomSpacer = new wxPanel(this, -1);
	this->OkButton = new wxButton(this, ID_GUI_CP_OK, wxT("&OK"));

	this->set_properties();
	this->do_layout();

	this->UpdateConnection(conn);
}

RAIN::GUI::ConnectionProperties::~ConnectionProperties()
{

}

void RAIN::GUI::ConnectionProperties::set_properties()
{
	this->SetTitle(wxT("Connections Properties"));
	this->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_BTNFACE));
	this->OkButton->SetDefault();
}

void RAIN::GUI::ConnectionProperties::do_layout()
{
	wxBoxSizer* MainSizer = new wxBoxSizer(wxVERTICAL);
	wxBoxSizer* BottomSizer = new wxBoxSizer(wxHORIZONTAL);

	wxFlexGridSizer* IdentifySizer = new wxFlexGridSizer(6, 2, 5, 5);
	wxFlexGridSizer* ConnectionSizer = new wxFlexGridSizer(6, 2, 5, 5);

	ConnectionSizer->Add(this->TopSpacer, 1, wxEXPAND, 0);
	ConnectionSizer->Add(this->MainText, 0, wxALL|wxEXPAND|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->DataInLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->DataInText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->DataOutLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->DataOutText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->ConnectedSinceLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->ConnectedSinceText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->CipherLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->CipherText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->KeySizeLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	ConnectionSizer->Add(this->KeySizeText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	this->ConnectionPage->SetAutoLayout(true);
	this->ConnectionPage->SetSizer(ConnectionSizer);
	ConnectionSizer->Fit(this->ConnectionPage);
	ConnectionSizer->SetSizeHints(this->ConnectionPage);
	ConnectionSizer->AddGrowableCol(1);

	IdentifySizer->Add(this->NameLabel, 0, wxLEFT|wxTOP|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->NameText, 0, wxALL|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->EmailLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->EmailText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->OrgLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->OrgText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->OrgUnitLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->OrgUnitText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->LocaleLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->LocaleText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->StateLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->StateText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->CountryLabel, 0, wxLEFT|wxBOTTOM|wxALIGN_RIGHT|wxFIXED_MINSIZE, 10);
	IdentifySizer->Add(this->CountryText, 0, wxLEFT|wxRIGHT|wxBOTTOM|wxFIXED_MINSIZE, 10);
	this->IdentityPage->SetAutoLayout(true);
	this->IdentityPage->SetSizer(IdentifySizer);
	IdentifySizer->Fit(this->IdentityPage);
	IdentifySizer->SetSizeHints(this->IdentityPage);
	IdentifySizer->AddGrowableCol(1);

	this->Notebook->AddPage(this->ConnectionPage, wxT("Connection"));
	this->Notebook->AddPage(this->IdentityPage, wxT("Identity"));
	
	MainSizer->Add(new wxNotebookSizer(Notebook), 1, wxLEFT|wxRIGHT|wxTOP|wxEXPAND, 5);

	BottomSizer->Add(this->BottomSpacer, 1, wxEXPAND, 0);
	BottomSizer->Add(this->OkButton, 0, wxALL|wxFIXED_MINSIZE, 5);
	MainSizer->Add(BottomSizer, 0, wxEXPAND, 0);

	this->SetAutoLayout(true);
	this->SetSizer(MainSizer);
	MainSizer->Fit(this);
	MainSizer->SetSizeHints(this);
	this->Layout();
}

void RAIN::GUI::ConnectionProperties::UpdateConnection(RAIN::Connection *conn)
{
	SSL			*ssl;
	X509		*cert;
	X509_NAME	*name_subject;

	ssl = conn->GetSSL();
	cert = SSL_get_peer_certificate(ssl);

	if (cert == NULL)
	{
		wxLogError("No Peer Certificate");
		this->EndModal(false);
	}

	wxString connstr = wxString::Format("%s to %s", (conn->isConnected) ? "Connected" : "Not connected", conn->pid->address.c_str());
	this->MainText->SetLabel(connstr);

	name_subject = X509_get_subject_name(cert);
	X509_NAME_ENTRY *nval = NULL;

	for (int n = 0; n < sk_X509_NAME_ENTRY_num(name_subject->entries); n++)
	{
		nval = sk_X509_NAME_ENTRY_value(name_subject->entries, n);

		switch (OBJ_obj2nid(nval->object))
		{
			/* individual */
		case NID_commonName:
			this->NameText->SetLabel(wxString(nval->value->data));
			break;
		case NID_organizationName:
			this->OrgText->SetLabel(wxString(nval->value->data));
			break;
		case NID_organizationalUnitName:
			this->OrgUnitText->SetLabel(wxString(nval->value->data));
			break;
			
			/* location */
		case NID_localityName:
			this->LocaleText->SetLabel(wxString(nval->value->data));
			break;
		case NID_stateOrProvinceName:
			this->StateText->SetLabel(wxString(nval->value->data));
			break;
		case NID_countryName:
			this->CountryText->SetLabel(wxString(nval->value->data));
			break;
		default:
			// do nothing
			break;
		}
	}
	
	// get email from x509v3 extension
	int							 subjAltName_pos;
	X509_EXTENSION				*subjAltName;
	X509V3_EXT_METHOD			*method;
	void						*emailraw;
	wxString					 email;

	subjAltName_pos = X509_get_ext_by_NID(cert, OBJ_sn2nid("subjectAltName"), -1);

	if (subjAltName_pos < 0)
	{
		email = "<empty>";
	} else {
		subjAltName = X509_get_ext(cert, subjAltName_pos);
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
	}
	
	this->EmailText->SetLabel(email);

	this->CipherText->SetLabel(wxString::Format("%s/%s", SSL_get_cipher_version(ssl), SSL_get_cipher_name(ssl)));
	this->KeySizeText->SetLabel(wxString::Format("%d bit", SSL_get_cipher_bits(ssl, NULL)));

	X509_PUBKEY *pkey = X509_get_X509_PUBKEY(cert);
	EVP_PKEY *evp_pkey = X509_PUBKEY_get(pkey);

	wxString cipherType;

	/* for some weird reason SN_rsaEncryption is not defined in openssl :(
	 * i expect there is a reason for that */
	switch (OBJ_obj2nid(pkey->algor->algorithm))
	{
	case NID_rsaEncryption:
		cipherType = SN_rsa;
		break;
	default:
		cipherType = OBJ_nid2sn(OBJ_obj2nid(pkey->algor->algorithm));
		break;
	}

	this->KeySizeText->SetLabel(wxString::Format("%s (%d bit)", cipherType.c_str(), EVP_PKEY_bits(evp_pkey)));
	this->DataOutText->SetLabel(wxString::Format("%d bytes", conn->GetBytesSent()));
	this->DataInText->SetLabel(wxString::Format("%d bytes", conn->GetBytesRecv()));
	this->ConnectedSinceText->SetLabel(conn->since.FormatISODate() + " " + conn->since.FormatISOTime());

	this->Fit();
}

void RAIN::GUI::ConnectionProperties::OKClick(wxCommandEvent& event)
{
	this->EndModal(true);
}
