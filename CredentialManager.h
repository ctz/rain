#ifndef _RAIN_CREDENTIALMANAGER_H
#define _RAIN_CREDENTIALMANAGER_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/ffile.h>
#include <openssl/ssl.h>
#undef CERT
//#include <ssl/ssl_locl.h>
#include <openssl/rand.h>
#include <openssl/x509.h>
#include <openssl/x509v3.h>

#include "CryptoHelper.h"


namespace RAIN
{
	enum CREDENTIAL_FILE
	{	
		CREDENTIAL_BASE,
		CA_PRIVATEKEY,
		CA_CERTIFICATE,
		CL_PRIVATEKEY,
		CL_CERTIFICATE,
		CL_CERTREQ,
		NETWORK_CRL,
		MANIFEST_DIR,
		PIECE_DIR,
		OUTPUT_DIR,
	};

	class PeerID;

	class CredentialManager
	{
	public:
		CredentialManager(bool initAll);
		~CredentialManager();

		void Initialise();

		static wxString GetPrivateKeyPassword();

		static void LockingCallback(int mode, int n, const char *file, int line);
		static int OpenSSLPasswordCallback(char *buf, int size, int flag, void *userdata);
		static int OpenSSLx509VerificationCallback(int ok, X509_STORE_CTX *store);
		static long ShowCertificate(SSL *ssl);

		SSL_CTX * GetSSLContext();
		PeerID * GetMyPeerID();

		static wxString GetCredentialFilename(RAIN::CREDENTIAL_FILE file);

	private:
		SSL_CTX *ssl_ctx;
		PeerID *myid;
	};
}

#endif //_RAIN_CREDENTIALMANAGER_H
