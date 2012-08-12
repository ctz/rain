#ifndef _RAIN_MESSAGE_H
#define _RAIN_MESSAGE_H

#include <wx/wx.h>
#include <wx/hashmap.h>

namespace RAIN
{
	namespace BEnc
	{
		class Value;
	}
}

WX_DECLARE_STRING_HASH_MAP(RAIN::BEnc::Value *, HeaderHash);

enum
{
	MSG_STATUS_UNKNOWN,
	MSG_STATUS_INCOMING,
	MSG_STATUS_OUTGOING,
	MSG_STATUS_LOCAL,
};

namespace RAIN
{
	namespace BEnc
	{
		class Dictionary;
		class List;
		class String;
		class Integer;
	}

	class MessageCore;
	class Message
	{
	public:
		Message();
		Message(RAIN::Message *that);
		Message(wxString *msg, int status);
		~Message();
		void Parse(wxString *hdrs);
		void ImportHeaders();
		void PrepareBroadcast();
		wxString GetFromHash();
		void Debug();
		BEnc::Value * GetReply();
		
		wxString CanonicalForm();

		HeaderHash headers;
		int status;
		int subsystem;
	};

	class MessageHandler
	{
	public:
		virtual void HandleMessage(RAIN::Message *msg) = 0;
		virtual int GetMessageHandlerID() = 0;
		virtual void Tick() = 0;
	};
};

#endif//_RAIN_MESSAGE_H
