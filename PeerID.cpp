#include "PeerID.h"
#include "CredentialManager.h"
#include "ConnectionPool.h"
#include "BValue.h"

#include <wx/socket.h>

using namespace RAIN;

/** a PeerID encapsulates everything we'll need to know about a peer - 
 *  - their address
 *  - their x509 certificate
 *  - their public key sha1 hash (which is used as a guid across the network)
 */
PeerID::PeerID(bool self)
{
	if (self)
	{
		// we make a peerid representing ourselves but we
		// can't know our own address for obvious reasons
		this->certificate = NULL;

		if (!wxFileExists(CredentialManager::GetCredentialFilename(CL_CERTIFICATE)))
			return;

		BIO *b = BIO_new_file(CredentialManager::GetCredentialFilename(CL_CERTIFICATE).c_str(), "r");

		if (!b)
		{
			wxLogVerbose("Warning: PeerID init: cannot open client certificate");
			return;
		}

		if (this->certificate = PEM_read_bio_X509(b, NULL, NULL, NULL))
		{
			this->HashPublicKey();
		} else {
			wxLogVerbose("Warning: PeerID init: cannot read client certificate");
			return;
		}
	} else {
		this->certificate = NULL;
	}
}

PeerID::PeerID(BEnc::Value *v)
{
	wxLogVerbose("PeerID::PeerID(BEnc::Value)");
	this->certificate = NULL;

	if (v && v->type == BENC_VAL_DICT)
	{
		BENC_SAFE_CAST(v, dv, Dictionary);
		
		BENC_SAFE_CAST(dv->getDictValue("hash"), hash, String);
		BENC_SAFE_CAST(dv->getDictValue("addr"), addr, String); 
		BENC_SAFE_CAST(dv->getDictValue("cert"), cert, String);

		if (hash && addr && cert)
		{
			/* full house */
			this->hash = hash->getString();
			this->address = addr->getString();
			wxString certstr = cert->getString();
			
			unsigned char *x509_b = (unsigned char *) certstr.c_str();
			this->certificate = d2i_X509(NULL, &x509_b, certstr.Length());
		} else if (addr && cert) {
			this->address = addr->getString();
			wxString certstr = cert->getString();
			
			unsigned char *x509_b = (unsigned char *) certstr.c_str();
			this->certificate = d2i_X509(NULL, &x509_b, certstr.Length());
			this->HashPublicKey();
		} else if (hash) {
			this->hash = hash->getString();
		}
	} else if (v && v->type == BENC_VAL_STRING) {
		/* this will be the hash */
		BENC_SAFE_CAST(v, hash, String);
		this->hash = hash->getString();
	}
}

PeerID::PeerID(const wxString& address, X509 *certificate)
{
	if (certificate)
	{
		this->SetCertificate(certificate);
	} else {
		this->certificate = NULL;
		this->hash = wxEmptyString;
	}
}

PeerID::PeerID(const wxString& hash, const wxString& address, X509 *certificate)
{
	if (certificate)
		this->certificate = X509_dup(certificate);
	else
		this->certificate = NULL;

	this->hash = hash;
	this->address = address;
}

PeerID::~PeerID()
{
	//if (this->certificate)
	//	X509_free(this->certificate);
}

void PeerID::Merge(PeerID *that)
{
	PEERID_WANTS wants = this->GetWants();

	if (wants == PEERID_WANTS_ALL)
	{
		this->SetCertificate(that->certificate);
		this->address = that->address;
	} else if (wants == PEERID_WANTS_CERT) {
		this->SetCertificate(that->certificate);
	} else if (wants == PEERID_WANTS_ADDR) {
		this->address = that->address;
	}
}

