#include "Connection.h"
#include "Message.h"
#include "PeerID.h"
#include "CredentialManager.h"
#include "MessageCore.h"
#include "CryptoHelper.h"
#include "BValue.h"
#include "BDecoder.h"
#include "RainSandboxApp.h"

#include <openssl/err.h>

using namespace RAIN;

extern MessageCore *globalMessageCore;

/* a connection is deemed unconnectable if it fails to connected more
 * than MAX_FAILURE_COUNT times */
#define MAX_FAILURE_COUNT 2

/** constructs a connection for server usage */
Connection::Connection(SSL *ssl)
: failureCount(0)
{
	this->ssl = ssl;
	this->ctx = NULL;
	this->pid = new PeerID(false);
	this->client = false;
	this->isConnected = true;

	if (this->pid->address.Length() == 0)
	{
		/* thanks to 'Joseph Bruni' (brunij@earthlink.net)
		 * Subject: Re: determining incoming connection address using BIOs */
		int sk;

#ifdef __WINDOWS__
		int skaddrlen;
#else
        socklen_t skaddrlen;
#endif

		sockaddr_in skaddr;
		skaddrlen = sizeof(skaddr);

		BIO_get_fd(SSL_get_wbio(this->ssl), &sk);

		getpeername(sk, (sockaddr*) &skaddr, &skaddrlen);

		this->pid->address = wxString::Format("%s", inet_ntoa(skaddr.sin_addr));
	}

	wxLogVerbose("Connection::Connection(server): Connection accepted from %s", this->pid->address.c_str());

	this->ImportCertificate();
}

/** constructs a connection for client usage */
Connection::Connection(SSL_CTX *ctx, const wxString& addr, bool connectNow)
: failureCount(0)
{
	this->ssl = NULL;
	this->ctx = ctx;
	this->pid = new PeerID(false);
	this->client = true;
	this->isConnected = false;
	this->pid->address = wxString(addr);

	if (connectNow)
	{
		this->Connect();

		if (this->isConnected)
			wxLogVerbose("Connection::Connection(client): Connection made to %s", this->pid->address.c_str());
		else
			return;
	}
}

Connection::~Connection()
{
	this->Disconnect(true);
	delete this->pid;
}

/** sets the connection's messagecore */
void Connection::SetMessageCore(MessageCore *core)
{
	this->core = core;
}

void Connection::ImportCertificate()
{
	if (this->ssl)
	{
		X509 *cert = SSL_get_peer_certificate(this->ssl);

		if (!cert)
		{
			wxLogError("Connection::ImportCertificate(): No Peer Certificate");
			return;
		}

		this->pid->SetCertificate(cert);
		X509_free(cert);
	}
}

/** tries to create an SSL connection to the peer
  * 
  * only valid for client connections */
bool Connection::Connect()
{
	BIO *client_bio = NULL;

	if (this->ssl != NULL)
	{
		this->Disconnect();
	}

	wxLogStatus("Trying to connect to %s", this->pid->address.c_str());
	client_bio = BIO_new_connect((char *)this->pid->address.c_str());
	BIO_set_nbio(client_bio, 1);

	// TODO: add timeout here
	int err;
	while ((err = BIO_do_connect(client_bio)) <= 0)
	{
		int shouldretry = BIO_should_retry(client_bio);
		
		if (shouldretry == 0)
		{
			wxLogError("Connection::Connect(): BIO_do_connect() failed for '%s' errorcode %d", this->pid->address.c_str(), err);
			this->isConnected = false;
			BIO_free_all(client_bio);
			this->failureCount++;
			return false;
		} else {
			wxThread::Sleep(100);
		}
	}

	if (!(this->ssl = SSL_new(this->ctx)))
	{
		wxLogError("Connection::Connect(): SSL_new() failed for '%s'", this->pid->address.c_str());
		this->isConnected = false;
		BIO_free_all(client_bio);
		this->failureCount++;
		return false;
	}

	SSL_set_bio(this->ssl, client_bio, client_bio);

	while ((err = SSL_connect(this->ssl)) <= 0)
	{
		if (err != -1)
		{
			wxLogError("Connection::Connect(): SSL_connect() failed for '%s'", this->pid->address.c_str());
			this->isConnected = false;

			/* SSL_free also frees the bio which we successfully attached
			 * to it above */
			SSL_free(this->ssl);
			this->ssl = NULL;
			this->failureCount++;
			return false;
		} else {
			int shouldretry_ssl = SSL_get_error(this->ssl, err);
			
			if (shouldretry_ssl == SSL_ERROR_WANT_READ || shouldretry_ssl == SSL_ERROR_WANT_WRITE)
			{
				wxThread::Sleep(100);
			} else {
				//wxLogError("SSL_connect() failed; wanted something other than read or write (!!!)");
				this->isConnected = false;
				this->failureCount++;
				return false;
			}
		}
	}

	//wxLogVerbose("SSL_connect() succeeded to '%s' with cipher %s", this->pid->address.c_str(), SSL_get_cipher(this->ssl));
	this->isConnected = true;
	this->since = wxDateTime::Now();

	this->ImportCertificate();

	wxASSERT(globalMessageCore);

	Message *msg = new Message();
	msg->status = MSG_STATUS_LOCAL;
	msg->subsystem = -1;
	msg->headers["Destination"] = new BEnc::String("Broadcast");
	msg->headers["Event"] = new BEnc::String("Client-Connection-Made");
	msg->headers["From"] = this->pid->AsBValue();
	globalMessageCore->PushMessage(msg);

	wxLogStatus("Connected to %s", this->pid->address.c_str());

	return true;
}

