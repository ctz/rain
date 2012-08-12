#include "ManifestService.h"
#include "BValue.h"
#include "CredentialManager.h"
#include "MessageCore.h"
#include "PeerID.h"
#include "PieceService.h"
#include "SystemIDs.h"

#include <wx/busyinfo.h>

using namespace RAIN;
extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;


/** a manifest entry is a single manifest, stored in a single file */
ManifestEntry::ManifestEntry(BEnc::Value *v)
{
	if (v != NULL)
		this->v = (BEnc::Dictionary *) BEnc::copy(v);
	else
		this->v = NULL;
}

ManifestEntry::ManifestEntry(wxFileName &filename)
{
	this->v = NULL;
	this->Load(filename);
}

void ManifestEntry::Load(wxFileName &filename)
{
	delete this->v;
	this->v = NULL;

	wxFFile f(filename.GetFullPath().c_str(), "rb");
	
	if (f.IsOpened())
	{
		wxString buff,all;

		while (!f.Eof())
		{
			wxChar *b = buff.GetWriteBuf(16384);
			size_t r = f.Read(b, 16384);
			buff.UngetWriteBuf(16384);
			buff.Truncate(r);
			all.Append(buff);
		}

		f.Close();

		this->v = new BEnc::Dictionary();
		this->v->Read(all);
	} else {
		this->v = NULL;
	}
}

void ManifestEntry::Save()
{
	BENC_SAFE_CAST(this->v, dv, Dictionary);
	BENC_SAFE_CAST(dv->getDictValue("hash"), dvhash, String);
	wxFileName fn(CredentialManager::GetCredentialFilename(RAIN::MANIFEST_DIR), CryptoHelper::HashToHex(dvhash->getString()), "rnm");
	wxString str;
	this->v->Write(&str);

	wxFFile f(fn.GetFullPath().c_str(), "wb");

	if (f.IsOpened())
	{
		f.Write(str.GetData(), str.Length());
		f.Close();
	} else {
		wxLogVerbose("Warning: Manifest write failed for %s - cannot open %s", CryptoHelper::HashToHex(dvhash->getString()).c_str(), fn.GetFullPath().c_str());
	}
}

bool ManifestEntry::Verify()
{
	// hack: remove this
	wxLogVerbose("ManifestEntry::Verify()");
	return true;

	// todo: verification of signed manifests
	if (this->v)
		return (v->hasDictValue("meta") && v->hasDictValue("files") && v->hasDictValue("pieces"));
	else
		return false;
}

ManifestEntry::~ManifestEntry()
{
	delete this->v;
}

BEnc::Value * ManifestEntry::getBenc()
{
	return BEnc::copy(this->v);
}

time_t ManifestEntry::getTimestamp()
{
	if (this->v && this->v->hasDictValue("Timestamp"))
	{
		BENC_SAFE_CAST(this->v->getDictValue("Timestamp"), timestamp, Integer);
		return timestamp->getNum();
	} else {
		return 0;
	}
}

BEnc::Dictionary * ManifestEntry::getMeta()
{
	BENC_SAFE_CAST(this->v->getDictValue("meta"), ret, Dictionary);
	return ret;
}

BEnc::Dictionary * ManifestEntry::getFiles()
{
	BENC_SAFE_CAST(this->v->getDictValue("files"), ret, Dictionary);
	return ret;
}

BEnc::Dictionary * ManifestEntry::getPieces()
{
	BENC_SAFE_CAST(this->v->getDictValue("pieces"), ret, Dictionary);
	return ret;
}

/** ManifestCreationFile describes one file to be added to a manifest */
ManifestCreationFile::ManifestCreationFile(const wxString& filename)
{
	this->filename = wxFileName(filename);
	this->UpdateFileSize();
}

ManifestCreationFile::ManifestCreationFile(const wxArrayString& base, const wxFileName& filename)
{
	this->prepend = base;
	this->filename = wxFileName(filename);
	this->UpdateFileSize();
}

void ManifestCreationFile::UpdateFileSize()
{
	//todo: use wxstat
	wxFile *f = new wxFile(this->filename.GetFullPath());

	if (!f->IsOpened())
	{
		this->fileSize = 0;
		return;
	}

	f->SeekEnd();
	this->fileSize = f->Tell();
	f->Close();
	delete f;
}

