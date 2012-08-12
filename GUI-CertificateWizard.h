#ifndef _GUI_CERTIFICATEWIZARD_H
#define _GUI_CERTIFICATEWIZARD_H

#include <wx/wx.h>
#include <wx/wizard.h>
#include <wx/combobox.h>

#include <openssl/x509.h>

enum
{
	CERT_CREATECA = 1,
	CERT_CREATECLIENT,
	CERT_SIGNCLIENT,
};

namespace RAIN
{
	class SandboxApp;

	namespace GUI
	{
		class CertificateWizard;

		class CertificateCreateType : public wxWizardPage
		{
		public:
			CertificateCreateType(RAIN::GUI::CertificateWizard *wizard);
			virtual bool TransferDataFromWindow();
			wxWizardPage * GetPrev() const;
			wxWizardPage * GetNext() const;

		private:
			void set_properties();
			void do_layout();

			RAIN::GUI::CertificateWizard *wizard;

			wxStaticText* TitleLabel;
			wxStaticText* TypeHelp;
			wxRadioBox* TypeChoice;
		};

		class CertificateSignSelect : public wxWizardPageSimple
		{
		public:
			CertificateSignSelect(RAIN::GUI::CertificateWizard *wizard);
			virtual bool TransferDataFromWindow();

		private:
			void set_properties();
			void do_layout();

			RAIN::GUI::CertificateWizard *wizard;

			wxStaticText* TitleLabel;

			wxStaticText* CertReqLocationLabel;
			wxTextCtrl* CertReqLocation;
			wxButton* CertReqLocationChooser;

			wxPanel* KeySpacer1;
			wxStaticText* OrLabel;
			wxPanel* KeySpacer2;

			wxStaticText* PEMPasteLabel;
			wxTextCtrl* PEMPasteText;
			wxPanel* KeySpacer3;
		};

		class CertificateSignConfirm : public wxWizardPageSimple
		{
		public:
			CertificateSignConfirm(RAIN::GUI::CertificateWizard *wizard);
			virtual bool TransferDataFromWindow();

			bool UpdateDynamics();

		private:
			void set_properties();
			void do_layout();

			RAIN::GUI::CertificateWizard *wizard;

			wxStaticText* TitleLabel;

			wxStaticText* NameLabel;
			wxTextCtrl* NameText;

			wxStaticText* OrganisationLabel;
			wxTextCtrl* OrganisationText;

			wxStaticText* OrganisationalUnitLabel;
			wxTextCtrl* OrganisationalUnitText;

			wxStaticText* EmailLabel;
			wxTextCtrl* EmailText;

			wxStaticText* LocalityLabel;
			wxTextCtrl* LocalityText;

			wxStaticText* StateProvinceLabel;
			wxTextCtrl* StateProvinceText;

			wxStaticText* CountryLabel;
			wxTextCtrl* CountryText;

			wxPanel* ConfirmSpacer;
			wxCheckBox* ConfirmBox;
		};

		class CertificateSubject : public wxWizardPageSimple
		{
		public:
			CertificateSubject(RAIN::GUI::CertificateWizard *wizard);
			virtual bool TransferDataFromWindow();

		private:
			void set_properties();
			void do_layout();

			RAIN::GUI::CertificateWizard *wizard;

			wxStaticText* TitleLabel;

			wxStaticText* SubjectHelp;

			wxStaticText* NameLabel;
			wxTextCtrl* NameText;

			wxStaticText* OrganisationLabel;
			wxTextCtrl* OrganisationText;

			wxStaticText* OrganisationalUnitLabel;
			wxTextCtrl* OrganisationalUnitText;

			wxStaticText* EmailLabel;
			wxTextCtrl* EmailText;

			wxStaticText* LocalityLabel;
			wxTextCtrl* LocalityText;

			wxStaticText* StateProvinceLabel;
			wxTextCtrl* StateProvinceText;

			wxStaticText* CountryLabel;
			wxComboBox* CountryChooser;
		};

		class CertificateKey : public wxWizardPageSimple
		{
		public:
			CertificateKey(RAIN::GUI::CertificateWizard *wizard);
			virtual bool TransferDataFromWindow();

			void UpdateDynamics();

		private:
			void set_properties();
			void do_layout();

			RAIN::GUI::CertificateWizard *wizard;

			wxStaticText* TitleLabel;
			wxStaticText* KeyHelp1;

			wxStaticText* KeySizeLabel;
			wxComboBox* KeySizeChooser;
			wxPanel* KeySpacer1;

			wxStaticText* CertReqLocationLabel;
			wxTextCtrl* CertReqLocation;
			wxButton* CertReqLocationChooser;

			wxStaticText* KeyHelp2;

			wxStaticText* PrivateKeyPass1Label;
			wxTextCtrl* PrivateKeyPassword1;
			wxPanel* KeySpacer2;

			wxStaticText* PrivateKeyPass2Label;
			wxTextCtrl* PrivateKeyPassword2;
			wxPanel* KeySpacer3;

			wxStaticText* PrivateKeyLocationLabel;
			wxTextCtrl* PrivateKeyLocation;
			wxButton* PrivateKeyLocationChooser;
		};

		class CertificateWorker : public wxWizardPageSimple, public wxThreadHelper
		{
		public:
			CertificateWorker(RAIN::GUI::CertificateWizard *wizard);
			virtual bool TransferDataFromWindow();

			static void RSAGenerateCallback(int a, int b, void *that);

			void * Entry();

		private:
			void set_properties();
			void do_layout();

			void WorkUpdate(int value, const wxString& message);

			bool done;

			RAIN::GUI::CertificateWizard *wizard;
			
			wxStaticText* TitleLabel;
			wxGauge* WorkGauge;
			wxStaticText* DoneLabel;
		};

		class CertificateWizard : public wxWizard
		{
		public:
			CertificateWizard(wxWindow *parent, RAIN::SandboxApp *app);

			bool RunWizard();

			RAIN::GUI::CertificateCreateType *type;

			RAIN::GUI::CertificateSignSelect *signsel;
			RAIN::GUI::CertificateSignConfirm *signconfirm;

			RAIN::GUI::CertificateSubject *subject;
			RAIN::GUI::CertificateKey *key;
			RAIN::GUI::CertificateWorker *worker;

			// CERT_CREATECA, CERT_CREATECLIENT or CERT_SIGNCLIENT
			int mode;

			// signee details
			wxString crqfilename;
			wxString crqstring;
			wxString certfilename;

			// subject details
			wxString name;
			wxString org;
			wxString orgunit;
			wxString email;
			wxString locality;
			wxString state;
			wxString country;

			// key/other stuff
			int keysize;
			wxString crlocation;
			wxString pkeypass;
			wxString pkeylocation;

			// to prevent race condition when signing (could be a problem)
			X509_REQ *req;

		private:
			RAIN::SandboxApp *app;
		};
	}
}

#endif//_GUI_CERTIFICATEWIZARD_H