bool Connection::IsConnectable()
{
	return (this->isConnected || this->failureCount < MAX_FAILURE_COUNT);
}

/** cleanly disconnects the connection and frees the underlying ssl connection */
void Connection::Disconnect(bool silent)
{
	if (this->ssl == NULL)
	{
		this->isConnected = false;
		return;
	} else {
		int shutdown_s = SSL_shutdown(this->ssl);

		/* if ssl_shutdown returns 0, it means the connection is not
		* yet shutdown and ssl_shutdown should be called again to complete it */
		if (shutdown_s == 0)
		{
			SSL_shutdown(this->ssl);
		} else if (shutdown_s == -1) {
			wxLogError("Connection::Disconnect(): SSL_shutdown() failed: fatal connection error");
		}

		this->isConnected = false;

		// could do something with SSL_clear here in the future
		SSL_free(ssl);
		this->ssl = NULL;
	}

	if (!silent)
	{
		Message *msg = new Message();
		msg->status = MSG_STATUS_LOCAL;
		msg->subsystem = -1;
		msg->headers["Destination"] = new BEnc::String("Broadcast");
		msg->headers["Event"] = new BEnc::String("Peer-Disconnected");
		msg->headers["From"] = this->pid->AsBValue();
		globalMessageCore->PushMessage(msg);
	}
}