wxString ManifestCreationFile::GetTranslatedPath(const wxString& meshbase)
{
	wxString spacer;

	if (meshbase.AfterLast('/').Length() != 0 || meshbase.Length() == 0)
		spacer = "/";

	wxFileName fn(meshbase + spacer);
	size_t meshbase_len = fn.GetDirCount();

	for (int i = 0; i < this->prepend.GetCount(); i++)
	{
		fn.InsertDir(meshbase_len++, this->prepend[i]);
	}

	fn.SetName(this->filename.GetFullName());
	return fn.GetFullPath(wxPATH_UNIX);
}

struct ManifestCreationFileSort
{
	bool operator()(ManifestCreationFile* const& a, ManifestCreationFile* const& b)
	{
		return (a->filename.GetFullPath().Cmp(b->filename.GetFullPath().c_str()) < 0);
	}
};

/** creates a manifestentry and returns it, along with an error code/messages */
ManifestCreationResult ManifestService::Create(std::vector<ManifestCreationFile*> *files, const wxString& base)
{
	ManifestCreationResult ret;
	wxBusyInfo busy(wxString::Format("Creating manifest for %d files", files->size()));

	/* sort files vec by case sensitive filename */
	std::sort(files->begin(), files->end(), ManifestCreationFileSort());

	ret.errorCode = MCE_ERROR_OK;
	ret.manifest = new ManifestEntry(NULL);
	ret.manifest->v = new BEnc::Dictionary();
	BEnc::Dictionary *b = ret.manifest->v,
		*bfiles = NULL;
	BEnc::List *bfiles_names = NULL,
		*bfiles_sizes = NULL,
		*bfiles_hashes = NULL,
		*bpieces = NULL;

	/* empty meta dict for misc data */
	b->hash["meta"] = new BEnc::Dictionary();

	/* files dict for resultant file info */
	bfiles = new BEnc::Dictionary();
	bfiles_names = new BEnc::List();
	bfiles_sizes = new BEnc::List();
	bfiles_hashes = new BEnc::List();

	for (size_t i = 0; i < files->size(); i++)
	{
		bfiles_names->pushList(new BEnc::String(files->at(i)->GetTranslatedPath(base)));

		BEnc::Integer *size = new BEnc::Integer(files->at(i)->fileSize);
		bfiles_sizes->pushList(size);

		wxString hash_str = CryptoHelper::SHA1HashFile(files->at(i)->filename);

		if (hash_str == "")
		{
			ret.error = wxString::Format("File %s not found", files->at(i)->filename.GetFullPath().c_str());
			ret.errorCode = MCE_ERROR_MISSING_FILE;
			delete ret.manifest;
			ret.manifest = NULL;
			return ret;
		}

		bfiles_hashes->pushList(new BEnc::String(hash_str));
	}

	// collapse back into manifest
	bfiles->hash["names"] = bfiles_names;
	bfiles->hash["sizes"] = bfiles_sizes;
	bfiles->hash["hashes"] = bfiles_hashes;
	b->hash["files"] = bfiles;

	/* all files' hash for a guid */
	std::vector<wxFileName> filenames;

	for (size_t i = 0; i < files->size(); i++)
	{
		filenames.push_back(wxFileName(files->at(i)->filename));
	}

	b->hash["hash"] = new BEnc::String(CryptoHelper::SHA1HashFiles(filenames));

	/* creation timestamp */
	b->hash["timestamp"] = new BEnc::Integer(wxDateTime::Now().GetTicks());

	BEnc::Dictionary *pieces = new BEnc::Dictionary();
	b->hash["pieces"] = pieces;

	PieceService::Create(files, pieces);

	if (ManifestService::Sign(*ret.manifest) == MSR_ERROR_OK)
	{
		// hooray!
		ret.manifest->Save();

		// add pieces to queue
		Message *tmpmsg = new Message();
		tmpmsg->status = MSG_STATUS_LOCAL;
		tmpmsg->subsystem = PIECEQUEUE_ID;
		tmpmsg->headers["Event"] = new BEnc::String("Queue-Piece");
		tmpmsg->headers["Piece-Direction"] = new BEnc::String("Outgoing");

		BENC_SAFE_CAST(b->getDictValue("pieces"), piecehashes, Dictionary);
		BENC_SAFE_CAST(piecehashes->getDictValue("hashes"), piecehashlist, List);
		BENC_SAFE_CAST(piecehashes->getDictValue("size"), piecesize, Integer);

		for (size_t i = 0; i < piecehashlist->getListSize(); i++)
		{
			Message *cpymsg = new Message(tmpmsg);
			BENC_SAFE_CAST(piecehashlist->getListItem(i), listitem, String);
			cpymsg->headers["Piece-Hash"] = new BEnc::String(listitem->getString());
			cpymsg->headers["Piece-Size"] = new BEnc::Integer(piecesize->getNum());
			globalMessageCore->PushMessage(cpymsg);
		}

		delete tmpmsg;

	} else {
		ret.error = "Signing failed";
		ret.errorCode = MCE_ERROR_BAD_PERMISSIONS;
		delete ret.manifest;
		ret.manifest = NULL;
	}

	return ret;
}