void PeerID::HashPublicKey()
{
	if (this->certificate == NULL)
		return;

	// hash our own public key
	X509_PUBKEY *pkey = X509_get_X509_PUBKEY(this->certificate);
	EVP_PKEY *evp_pkey = X509_PUBKEY_get(pkey);

	int len = i2d_PUBKEY(evp_pkey, NULL);

	wxString pubkey;
	pubkey.Alloc(len);
	char *buff = pubkey.GetWriteBuf(len);
	i2d_PUBKEY(evp_pkey, (unsigned char**)&buff);
	pubkey.UngetWriteBuf(len);

	// hash public key
	this->hash = CryptoHelper::SHA1Hash(pubkey);
}

void PeerID::SetCertificate(X509 *c)
{
	if (!c)
		return;

	this->certificate = X509_dup(c);

	// hash public key
	X509_PUBKEY *pkey = X509_get_X509_PUBKEY(this->certificate);
	EVP_PKEY *evp_pkey = X509_PUBKEY_get(pkey);

	int len = i2d_PUBKEY(evp_pkey, NULL);

	wxString pubkey;
	pubkey.Alloc(len);
	char *buff = pubkey.GetWriteBuf(len);
	i2d_PUBKEY(evp_pkey, (unsigned char**)&buff);
	pubkey.UngetWriteBuf(len);

	this->hash = CryptoHelper::SHA1Hash(pubkey);
}

bool PeerID::HashMatches(const wxString& p)
{
	return RAIN::SafeStringsEq(p, this->hash);
}

bool PeerID::AddressMatches(const wxString& a)
{
	unsigned long porta = DEFAULT_PORT, portb = DEFAULT_PORT;
	wxIPV4address addra, addrb;

	if (a.Contains(":"))
	{
		addra.Hostname(a.BeforeLast(':'));
		a.AfterLast(':').ToULong(&porta);
		addra.Service(porta);
	} else {
		addra.Hostname(a);
		addra.Service(porta);
	}

	if (this->address.Contains(":"))
	{
		addrb.Hostname(this->address.BeforeLast(':'));
		this->address.AfterLast(':').ToULong(&portb);
		addrb.Service(portb);
	} else {
		addrb.Hostname(this->address);
		addrb.Service(portb);
	}

	return (addra.IPAddress() == addrb.IPAddress() && addra.Service() == addrb.Service());
}

wxString PeerID::CertificateAsString()
{
	BIO *b = BIO_new(BIO_s_mem());
	i2d_X509_bio(b, this->certificate);
	
	BUF_MEM *bptr;
	BIO_get_mem_ptr(b, &bptr);
	BIO_set_close(b, BIO_NOCLOSE);
	BIO_free(b);

	wxString ret(bptr->data, bptr->length);
	BUF_MEM_free(bptr);

	return ret;
}

bool PeerID::HasAddress()
{
	return !(this->address.Length() == 0);
}

wxString PeerID::GetCertificateField(int nid)
{
	if (!this->certificate)
		return "";

	X509_NAME *name = X509_get_subject_name(this->certificate);

	if (!name)
		return "";

	int pos = X509_NAME_get_index_by_NID(name, nid, -1);

	if (pos == -1)
		return "";

	X509_NAME_ENTRY *nval = X509_NAME_get_entry(name, pos);

	return wxString(nval->value->data);
}

PEERID_WANTS PeerID::GetWants()
{
	if ((this->address.Length() == 0) && !this->certificate)
	{
		return PEERID_WANTS_ALL;
	}

	if (!this->certificate)
	{
		return PEERID_WANTS_CERT;
	}

	if (this->address.Length() == 0)
	{
		return PEERID_WANTS_ADDR;
	}

	return PEERID_WANTS_NONE;
}

BEnc::Value * PeerID::AsBValue()
{
	BEnc::Dictionary *v = new BEnc::Dictionary();
	v->hash["hash"] = new BEnc::String(this->hash);

	if (this->address.Length() > 0)
		v->hash["addr"] = new BEnc::String(this->address);

	if (this->certificate)
		v->hash["cert"] = new BEnc::String(this->CertificateAsString());

	return v;
}