/** polls the connection for incoming messages */
void Connection::Poll()
{
	const int sizeReadLength = 16384;
	bool readWantsWrite = false;
	ERR_clear_error();

	wxMutexLocker ml(this->incomingBuffer);

	/* check read */
	wxString readBuffer;
	char *buffer = readBuffer.GetWriteBuf(sizeReadLength);
	int read = SSL_read(this->ssl, buffer, sizeReadLength);
	readBuffer.UngetWriteBuf(sizeReadLength);

	if (read == 0)
	{
		/* possible disconnection, let's call disconnect to finish the
		 * disconnection cleanly and/or tidy up the connection */
		//wxLogVerbose("Poll::Read calling disconnect - SSL_read returned 0");
		this->Disconnect();
		return;
	} else if (read < 0) {
		/* the ssl_read failed, but we need to do something to get it
		 * to work - let's see why it failed first */
		int rFailureCode = SSL_get_error(this->ssl, read);

		switch (rFailureCode)
		{
		case SSL_ERROR_ZERO_RETURN:
			/* connection cleanly closed :)
			 * as above, we'll tidy */
			//wxLogVerbose("Poll::Read calling disconnect - SSL_write returned SSL_ERROR_ZERO_RETURN");
			this->Disconnect();
			break;
		case SSL_ERROR_WANT_WRITE:
			readWantsWrite = true;
		case SSL_ERROR_WANT_READ:
			/* these are not fatal errors, we just need to wait for
			 * whatever renegotiation is occuring to finish, or for
			 * the underlying tcp socket to get more data
			 *
			 * the next poll will try again */
			//wxLogVerbose("Poll:Read returned SSL_ERROR_WANT_READ or SSL_ERROR_WANT_WRITE");
			break;
		default:
			/* this is probably SSL_ERROR_SSL or SSL_ERROR_SYSCALL or ...
			 * we'll force a disconnect this end and tidy */
			wxLogVerbose("Connection::Poll::Read calling disconnect - %d = SSL_write returned SSL_ERROR_SSL or SSL_ERROR_SYSCALL: %s", rFailureCode, ERR_error_string(ERR_get_error(), NULL));
			this->Disconnect();
			break;
		}

		/* if the reason for ssl_read's failure was fatal, we're going
		 * be disconnected now
		 *
		 * if it wasn't fatal, we'll try reading again next poll */
	} else { // (read > 0)
		/* read is now the number of bytes we read successfully :)
		 *
		 * hooray!
		 *
		 * we append the bytes we read to the incoming message buffer,
		 * then test the message buffer for validity! it will be cleared
		 * by CheckMessageBuffer if it is unrecoverably malformed or
		 * correct - a Message will be added to the application's
		 * message queue if this is the case! */
		wxLogVerbose("Connection::Poll(): read %d bytes from %s", read, this->pid->address.c_str());
		readBuffer.Truncate(read);
		this->incomingMessageBuffer.Append(readBuffer);
		this->CheckMessageBuffer();
	}
	
	wxMutexLocker locker(outgoingBuffer);
	/* check write */
	if (this->outgoingMessageBuffer.Length() > 0 || readWantsWrite)
	{
		if (!this->ssl)
			return;

		/* we have stuff to write */
		int write = SSL_write(this->ssl, this->outgoingMessageBuffer.c_str(), this->outgoingMessageBuffer.Length());

		if (write == 0)
		{
			/* possible disconnection, let's call disconnect to finish the
			* disconnection cleanly and/or tidy up the connection */
			//wxLogVerbose("Poll::Write calling disconnect - SSL_write returned 0");
			this->Disconnect();
			return;
		} else if (write < 0) {
			/* the ssl_read failed, but we need to do something to get it
			* to work - let's see why it failed first */
			int wFailureCode = SSL_get_error(this->ssl, write);

			switch (wFailureCode)
			{
			case SSL_ERROR_ZERO_RETURN:
				/* connection cleanly closed :)
				* as above, we'll tidy */
				//wxLogVerbose("Poll::Write calling disconnect - SSL_write returned SSL_ERROR_ZERO_RETURN");
				this->Disconnect();
				break;
			case SSL_ERROR_WANT_READ:
			case SSL_ERROR_WANT_WRITE:
				/* these are not fatal errors, we just need to wait for
				* whatever renegotiation is occuring to finish, or for
				* the underlying tcp socket to get more data
				*
				* the next poll will try again */
				break;
			default:
				/* this is probably SSL_ERROR_SSL or SSL_ERROR_SYSCALL or ...
				* we'll force a disconnect this end and tidy */
				wxLogVerbose("Connection::Poll::Write calling disconnect - %d = SSL_write returned SSL_ERROR_SSL or SSL_ERROR_SYSCALL: %s", wFailureCode, ERR_error_string(ERR_get_error(), NULL));
				this->Disconnect();
				break;
			}

			/* if the reason for ssl_write's failure was fatal, we're going
			* be disconnected now
			*
			* if it wasn't fatal, we'll try writing again next poll */
		} else { // (write > 0)
			/* write is now the number of bytes we read successfully :)
			*
			* hooray!
			*
			* we remove 'write' bytes from the front of the
			* outgoingMessageBuffer */
			wxLogVerbose("Connection::Poll(): wrote %d/%d bytes from %s", write, this->outgoingMessageBuffer.Length(), this->pid->address.c_str());
			this->outgoingMessageBuffer = this->outgoingMessageBuffer.Right(this->outgoingMessageBuffer.Length() - write);

			/* then flush the underlying bio */
			wxLogVerbose("Connection::Poll(): flushed after write: %d", BIO_flush(SSL_get_wbio(this->ssl)));
		}
	}
}

/** checks the incoming message buffer, and dispatch()es a message if it
  * is valid */