ManifestSignatureResult ManifestService::Sign(ManifestEntry& m)
{
	// we cannot sign already signed manifests, it makes no sense and
	// we cannot sign manifests already containing a certificate
	if (m.v->hasDictValue("signature") || m.v->hasDictValue("origin"))
		return MSR_ERROR_ALREADYSIGNED;

	m.v->hash["origin"] = new BEnc::String(globalCredentialManager->GetMyPeerID()->CertificateAsString());

	EVP_MD_CTX md_ctx;
	EVP_SignInit(&md_ctx, EVP_sha1());

	wxString benc;
	m.v->Write(&benc);

	EVP_SignUpdate(&md_ctx, benc.c_str(), benc.Length());

	// load private key
	BIO *bio = BIO_new_file(RAIN::CredentialManager::GetCredentialFilename(RAIN::CL_PRIVATEKEY).c_str(), "rb");
	
	if (!bio)
		return MSR_ERROR_BADCREDENTIALS;
	
	EVP_PKEY *pkey = PEM_read_bio_PrivateKey(bio, NULL, RAIN::CredentialManager::OpenSSLPasswordCallback, NULL);
	BIO_free(bio);

	if (!pkey)
		return MSR_ERROR_BADCREDENTIALS;

	// do actual signing
	wxString signature;
	size_t realsigsize,maxsigsize = EVP_PKEY_size(pkey);
	signature.Alloc(maxsigsize);
	unsigned char *sigbuff = (unsigned char*) signature.GetWriteBuf(maxsigsize);
	EVP_SignFinal(&md_ctx, sigbuff, &realsigsize, pkey);
	signature.UngetWriteBuf(maxsigsize);
	signature.Truncate(realsigsize);

	// update manifest with signatures
	m.v->hash["signature"] = new BEnc::String(signature);

	return MSR_ERROR_OK;
}

ManifestSignatureResult ManifestService::VerifySignature(ManifestEntry& m)
{
	if (!(m.v->hasDictValue("signature") && m.v->hasDictValue("origin")))
		return MSR_ERROR_UNSIGNED;

	// extract public key
	BENC_SAFE_CAST(m.v->getDictValue("origin"), originstr, String);
	wxString certstr = originstr->getString();

	// remove signature
	BENC_SAFE_CAST(m.v->getDictValue("signature"), signaturestr, String);
	wxString signature = signaturestr->getString();
	BEnc::Dictionary *v = new BEnc::Dictionary(m.v);
	delete v->getDictValue("signature");
	v->hash.erase("signature");

	wxString data;
	v->Write(&data);

	delete v;
			
	unsigned char *x509_b = (unsigned char *) certstr.c_str();
	X509 *certificate = d2i_X509(NULL, &x509_b, certstr.Length());

	if (certificate)
	{
		X509_PUBKEY *pkey = X509_get_X509_PUBKEY(certificate);
		EVP_PKEY *evp_pkey = X509_PUBKEY_get(pkey);

		EVP_MD_CTX md_ctx;
		EVP_VerifyInit(&md_ctx, EVP_sha1());
		EVP_VerifyUpdate(&md_ctx, data.c_str(), data.Length());
		
		int result = EVP_VerifyFinal(&md_ctx, (unsigned char*)signature.c_str(), signature.Length(), evp_pkey);

		X509_free(certificate);

		switch (result)
		{
		case -1:	return MSR_ERROR_BADCREDENTIALS;
		case 0:		return MSR_ERROR_BADSIGNATURE;
		case 1:		return MSR_ERROR_OK;
		default:	return MSR_ERROR_BADCREDENTIALS;
		}
	} else {
		return MSR_ERROR_BADCREDENTIALS;
	}
}
