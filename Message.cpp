#include "Message.h"
#include "BDecoder.h"
#include "BValue.h"
#include "CredentialManager.h"
#include "PeerID.h"

extern RAIN::CredentialManager *globalCredentialManager;

/** creates a new empty message */
RAIN::Message::Message()
{
	this->status = MSG_STATUS_UNKNOWN;
	this->subsystem = -1;
}

/** creates a new message as a copy of <b>that</b> */
RAIN::Message::Message(RAIN::Message *that)
{
	this->status = that->status;
	this->subsystem = that->subsystem;
	this->headers.clear();

	HeaderHash::iterator it;
	for (it = that->headers.begin(); it != that->headers.end(); ++it)
	{
		this->headers[it->first] = RAIN::BEnc::copy(it->second);
	}
}

/** creates a new message from the contents of <b>msg</b> */
RAIN::Message::Message(wxString *msg, int status)
{
	this->status = status;
	this->subsystem = -1;

	this->Parse(msg);
}

RAIN::Message::~Message()
{
	HeaderHash::iterator it;
	for (it = this->headers.begin(); it != this->headers.end(); ++it)
	{
		delete it->second;
	}

	while (this->headers.size() > 0)
	{
		this->headers.erase(this->headers.begin());
	}
}

/** decodes the message <b>msg</b> (using benc::decoder) */
void RAIN::Message::Parse(wxString *msg)
{
	RAIN::BEnc::Decoder decoder;
	decoder.Verify(*msg);

	if (decoder.errorState >= BENC_NOERROR)
	{
		this->headers.clear();
		decoder.PopulateHash(this->headers);
	} else {
		this->headers.clear();
	}

	this->ImportHeaders();
}

/** sets the message's subsystem from its headers */
void RAIN::Message::ImportHeaders()
{
	this->subsystem = -1;
	if (this->headers.count("Subsystem"))
	{
		BENC_SAFE_CAST(this->headers["Subsystem"], subsys, Integer);
		
		if (subsys)
		{
			this->subsystem = subsys->getNum();
		}
	}
}

/** preparing a message for broadcast is a complex mess, 
  * so this is a helper */
void RAIN::Message::PrepareBroadcast()
{
	this->headers["Destination"] = new BEnc::String("Broadcast");
	this->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
	BEnc::Dictionary *viadict = new BEnc::Dictionary();
	viadict->hash[globalCredentialManager->GetMyPeerID()->hash] = new BEnc::Dictionary();
	this->headers["Via"] = viadict;
	wxString serial = CryptoHelper::CreateSerial(this->GetFromHash());
	this->headers["Serial"] = new BEnc::String(serial);
	wxLogVerbose("Message::PrepareBroadcast(): Set serial to %s", CryptoHelper::HashToHex(serial, true).c_str());
}

wxString RAIN::Message::GetFromHash()
{
	BENC_SAFE_CAST(this->headers["From"], fromdict, Dictionary);

	if (fromdict)
	{
		BENC_SAFE_CAST(fromdict->getDictValue("hash"), fromhash, String);

		if (fromhash)
			return fromhash->getString();
		else
			return "";
	} else {
		return "";
	}
}

/** spams a lot of debugging info about the message */
void RAIN::Message::Debug()
{
	wxLogVerbose("\tdebugging message 0x%08x with %d tuples", this, this->headers.size());
	
	HeaderHash::iterator it;
    for (it = this->headers.begin(); it != this->headers.end(); ++it)
    {
		wxLogVerbose("\t\t%s => ", it->first.c_str());
		it->second->Debug();
	}
}

/** packs this message into a bencoder dict and returns it */
wxString RAIN::Message::CanonicalForm()
{
	wxString buffer;
	if (this->subsystem != -1 && this->headers.count("Subsystem") == 0)
	{
		this->headers["Subsystem"] = new RAIN::BEnc::Integer(this->subsystem);
	}

	buffer.Append('d');
	
	HeaderHash::iterator it;
    for(it = this->headers.begin(); it != this->headers.end(); ++it)
    {
		if (it->second)
		{
			buffer.Append(wxString::Format("%d:", it->first.Length()));
			buffer.Append(it->first);
			it->second->Write(&buffer);
		}
    }

	buffer.Append('e');
	return buffer;
}

RAIN::BEnc::Value * RAIN::Message::GetReply()
{
	BENC_SAFE_CAST(this->headers["From"], fromdict, Dictionary);

	if (fromdict)
	{
		BENC_SAFE_CAST(fromdict->getDictValue("hash"), fromhash, String);
		
		if (fromhash)
			return new BEnc::String(fromhash->getString());
		else
			return NULL;
	}

	return NULL;
}
