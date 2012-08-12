#include "PieceService.h"
#include "ManifestService.h"
#include "BValue.h"
#include "CryptoHelper.h"
#include "CredentialManager.h"

#include <wx/busyinfo.h>

#define PIECE_SIZE 524288
#define DEFAULT_CIPHER "aes-192-cbc"

using namespace RAIN;
extern CredentialManager *globalCredentialManager;
extern MessageCore *globalMessageCore;

/** creates a set of pieces in the local piece cache using <i>files</i> as 
  * a list of source files and puts the result in the dictionary <i>pieces</i>
  * \return a PieceCreationResult filled with statistics of the creation */
PieceCreationResult PieceService::Create(std::vector<ManifestCreationFile*> *files, BEnc::Dictionary *pieces)
{
	wxBusyInfo busy(wxString::Format("Splitting and encrypting %d files", files->size()));
	wxLogVerbose("PieceService::Create given %d files", files->size());

	for (size_t i = 0; i < files->size(); i++)
	{
		wxLogVerbose("PieceService::Create: %d => %s", i, files->at(i)->filename.GetFullPath().c_str());
	}

	PieceCreationResult ret;
	ret.dataSize = 0;
	ret.pieces = 0;
	ret.pieceSize = PIECE_SIZE;

	unsigned long long globalOffset = 0;
	unsigned long long fileStartOffset = 0;
	
	std::vector<ManifestCreationFile*>::iterator it;

	bool finished = false, error = false;

	unsigned char inBuff[PIECE_SIZE];
	size_t inSize = 0, rdSize = 0;

	/* holds the hashes of all the pieces we create */
	std::vector<wxString> hashes;

	/* generate key & iv */
	unsigned char key[EVP_MAX_KEY_LENGTH], iv[EVP_MAX_IV_LENGTH];
	RAND_bytes(key, EVP_MAX_KEY_LENGTH);
	RAND_bytes(iv, EVP_MAX_IV_LENGTH);

	/* init encryptor */
	
	off_t pieceNumber = 0;
	wxFFile tmp;
	
	wxString tmpfile = PieceService::OpenNextPiece(&tmp);

	for (it = files->begin(); it != files->end(); it++)
	{
		wxFFile *fIn = new wxFFile((*it)->filename.GetFullPath(), "rb");

		while (!fIn->Eof())
		{
			rdSize = fIn->Read(inBuff + inSize, PIECE_SIZE - inSize);

			inSize += rdSize;

			if (PIECE_SIZE == inSize)
			{
				// we've now read a full piece, save it and advance
				ret.dataSize += PieceService::EncryptAndWritePiece(key, iv, wxString(DEFAULT_CIPHER), &tmp, inBuff, inSize);
				pieceNumber++;

				if (tmp.IsOpened())
					tmp.Close();
				hashes.push_back(PieceService::SealPiece(tmpfile));

				tmpfile = PieceService::OpenNextPiece(&tmp);
				inSize = 0;
			}
		}

		fIn->Close();
		delete fIn;
	}

	ret.dataSize += PieceService::EncryptAndWritePiece(key, iv, wxString(DEFAULT_CIPHER), &tmp, inBuff, inSize);
	ret.pieces = pieceNumber + 1;

	if (tmp.IsOpened())
		tmp.Close();
	hashes.push_back(PieceService::SealPiece(tmpfile));

	BEnc::Dictionary *keys = new BEnc::Dictionary();
	keys->hash["data"] = new BEnc::String(key);
	keys->hash["iv"] = new BEnc::String(iv);
	keys->hash["protocol"] = new BEnc::String(DEFAULT_CIPHER);
	pieces->hash["keys"] = keys;

	pieces->hash["size"] = new BEnc::Integer(PIECE_SIZE);

	BEnc::List *hashlist = new BEnc::List();

	for (size_t i = 0; i < hashes.size(); i++)
	{
		hashlist->pushList(new BEnc::String(hashes[i]));
	}

	pieces->hash["hashes"] = hashlist;

	return ret;
}

/** closes <i>f</i>, generates a new temporary filename and
  * reopens <i>f</i> using it.
  * \return the temporary filename */
wxString PieceService::OpenNextPiece(wxFFile *f)
{
	if (f->IsOpened())
		f->Close();

	wxString tmp = wxFileName::CreateTempFileName("rain");

	f->Open(tmp, "wb");
	return tmp;
}

/** hashes the file <i>filename</i> (which is a temp file), then
  * copies the file to move it into the piece cache and removes
  * the temp file
  * \return the raw hash of the file contents */
wxString PieceService::SealPiece(const wxString& filename)
{
	wxString hash = CryptoHelper::SHA1HashFile(wxFileName(filename));
	wxString nicehash = CryptoHelper::HashToHex(hash);

	::wxCopyFile(filename, PieceService::GetPieceFileName(nicehash), true);
	::wxRemoveFile(filename);

	return hash;
}

