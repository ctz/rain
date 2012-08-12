#ifndef __PIECESERVICE_H
#define __PIECESERVICE_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/ffile.h>

#include <openssl/evp.h>

#include <vector>
#include <algorithm>

namespace RAIN
{
	namespace BEnc
	{
		class Value;
		class Dictionary;
	};

	class ManifestCreationFile;
	class ManifestEntry;

	class PieceCreationResult
	{
	public:
		size_t pieces;
		size_t pieceSize;
		unsigned long long dataSize;
	};

	enum PieceUnpackingResult
	{
		PUR_MISSING_PIECES = -2,
		PUR_BAD_MANIFEST,
		PUR_OK = 0
	};

	class PieceService
	{
	public:
		static PieceCreationResult Create(std::vector<ManifestCreationFile*> *files, BEnc::Dictionary *pieces);
		static enum PieceUnpackingResult Unpack(ManifestEntry *manifest);

		static bool HasAllPieces(ManifestEntry *manifest);

	private:
		/* used by Create() */
		static size_t EncryptAndWritePiece(const unsigned char *key, const unsigned char *iv, const wxString& protocol, wxFFile *f, unsigned char *in, size_t inl);
		static wxString OpenNextPiece(wxFFile *f);
		static wxString SealPiece(const wxString& filename);
		static wxString GetPieceFileName(const wxString& hexhash);

		/* used by Unpack() */
		static wxString DecryptPiece(const wxString& hash, const unsigned char *key, const unsigned char *iv, const wxString& protocol);
	};
}

#endif //__PIECESERVICE_H
