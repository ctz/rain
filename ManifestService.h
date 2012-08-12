#ifndef __MANIFESTSERVICE_H
#define __MANIFESTSERVICE_H

#include <wx/wx.h>
#include <wx/filename.h>
#include <wx/file.h>
#include <wx/ffile.h>

#include <vector>
#include <algorithm>

#include "Message.h"

namespace RAIN
{
	namespace BEnc
	{
		class Dictionary;
	};

	/** an interface to a single manifest entry, stored in the manifest cache */
	class ManifestEntry
	{
	public:
		// benc constructor
		ManifestEntry(BEnc::Value *v);
		// load constructor
		ManifestEntry(wxFileName& filename);
		~ManifestEntry();

		void Load(wxFileName& filename);
		bool Verify();
		void Save();

		BEnc::Value * getBenc();

		time_t getTimestamp();
		BEnc::Dictionary * getMeta();
		BEnc::Dictionary * getFiles();
		BEnc::Dictionary * getPieces();

		BEnc::Dictionary *v;
	};

	/** possible errors returned by Create() */
	enum ManifestCreationError
	{
		MCE_ERROR_OK,
		MCE_ERROR_MISSING_FILE,
		MCE_ERROR_BAD_PERMISSIONS,
	};

	enum ManifestSignatureResult
	{
		MSR_ERROR_OK,
		MSR_ERROR_UNSIGNED,
		MSR_ERROR_BADSIGNATURE,
		MSR_ERROR_ALREADYSIGNED,
		MSR_ERROR_BADCREDENTIALS,
	};

	/** returned by Create() */
	class ManifestCreationResult
	{
	public:
		ManifestCreationError	errorCode;
		wxString				error;
		ManifestEntry			*manifest;
	};

	/** a single file given to Create() */
	class ManifestCreationFile
	{
	public:
		ManifestCreationFile(const wxString& filename);
		ManifestCreationFile(const wxArrayString& base, const wxFileName& filename);

		wxString GetTranslatedPath(const wxString& meshbase);

		wxFileName filename;
		wxArrayString prepend;
		unsigned long long fileSize;

	private:
		void UpdateFileSize();
	};

	/** the actual manifestservice class, a few static methods for usage by GUI or CLI
	  * clients */
	class ManifestService
	{
	public:
		static ManifestCreationResult  Create(std::vector<ManifestCreationFile*> *files, const wxString& base);
		static ManifestSignatureResult Sign(ManifestEntry& m);
		static ManifestSignatureResult VerifySignature(ManifestEntry& m);
	};
}

#endif //__MANIFESTSERVICE_H