/** \return the filename of the piece with hexadecimally encoded
  * hash <i>hexhash</i> */
wxString PieceService::GetPieceFileName(const wxString& hexhash)
{
	return wxFileName(CredentialManager::GetCredentialFilename(RAIN::PIECE_DIR), hexhash, "rnp").GetFullPath();
}

/** encrypts the data at <i>in</i> length <i>inl</i> using the encryption
  * context <i>ctx</i> and then writes it to file <i>f</i> which must
  * be open and writable
  * \return the number of bytes written as reported by wxFFile::Write */
size_t PieceService::EncryptAndWritePiece(const unsigned char *key, const unsigned char *iv, const wxString& protocol, wxFFile *f, unsigned char *in, size_t inl)
{
	EVP_CIPHER_CTX ctx;
	EVP_EncryptInit(&ctx, EVP_get_cipherbyname(protocol.c_str()), key, iv);

	unsigned char *outBuff = new unsigned char[inl + EVP_CIPHER_block_size(EVP_get_cipherbyname(protocol.c_str()))];
	size_t written = 0;
	int ret = EVP_EncryptUpdate(&ctx, outBuff, (int*)&written, in, inl);
	wxLogVerbose("PieceService::EncryptAndWritePiece: EVP_EncryptUpdate said %d; encrypted %d bytes from %d input", ret, written, inl);
	size_t actuallyWritten = f->Write(outBuff, written);
	wxLogVerbose("PieceService::EncryptAndWritePiece: wrote %d bytes out", actuallyWritten);

	ret = EVP_EncryptFinal(&ctx, outBuff, (int*)&written);
	wxLogVerbose("PieceService::EncryptAndWritePiece: EVP_EncryptFinal said %d; written = %d", ret, written);
	
	if (written > 0)
		actuallyWritten += f->Write(outBuff, written);

	delete [] outBuff;
	return actuallyWritten;
}

enum PieceUnpackingResult PieceService::Unpack(ManifestEntry *manifest)
{
	if (!PieceService::HasAllPieces(manifest))
	{
		return PUR_MISSING_PIECES;
	}

	wxBusyInfo busy("Decrypting and joining files");

	BEnc::Dictionary *pieces = manifest->getPieces();
	BENC_SAFE_CAST(pieces->getDictValue("hashes"), piecelist, List);
	wxASSERT(piecelist && piecelist->getListSize() > 0);

	if (!piecelist)
		return PUR_BAD_MANIFEST;

	/* expand file metadata */
	BEnc::Dictionary *files = manifest->getFiles();

	if (!files)
		return PUR_BAD_MANIFEST;

	BENC_SAFE_CAST(files->getDictValue("names"), fnamelist, List);
	BENC_SAFE_CAST(files->getDictValue("sizes"), fsizelist, List);
	wxASSERT(fnamelist && fsizelist && fnamelist->getListSize() == fsizelist->getListSize());
	wxASSERT(fnamelist && fnamelist->getListSize() > 0);

	/* get key and iv */
	unsigned char *key;
	unsigned char *iv;

	BENC_SAFE_CAST(pieces->getDictValue("keys"), keys, Dictionary);
	BENC_SAFE_CAST(keys->getDictValue("data"), keydata, String);
	BENC_SAFE_CAST(keys->getDictValue("protocol"), encprotocol, String);

	if (!keys || !keydata || !encprotocol)
		return PUR_BAD_MANIFEST;

	if (keys->hasDictValue("iv"))
	{
		BENC_SAFE_CAST(keys->getDictValue("iv"), ivdata, String);
		iv = (unsigned char *) ivdata->getString().GetData();
	} else {
		iv = NULL;
	}

	key = (unsigned char *) keydata->getString().GetData();
	
	int fileidx = 0, pieceidx = 0;
	size_t foffset = 0;
	size_t baseoffset = 0;

	/* select and load first piece */
	BENC_SAFE_CAST(piecelist->getListItem(pieceidx), currpiece, String);
	wxLogVerbose("PieceService::Unpack: Decrypting piece %d/%d - %s", pieceidx+1, piecelist->getListSize(), CryptoHelper::HashToHex(currpiece->getString(), true));
	wxString piecebuffer = PieceService::DecryptPiece(currpiece->getString(), key, iv, encprotocol->getString());

