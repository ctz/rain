#ifndef _RAIN_PEERID_H
#define _RAIN_PEERID_H

#include <wx/wx.h>
#include <openssl/x509.h>

namespace RAIN
{
	enum PEERID_WANTS
	{
		PEERID_WANTS_NONE,
		PEERID_WANTS_CERT,
		PEERID_WANTS_ADDR,
		PEERID_WANTS_ALL,
	};

	namespace BEnc
	{
		class Value;
	}

	class PeerID
	{
	public:
		PeerID(const wxString& hash, const wxString& address, X509 *certificate);
		PeerID(const wxString& address, X509 *certificate);
		PeerID(BEnc::Value *v);
		PeerID(bool self = false);
		~PeerID();

		void Merge(PeerID *that);

		bool HasAddress();

		void HashPublicKey();
		void SetCertificate(X509 *c);
		bool HashMatches(const wxString& p);
		bool AddressMatches(const wxString& a);
		PEERID_WANTS GetWants();
		wxString GetCertificateField(int nid);
		wxString CertificateAsString();
		BEnc::Value * AsBValue();
		
		wxString hash;
		wxString address;
		X509 *certificate;
	};
}

#endif//_RAIN_PEERID_H
