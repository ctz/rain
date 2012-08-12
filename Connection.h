#ifndef _RAIN_CONNECTION_H
#define _RAIN_CONNECTION_H

#include <wx/wx.h>
#include <wx/datetime.h>
#include <openssl/bio.h>
#include <openssl/err.h>
#include <openssl/rand.h>
#include <openssl/ssl.h>
#include <openssl/x509.h>

#ifndef __WINDOWS__
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <netdb.h>
#endif//__WINDOWS__

namespace RAIN
{
	class MessageCore;
	class Message;
	class PeerID;

	class Connection
	{
	public:
		Connection(SSL *ssl);
		Connection(SSL_CTX *ctx, const wxString& addr, bool connectNow);
		~Connection();

		void SetMessageCore(RAIN::MessageCore *core);

		/* client-only init */
		bool Connect();
		bool IsConnectable();

		/* valid for both client and server connections */
		void Disconnect(bool silent = false);
		void Poll();

		/* message tx */
		bool SendMessage(RAIN::Message *msg);
		bool SendRawString(const wxString& what);

		/* stats */
		unsigned long GetBytesSent();
		unsigned long GetBytesRecv();

		SSL* GetSSL();

		bool client, isConnected;

		void ImportCertificate();
		PeerID *pid;
		wxDateTime since;

	private:
		SSL *ssl;
		SSL_CTX *ctx;
		int failureCount;

		RAIN::MessageCore *core;

		// contains incoming messages read over a number of polls
		wxString incomingMessageBuffer;
		// contains outgoing messages written at any time
		wxString outgoingMessageBuffer;

		wxMutex outgoingBuffer, incomingBuffer;

		// sees if incomingMessageBuffer has a full, valid message
		void CheckMessageBuffer();
	};
}

#endif //_RAIN_CONNECTION_H
