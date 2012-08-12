#ifndef __RAIN_CRYPTOHELPER_H
#define __RAIN_CRYPTOHELPER_H

#include <openssl/evp.h>
#include <wx/wx.h>
#include <wx/ffile.h>
#include <wx/filename.h>

#include <vector>

namespace RAIN
{
	bool SafeStringsEq(const wxString& a, const wxString& b);

	class CryptoHelper
	{
	public:
		static wxString SHA1Hash(const wxString& data);
		static wxString SHA1HashFile(const wxFileName& filename);
		static wxString SHA1HashFiles(std::vector<wxFileName>& filenames);

		static wxString HexToHash(const wxString& hex);
		static wxString HashToHex(const wxString& hash, bool addSpaces = false);

		static wxString CreateSerial(const wxString& hash);

		static bool ValidateHexSHA1Hash(const wxString& hash);
	};
};

#endif//__RAIN_CRYPTOHELPER_H
