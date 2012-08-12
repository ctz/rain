#ifndef __BDECODER_H
#define __BDECODER_H

#include <wx/wx.h>

enum
{
	// non fatal/success
	BENC_NOERROR_TOOMUCHDATA = 2,
	BENC_NOERROR = 1,
	// who knows what went wrong
	BENC_UNKNOWN_ERROR = 0,
	// non-fatal (perhaps transitory for truncated streams)
	BENC_TRUNCATED_STREAM = -1,
	// fatal (some kind of corruption)
	BENC_BAD_TOKEN = -2,
};

#include "BValue.h"
#include "Message.h"

namespace RAIN
{
	namespace BEnc
	{
		class Decoder
		{
		public:
			Decoder();
			~Decoder();

			int DoDecode(wxString &str);
			int TryDecode(const wxString &str);

			void Decode(wxString &str);
			void Verify(const wxString &str);

			void PopulateHash(HeaderHash& hash);
			BEnc::Value * getRoot();

			int errorState;

		private:

			BEnc::Dictionary* root;
		};
	}
}

#endif//__BDECODER_H
