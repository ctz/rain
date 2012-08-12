#include "CredentialManager.h"
#include "PeerID.h"

#include "_wxconfig.h"

using namespace RAIN;

CredentialManager *globalCredentialManager = NULL;
wxMutex *globalMutexStore = NULL;

/** Initialises a CredentialManager. The Credential Manager acts as a service to
  * other classes.  It sets up an SSL_CTX (SSL context) from which the SSL server
  * socket and client sockets are created.  It provides neccessary OpenSSL callbacks
  * which gather passwords and verify SSL certificates.
  */
CredentialManager::CredentialManager(bool initAll)
{
	// init openssl
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	RAND_poll();

	this->ssl_ctx = NULL;

	// create a peerid for myself
	this->myid = new PeerID(true);

	// set up threading callbacks
	int mutexes = CRYPTO_num_locks();
	::globalMutexStore = new wxMutex[mutexes];
	CRYPTO_set_locking_callback(CredentialManager::LockingCallback);

	// only initialise ssl_context if we have a client certificate
	// (loaded in self-peerid constructor)
	if (initAll && this->myid->certificate)
		this->Initialise();
}

CredentialManager::~CredentialManager()
{
	delete[] ::globalMutexStore;
}

void CredentialManager::Initialise()
{
	this->ssl_ctx = SSL_CTX_new(TLSv1_method());

	SSL_CTX_set_timeout(this->ssl_ctx, 1800);
	SSL_CTX_set_verify(this->ssl_ctx, SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT, CredentialManager::OpenSSLx509VerificationCallback);
	SSL_CTX_set_default_passwd_cb(this->ssl_ctx, CredentialManager::OpenSSLPasswordCallback);
	SSL_CTX_set_mode(this->ssl_ctx, SSL_MODE_ENABLE_PARTIAL_WRITE);
	SSL_CTX_set_mode(this->ssl_ctx, SSL_MODE_ACCEPT_MOVING_WRITE_BUFFER);

	if (!wxFileExists(CredentialManager::GetCredentialFilename(CA_CERTIFICATE)))
	{
		wxLogVerbose("Warning: CA certificate does not exist - cannot initialise network.");
		SSL_CTX_free(this->ssl_ctx);
		this->ssl_ctx = NULL;
		return;
	}

	SSL_CTX_load_verify_locations(this->ssl_ctx, CredentialManager::GetCredentialFilename(CA_CERTIFICATE).c_str(), NULL);

	if (!wxFileExists(CredentialManager::GetCredentialFilename(CL_CERTIFICATE)))
	{
		wxLogVerbose("Warning: Client certificate does not exist - cannot initialise network.");
		SSL_CTX_free(this->ssl_ctx);
		this->ssl_ctx = NULL;
		return;
	}

	if (SSL_CTX_use_certificate_file(this->ssl_ctx, CredentialManager::GetCredentialFilename(CL_CERTIFICATE), SSL_FILETYPE_PEM) != 1)
		wxLogError("SSL_CTX_use_certificate_file failed");

	if (!wxFileExists(CredentialManager::GetCredentialFilename(CL_PRIVATEKEY)))
	{
		wxLogVerbose("Warning: Client private key does not exist - cannot initialise network.");
		SSL_CTX_free(this->ssl_ctx);
		this->ssl_ctx = NULL;
		return;
	}

	int tries = 3;
	while (tries-- && SSL_CTX_use_PrivateKey_file(this->ssl_ctx, CredentialManager::GetCredentialFilename(CL_PRIVATEKEY), SSL_FILETYPE_PEM) != 1)
	{	
		wxThread::Sleep(500);
		wxLogError(wxT("SSL_CTX_use_PrivateKey_file failed"));
	}

	if (tries < 0)
	{
		wxThread::Sleep(500);
		wxLogFatalError(wxT("Incorrect password entered 3 times, exiting."));
		return;
	}
}

/** openssl mutex callback */
void CredentialManager::LockingCallback(int mode, int n, const char *file, int line)
{
	if (::globalMutexStore == NULL)
	{
		wxASSERT(0);
		return;
	}

	if (mode & CRYPTO_LOCK)
		::globalMutexStore[n].Lock();
	else
		::globalMutexStore[n].Unlock();
}

/** a generic "get password" dialog function */
wxString CredentialManager::GetPrivateKeyPassword()
{
	return wxGetPasswordFromUser("Enter the password for your private key", "Password", "", NULL);
}

/** This is used by OpenSSL as a callback to gather passwords
  * for the private key file loaded into the SSL context */
int CredentialManager::OpenSSLPasswordCallback(char *buf, int size, int flag, void *userdata)
{
	wxString passwd = CredentialManager::GetPrivateKeyPassword();
	strncpy(buf, passwd.c_str(), size);
	return passwd.Length();
}

/** Another OpenSSL callback, this time called when a certificate
  * chain needs to be verified */
int CredentialManager::OpenSSLx509VerificationCallback(int ok, X509_STORE_CTX *store)
{
	/* pretty much stolen from the openssl docs */
	if (!ok)
	{
		wxString errStr;
		char *errStrBuff;

		X509 *cert = X509_STORE_CTX_get_current_cert(store);
		int  depth = X509_STORE_CTX_get_error_depth(store);
		int  err   = X509_STORE_CTX_get_error(store);

		wxLogVerbose("** Error with certificate at depth: %i", depth);

		errStrBuff = errStr.GetWriteBuf(1024);
		X509_NAME_oneline(X509_get_issuer_name(cert), errStrBuff, 1024);
		errStr.UngetWriteBuf(1024);
		wxLogVerbose("  - Issuer = %s", errStr.c_str());

		errStrBuff = errStr.GetWriteBuf(1024);
		X509_NAME_oneline(X509_get_subject_name(cert), errStrBuff, 1024);
		errStr.UngetWriteBuf(1024);
		wxLogVerbose("  - Subject = %s", errStr.c_str());
		wxLogVerbose("** Error %d: %s", err, X509_verify_cert_error_string(err));

//TODO: remove comment#ifdef __WXDEBUG__
		if (err == X509_V_ERR_DEPTH_ZERO_SELF_SIGNED_CERT || err == X509_V_ERR_CERT_HAS_EXPIRED)
		{
			// debugging - allow self-signs
			wxLogVerbose("** DEBUG: Verifying self-sign or expired cert");
			return 1;
		}
//#endif//__WXDEBUG__
	}

	return ok;
}