void Connection::CheckMessageBuffer()
{
	/* this is called every time Poll() gets some valid data, there must
	 * be a better way - but it'll do for now */
	long length = this->incomingMessageBuffer.Length();
	int i = 0;

	if (length == 0)
	{
		/* this should never happen */
		return;
	} else {
		/* possibly, the incoming buffer has a valid message for us!
		 * 
		 * let's see how bdecoder handles what we have */
		BEnc::Decoder decoder;

		wxLogVerbose("Connection::CheckMessageBuffer(): trying to bdecode buffer");

		/* note that verify will not mess up our buffer it doesn't contain
		 * a full valid message */
		decoder.Verify(this->incomingMessageBuffer);
		bool gotMessage = false;

		switch (decoder.errorState)
		{
		case BENC_NOERROR:
			wxLogVerbose("Connection::CheckMessageBuffer(): BENC_NOERROR");
			/* no error */
			gotMessage = true;
			break;
		case BENC_NOERROR_TOOMUCHDATA:
			wxLogVerbose("Connection::CheckMessageBuffer(): BENC_NOERROR_TOOMUCHDATA");
			/* more than one message in our buffer is no problem,
			 * and we can safely extract and push a message into our messagecore */
			gotMessage = true;
			break;
		case BENC_BAD_TOKEN:
			wxLogVerbose("Connection::CheckMessageBuffer(): bad token");
		case BENC_UNKNOWN_ERROR:
			wxLogVerbose("Connection::CheckMessageBuffer(): fatal error");

			/* this basically means we have some fragment of bad data before any
			 * possible good data. we try to find any good data at all, else throw
			 * the whole thing out */
			while (this->incomingMessageBuffer.Length() > 0)
			{
				decoder.Verify(this->incomingMessageBuffer);
				if (decoder.errorState >= BENC_NOERROR)
					break;
				else
					this->incomingMessageBuffer = this->incomingMessageBuffer.Mid(1);
			}

			/* now we either have a trimmed valid buffer, or no buffer whatsoever */
			gotMessage = (decoder.errorState >= BENC_NOERROR);
			break;
		case BENC_TRUNCATED_STREAM:
			/* no problems, we need to do more reads probably */
			wxLogVerbose("Connection::CheckMessageBuffer(): truncated stream");
			gotMessage = false;
			break;
		}

		if (gotMessage)
		{
			decoder.Decode(this->incomingMessageBuffer);
			Message *m = new Message();
			m->status = MSG_STATUS_INCOMING;
			decoder.PopulateHash(m->headers);
			m->ImportHeaders();
			this->core->PushMessage(m);
		}

		/* if we still have stuff to parse, do it again */
		if (gotMessage && this->incomingMessageBuffer.Length() > 0)
			this->CheckMessageBuffer();
	}
}

/** adds the message to the outgoing message queue, which will be flushed the next
  * time Poll() is called, if the connection is open */
bool Connection::SendMessage(Message *msg)
{
	if (!this->isConnected)
		return false;

	// TODO: this isn't threadsafe yet
	wxString msgstr = msg->CanonicalForm();
	wxMutexLocker locker(outgoingBuffer);
	this->outgoingMessageBuffer.Append(msgstr);
	//wxLogVerbose("adding %d bytes to connection buffer", msgstr.Length());
	return true;
}

/** don't use this - debugging only */
bool Connection::SendRawString(const wxString& what)
{
	wxASSERT(0);

	if (this->isConnected)
	{
		SSL_write(this->ssl, what.c_str(), what.Length());
		wxLogVerbose("wrote %d bytes to connection %08x", what.Length(), this->ssl);
		return true;
	} else {
		wxLogError("tried writing %d bytes to a closed connection", what.Length());
		return false;
	}
}

/** gets the number of bytes sent over the underlying connection */
unsigned long Connection::GetBytesSent()
{
	if (this->ssl)
		return BIO_number_written(this->ssl->rbio);
	else
		return 0;
}

/** gets the number of bytes received from the underlying connection */
unsigned long Connection::GetBytesRecv()
{
	if (this->ssl)
		return BIO_number_read(this->ssl->rbio);
	else
		return 0;
}

/** gets the underlying openssl SSL structure */
SSL * Connection::GetSSL()
{
	return this->ssl;
}
