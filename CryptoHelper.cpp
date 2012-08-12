#include "CryptoHelper.h"
#include "Connection.h"

/** Returns the raw SHA1 hash of data */
wxString RAIN::CryptoHelper::SHA1Hash(const wxString& data)
{
	EVP_MD_CTX ctx;
	EVP_DigestInit(&ctx, EVP_sha1());
	EVP_DigestUpdate(&ctx, data.c_str(), data.length());

	unsigned int outlen = 0;
	wxString out;
	out.Alloc(SHA_DIGEST_LENGTH);
	unsigned char *buff = (unsigned char*) out.GetWriteBuf(SHA_DIGEST_LENGTH);
	EVP_DigestFinal(&ctx, buff, &outlen);
	out.UngetWriteBuf(SHA_DIGEST_LENGTH);
	out.Truncate(outlen);
	return out;
}

#define BLOCKSIZE 1024

/** returns the raw SHA1 hash of file <b>filename</b> */
wxString RAIN::CryptoHelper::SHA1HashFile(const wxFileName& filename)
{
	EVP_MD_CTX ctx;
	EVP_DigestInit(&ctx, EVP_sha1());

	wxFFile file(filename.GetFullPath(), "rb");

	if (!file.IsOpened())
		return "";

	wxChar buff[BLOCKSIZE+1];
	size_t read = 0;

	while (!file.Eof())
	{
		read = file.Read(buff, BLOCKSIZE);

		if (read > 0)
			EVP_DigestUpdate(&ctx, buff, read);
	}

	file.Close();

	size_t outlen = 0;
	wxString out;
	out.Alloc(SHA_DIGEST_LENGTH);
	unsigned char *buffer = (unsigned char*) out.GetWriteBuf(SHA_DIGEST_LENGTH);
	EVP_DigestFinal(&ctx, buffer, &outlen);
	out.UngetWriteBuf(SHA_DIGEST_LENGTH);
	out.Truncate(outlen);
	return out;
}

wxString RAIN::CryptoHelper::SHA1HashFiles(std::vector<wxFileName>& filenames)
{
	EVP_MD_CTX ctx;
	EVP_DigestInit(&ctx, EVP_sha1());

	wxChar buff[BLOCKSIZE+1];

	for (int i = 0; i < filenames.size(); i++)
	{
		wxFFile file(filenames.at(i).GetFullPath(), "rb");

		if (!file.IsOpened())
			return "";

		size_t read = 0;

		while (!file.Eof())
		{
			read = file.Read(buff, BLOCKSIZE);

			if (read > 0)
				EVP_DigestUpdate(&ctx, buff, read);
		}

		file.Close();
	}

	size_t outlen = 0;
	wxString out;
	out.Alloc(SHA_DIGEST_LENGTH);
	unsigned char *buffer = (unsigned char*) out.GetWriteBuf(SHA_DIGEST_LENGTH);
	EVP_DigestFinal(&ctx, buffer, &outlen);
	out.UngetWriteBuf(SHA_DIGEST_LENGTH);
	out.Truncate(outlen);
	return out;
}

wxString RAIN::CryptoHelper::HexToHash(const wxString& hexhash)
{
	wxASSERT(hexhash.Length() % 2 == 0);

	wxString hex = hexhash.Upper();

	wxString ret('\0', hexhash.Length()/2);

	for (size_t i = 0; i < hexhash.Length(); i += 2)
	{
		wxChar a = hex.GetChar(i), b = hex.GetChar(i+1);
		int ia, ib;

		if (a >= '0' && a <= '9')
		{
			ia = a - '0';
		} else if (a >= 'A' && a <= 'F') {
			ia = a - 'A' + 10;
		} else {
			wxASSERT(0);
		}

		if (b >= '0' && b <= '9')
		{
			ib = b - '0';
		} else if (b >= 'A' && b <= 'F') {
			ib = b - 'A' + 10;
		} else {
			wxASSERT(0);
		}

		ret.SetChar(i / 2, (char) ia * 16 + ib);
	}

	return ret;
}

/** Converts a raw binary string to its hexadecimal coded equivelent
  * \param addSpaces If true, a - is added every 2 bytes to aid readability */
wxString RAIN::CryptoHelper::HashToHex(const wxString& hash, bool addSpaces)
{
	wxString ret;

	for (int i = 0; i < hash.Length(); i++)
	{
		ret.Append(wxString::Format("%02X", (unsigned char) hash.GetChar(i)));

		if (addSpaces && i % 2 == 1 && i < hash.Length() - 1)
			ret.Append("-");
	}

	return ret;
}

bool RAIN::CryptoHelper::ValidateHexSHA1Hash(const wxString& hash)
{
	if (hash.Length() != 40)
		return false;

	for (size_t i = 0; i < 40; i++)
	{
		if ((hash.GetChar(i) >= 'A' && hash.GetChar(i) <= 'F') ||
			(hash.GetChar(i) >= 'a' && hash.GetChar(i) <= 'f') ||
			(hash.GetChar(i) >= '0' && hash.GetChar(i) <= '9'))
			continue;
		else
			return false;
	}

	return true;
}

/** creates a time and source-unique serial number for a broadcast
  * message
  * \param hash	the hash of the source node (usually us) */
wxString RAIN::CryptoHelper::CreateSerial(const wxString& hash)
{
	return CryptoHelper::SHA1Hash(
		hash + wxString::Format("%ld.%ld", wxDateTime::UNow().GetTicks(),  wxDateTime::UNow().GetMillisecond()));
}

bool RAIN::SafeStringsEq(const wxString& a, const wxString& b)
{
	if (a.Length() != b.Length())
		return false;

	size_t len = a.Length();

	for (size_t i = 0; i < len; i++)
	{
		if (a.GetChar(i) != b.GetChar(i))
			return false;
	}

	return true;
}