/** Returns the current SSL_CTX object, which is used as a template for SSL client and server sockets */
SSL_CTX * CredentialManager::GetSSLContext()
{
	return this->ssl_ctx;
}

/** Returns the local PeerID, which is a wrapper for our certificate and public key hash */
PeerID * CredentialManager::GetMyPeerID()
{
	return this->myid;
}

/** returns the local, absolute filename of some CREDENTIAL_FILE */
wxString CredentialManager::GetCredentialFilename(CREDENTIAL_FILE file)
{
	wxConfig *cfg = (wxConfig *) wxConfig::Get();
	wxString _default;

#ifdef __WIN32__
	switch (file)
	{
	case CA_CERTIFICATE:
		_default = "ca-cert.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/CACertificate", _default);
	case CA_PRIVATEKEY:
		_default = "ca-pkey.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/CAPrivateKey", _default);
	case CL_CERTIFICATE:
		_default = "client-cert.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/ClientCertificate", _default);
	case CL_PRIVATEKEY:
		_default = "client-pkey.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/ClientPrivateKey", _default);
	case CL_CERTREQ:
		_default = "client-creq.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/ClientCertificateRequest", _default);
	case NETWORK_CRL:
		_default = "revoked.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/CertificateRevocations", _default);
	case MANIFEST_DIR:
		_default = wxString::Format("%s\\Application Data\\RAIN\\manifest-cache\\", wxGetUserHome());
		return cfg->Read("/ManifestCache", _default);
	case PIECE_DIR:
		_default = wxString::Format("%s\\Application Data\\RAIN\\piece-cache\\", wxGetUserHome());
		return cfg->Read("/PieceCache", _default);
	case OUTPUT_DIR:
		_default = wxString::Format("%s\\Application Data\\RAIN\\output\\", wxGetUserHome());
		return cfg->Read("/Output", _default);
	case CREDENTIAL_BASE:
		_default = wxString::Format("%s\\Application Data\\RAIN\\", wxGetUserHome());
		return cfg->Read("/Credentials", _default);
	default:
		return wxEmptyString;
	}
#else
	switch (file)
	{
	case CA_CERTIFICATE:
		_default = "ca-cert.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/CACertificate", _default);
	case CA_PRIVATEKEY:
		_default = "ca-pkey.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/CAPrivateKey", _default);
	case CL_CERTIFICATE:
		_default = "client-cert.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/ClientCertificate", _default);
	case CL_PRIVATEKEY:
		_default = "client-pkey.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/ClientPrivateKey", _default);
	case CL_CERTREQ:
		_default = "client-creq.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/ClientCertificateRequest", _default);
	case NETWORK_CRL:
		_default = "revoked.pem";
		return CredentialManager::GetCredentialFilename(CREDENTIAL_BASE) + cfg->Read("/CertificateRevocations", _default);
	case MANIFEST_DIR:
		_default = wxString::Format("%s/.RAIN/manifest-cache/", wxGetUserHome());
		return cfg->Read("/ManifestCache", _default);
	case PIECE_DIR:
		_default = wxString::Format("%s/.RAIN/piece-cache/", wxGetUserHome());
		return cfg->Read("/PieceCache", _default);
	case OUTPUT_DIR:
		_default = wxString::Format("%s/.RAIN/output/", wxGetUserHome());
		return cfg->Read("/Output", _default);
	case CREDENTIAL_BASE:
		_default = wxString::Format("%s/.RAIN/", wxGetUserHome());
		return cfg->Read("/Credentials", _default);
	default:
		return wxEmptyString;
	}
#endif//__WIN32__
}

/** Debugging. Dumps the contents of an X509 certificate */
long CredentialManager::ShowCertificate(SSL *ssl)
{
	X509		*cert;
	X509_NAME	*cert_subject, *cert_issuer;
	
	cert = SSL_get_peer_certificate(ssl);
	if (cert == NULL)
	{
		wxLogVerbose("CredentialManager::ShowCertificate - no peer cert");
		return X509_V_ERR_APPLICATION_VERIFICATION;
	}

	cert_subject = X509_get_subject_name(cert);
	cert_issuer = X509_get_issuer_name(cert);
	
	/* iterate over the x509 issuer and subject; print */
	X509_NAME_ENTRY *nval = NULL;
	
	int n;
	wxLogVerbose("** Subject:");
	for (n = 0; n < sk_X509_NAME_ENTRY_num(cert_subject->entries); n++)
	{
		nval = sk_X509_NAME_ENTRY_value(cert_subject->entries, n);
		wxLogVerbose("\t%s => %s", OBJ_nid2ln(OBJ_obj2nid(nval->object)), nval->value->data);
	}

	wxLogVerbose("** Issuer:");
	for (n = 0; n < sk_X509_NAME_ENTRY_num(cert_issuer->entries); n++)
	{
		nval = sk_X509_NAME_ENTRY_value(cert_issuer->entries, n);
		wxLogVerbose("\t%s => %s", OBJ_nid2ln(OBJ_obj2nid(nval->object)), nval->value->data);
	}

	X509_free(cert);

	return 0;
}
