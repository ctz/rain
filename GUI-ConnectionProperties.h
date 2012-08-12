#ifndef _RAIN_GUI_CERTIFICATEPROPERTIES_H
#define _RAIN_GUI_CERTIFICATEPROPERTIES_H

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/datetime.h>

#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/asn1.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>

namespace RAIN
{
	class Connection;

	namespace GUI
	{
		class ConnectionProperties : public wxDialog
		{
		public:
			ConnectionProperties(wxWindow *parent, RAIN::Connection *conn);
			~ConnectionProperties();

			void set_properties();
			void do_layout();

			void UpdateConnection(RAIN::Connection *conn);
			void OKClick(wxCommandEvent& event);

			DECLARE_EVENT_TABLE();

		private:
			wxPanel* TopSpacer;
			wxStaticText* MainText;
			wxStaticText* DataInLabel;
			wxStaticText* DataInText;
			wxStaticText* DataOutLabel;
			wxStaticText* DataOutText;
			wxStaticText* ConnectedSinceLabel;
			wxStaticText* ConnectedSinceText;
			wxStaticText* CipherLabel;
			wxStaticText* CipherText;
			wxStaticText* KeySizeLabel;
			wxStaticText* KeySizeText;
			wxPanel* ConnectionPage;
			wxStaticText* NameLabel;
			wxStaticText* NameText;
			wxStaticText* EmailLabel;
			wxStaticText* EmailText;
			wxStaticText* OrgLabel;
			wxStaticText* OrgText;
			wxStaticText* OrgUnitLabel;
			wxStaticText* OrgUnitText;
			wxStaticText* LocaleLabel;
			wxStaticText* LocaleText;
			wxStaticText* StateLabel;
			wxStaticText* StateText;
			wxStaticText* CountryLabel;
			wxStaticText* CountryText;
			wxPanel* IdentityPage;
			wxNotebook* Notebook;
			wxPanel* BottomSpacer;
			wxButton* OkButton;
		};
	}
}

enum
{
	ID_GUI_CP_OK,
};

#endif//_RAIN_GUI_CERTIFICATEPROPERTIES_H