	while (fileidx < fnamelist->getListSize())
	{
		BENC_SAFE_CAST(fnamelist->getListItem(fileidx), filename, String);
		BENC_SAFE_CAST(fsizelist->getListItem(fileidx), filesize, Integer);

		if (!filename || !filesize)
			return PUR_BAD_MANIFEST;

		//TODO: sanity check on incoming filename
		wxString realfilename = CredentialManager::GetCredentialFilename(RAIN::OUTPUT_DIR) + filename->getString();
		wxFileName::Mkdir(wxFileName(realfilename).GetPath(), 0777, wxPATH_MKDIR_FULL);
		realfilename = wxFileName(realfilename).GetFullPath();
		foffset = 0;

		wxLogVerbose("PieceService::Unpack: opening %s for writing", realfilename.c_str());
		wxFFile *file = new wxFFile(realfilename, "wb");

		while (foffset < filesize->getNum())
		{
			size_t needed = filesize->getNum() - foffset;

			if (needed <= piecebuffer.Length())
			{
				// write a piece of the chunk
				foffset += file->Write(piecebuffer.GetData(), needed);
				wxLogVerbose("	PieceService::Unpack: wrote %d bytes to file (partial chunk)", needed);
				piecebuffer = piecebuffer.Mid(needed);
			} else {
				// write the entire chunk
				foffset += file->Write(piecebuffer.GetData(), piecebuffer.Length());
				wxLogVerbose("	PieceService::Unpack: wrote %d bytes to file (entire chunk)", piecebuffer.Length());
				piecebuffer.Empty();
			}

			if (piecebuffer.IsEmpty())
			{
				pieceidx++;

				if (pieceidx >= piecelist->getListSize())
				{
					wxLogVerbose("PieceService::Unpack: Ran out of pieces");
					break;
				}

				BENC_SAFE_CAST(piecelist->getListItem(pieceidx), currpiece, String);
				wxLogVerbose("PieceService::Unpack: Decrypting piece %d/%d - %s", pieceidx+1, piecelist->getListSize(), CryptoHelper::HashToHex(currpiece->getString(), true));
				piecebuffer = PieceService::DecryptPiece(currpiece->getString(), key, iv, encprotocol->getString());
			}
		}

		fileidx++;
		wxLogVerbose("PieceService::Unpack - closing");
		file->Close();
		delete file;
	}

	return PUR_OK;
}

/** returns whether all pieces named in <i>manifest</i> are present
  * in our local piece cache */
bool PieceService::HasAllPieces(ManifestEntry *manifest)
{
	BEnc::Dictionary *pieces = manifest->getPieces();
	BEnc::Value *v = pieces->getDictValue("hashes");
	BENC_SAFE_CAST(v, piecelist, List);

	for (size_t i = 0; i < piecelist->getListSize(); i++)
	{
		v = piecelist->getListItem(i);
		BENC_SAFE_CAST(v, piecename, String);
		if (!wxFileExists(PieceService::GetPieceFileName(CryptoHelper::HashToHex(piecename->getString()))))
			return false;
	}

	return true;
}

#define READ_BLOCK_SIZE 262144

/** returns the raw decrypted content of the piece with raw hash
  * <i>hash</i> */
wxString PieceService::DecryptPiece(const wxString& hash, const unsigned char *key, const unsigned char *iv, const wxString& protocol)
{
	/* init decryptor */
	EVP_CIPHER_CTX ctx;
	EVP_DecryptInit(&ctx, EVP_get_cipherbyname(protocol.c_str()), key, iv);
	size_t blocksize = EVP_CIPHER_block_size(EVP_get_cipherbyname(protocol.c_str()));

	wxString buff;
	wxFFile f;
	f.Open(PieceService::GetPieceFileName(CryptoHelper::HashToHex(hash)), "rb");
	wxASSERT(f.IsOpened());

	while (true)
	{
		wxString b;
		unsigned char *bs = (unsigned char*) b.GetWriteBuf(READ_BLOCK_SIZE);
		size_t read = f.Read(bs, READ_BLOCK_SIZE);
		b.UngetWriteBuf(READ_BLOCK_SIZE);
		wxLogVerbose("PieceService::DecryptPiece: read %d bytes from %s", read, f.GetName().c_str());

		if (read == 0)
			break;

		b.Truncate(read);
		buff.Append(b);
	}

	wxLogVerbose("PieceService::DecryptPiece: read %d bytes from %s", buff.Length(), f.GetName().c_str());
	f.Close();

	wxString decbuff, ret;
	int declen = 0;
	unsigned char *buffc = (unsigned char *) decbuff.GetWriteBuf(buff.Length() + blocksize);
	int decupdate = EVP_DecryptUpdate(&ctx, buffc, &declen, (const unsigned char*) buff.GetData(), buff.Length());
	decbuff.UngetWriteBuf(buff.Length() + blocksize);
	decbuff.Truncate(declen);
	ret.Append(decbuff);
	wxLogVerbose("PieceService::DecryptPiece: EVP_DecryptUpdate = %d and reported %d bytes decrypted", decupdate, declen);

    buffc = (unsigned char *) decbuff.GetWriteBuf(blocksize);
	decupdate = EVP_DecryptFinal(&ctx, buffc, &declen);
	decbuff.UngetWriteBuf(blocksize);
	decbuff.Truncate(declen);
	ret.Append(decbuff);

	wxLogVerbose("PieceService::DecryptPiece: Got %d bytes from EVP_DecryptFinal; return = %d", declen, decupdate);
	wxLogVerbose("PieceService::DecryptPiece: Finished and returning %d bytes", ret.Length());

	return ret;
}
