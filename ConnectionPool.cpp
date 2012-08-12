#include "ConnectionPool.h"
#include "RainSandboxApp.h"
#include "CredentialManager.h"
#include "Message.h"
#include "RoutingManager.h"
#include "MessageCore.h"
#include "Connection.h"
#include "PeerID.h"
#include "BValue.h"

#include "_wxconfig.h"

extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;

/** Creates a listener thread which opens an SSL socket
  * and creates connections for each received connection. */
RAIN::ConnectionListener::ConnectionListener(RAIN::ConnectionPool *cp)
: wxThreadHelper()
{
	this->connectionPool = cp;
	wxLogVerbose(wxT("RAIN::ConnectionListener created"));
	this->Create();
}

/** Thread body.  Accepts SSL connections and adds peers as they connect */
wxThread::ExitCode RAIN::ConnectionListener::Entry()
{
	BIO *listener, *client;
	SSL *client_ssl;
	SSL_CTX* ssl_ctx = globalCredentialManager->GetSSLContext();

	wxLogVerbose(wxT("starting up RAIN::ConnectionListener; ssl_ctx = %08x"), ssl_ctx);

	listener = BIO_new_accept(DEFAULT_ACCEPT_STR);

	if (!listener)
	{
		wxLogError("Server socket creation failed");
		return NULL;
	}

	/* set listener to be non blocking - needed for
	 * this thread's proper behaviour with timely exiting 
	 * 
	 * also set BIO_BIND_REUSEADDR */
	// broken in gcc severely
	const char a[] = "a";
	BIO_ctrl(listener, BIO_C_SET_ACCEPT, 1, (void*)a);
	BIO_set_bind_mode(listener, BIO_BIND_REUSEADDR);

	// now bind the port
	if (BIO_do_accept(listener) <= 0)
	{
		wxLogError("Server socket binding failed");
		BIO_free_all(listener);
		return NULL;
	}

	while (true)
	{
		if (this->GetThread()->TestDestroy())
		{
			wxLogVerbose("RAIN::ConnectionListener thread finishing");
			BIO_free_all(listener);
			return NULL;
		} else {
			//wxLogVerbose("RAIN::ConnectionListener thread continuing");
		}

		if (BIO_do_accept(listener) > 0)
		{
			// there's a connection waiting to be accepted
			client = BIO_pop(listener);

			if (!(client_ssl = SSL_new(ssl_ctx)))
			{
				wxLogFatalError("RAIN::ConnectionListener - Error creating new SSL connection from context");
				BIO_free_all(client);
				BIO_free_all(listener);
				return NULL;
			} else {
				SSL_set_bio(client_ssl, client, client);

				int sa_ret = SSL_accept(client_ssl);
				int SSLge = SSL_get_error(client_ssl, sa_ret);
				int runawayGuard = 15;

				while (runawayGuard-- && sa_ret == -1 && (SSLge == SSL_ERROR_SSL || SSLge == SSL_ERROR_WANT_READ || SSLge == SSL_ERROR_WANT_WRITE))
				{
					/* the handshaking occurs in a non-blocking way, the underlying bio 'client'
					 * inherits the listener's non-blockingness */
					sa_ret = SSL_accept(client_ssl);
					SSLge = SSL_get_error(client_ssl, sa_ret);
					wxThread::Sleep(1000);
				}

				if (runawayGuard < 1 || sa_ret <= 0)
				{
					SSL_free(client_ssl);

					if (runawayGuard < 1)
						wxLogError("RAIN::ConnectionListener - Timeout while handshaking for new SSL connection");
					else
						wxLogError("RAIN::ConnectionListener - Error accepting new SSL connection: ssl_accept returned %d; ssl_get_error returned %d", sa_ret, SSLge);
				} else {
					wxLogVerbose("RAIN::ConnectionListener - SSL_accept succeeded");

					RAIN::Connection *nc = new RAIN::Connection(client_ssl);
					this->connectionPool->AddServedPeer(nc);
				}
			}
		}

		wxThread::Sleep(2000);
	}
}

/** Creates the ConnectionPool.  This is a collection of all the
  * current connections, both incoming and outgoing. 
  *
  * \param app The Current Application
  * \param useConnectionCache If true, the connectionpool will attempt to reconnect
  *                           peers connected last time
  */
RAIN::ConnectionPool::ConnectionPool(RAIN::SandboxApp *app)
{
	this->app = app;

	/* ignore useConnectionCache for the moment
	 * but we should reload old connections here */
	this->listener = new RAIN::ConnectionListener(this);
	this->listener->GetThread()->Run();

	wxConfig *cfg = (wxConfig *) wxConfig::Get();
	bool useConnectionCache = (cfg->Read("/UseConnectionCache", (long) 1) == 1);

	if (useConnectionCache)
	{
		long cookie = 0;
		wxString name;
		cfg->SetPath("/ConnectionCache");
		bool cont = cfg->GetFirstEntry(name, cookie);

		while (cont)
		{
			this->ConnectPeer(cfg->Read(name, ""));
			cont = cfg->GetNextEntry(name, cookie);
		}
	}
}

RAIN::ConnectionPool::~ConnectionPool()
{
	delete this->listener;

	wxConfig *cfg = (wxConfig *) wxConfig::Get();
	cfg->DeleteGroup("/ConnectionCache");

	size_t n = 0;

	for (size_t i = 0; i < this->pool.size(); i++)
	{
		/* save open connection */
		if (this->pool.at(i) && this->pool.at(i)->isConnected)
		{
			cfg->Write(wxString::Format("/ConnectionCache/%03d", n), this->pool.at(i)->pid->address);
			n++;
		}

		/* free connection */
		delete this->pool.at(i);
	}

	this->pool.erase(this->pool.begin(), this->pool.end());
}

/** Returns true if the peer <i>hash</i> is connected directly to us
  * \param hash	The hash of the peer in question
  */
bool RAIN::ConnectionPool::IsPeerConnected(const wxString& hash)
{
	wxMutexLocker locker(this->poolMutex);
	for (size_t i = 0; i < this->pool.size(); i++)
	{
		if (this->pool.at(i) && this->pool.at(i)->pid && this->pool.at(i)->pid->HashMatches(hash))
			return true;
	}

	return false;
}

/** As IsPeerConnected but for addresses only */
bool RAIN::ConnectionPool::IsAddressConnected(const wxString& addr)
{
	wxMutexLocker locker(this->poolMutex);
	for (size_t i = 0; i < this->pool.size(); i++)
	{
		if (this->pool.at(i) && this->pool.at(i)->pid && this->pool.at(i)->pid->AddressMatches(addr))
			return true;
	}

	return false;
}

/** Connects a single peer as a client.
  * \param addr The address of the client; either IP address
  *             or DNS name.  This must also specify a port
  *             number
  */
void RAIN::ConnectionPool::ConnectPeer(const wxString& addr)
{
	if (addr.IsEmpty() || this->IsAddressConnected(addr))
	{
		wxLogWarning("Tried to connect to already-connected or empty address");
		return;
	}

	/* we don't connect if in the main thread, because that causes ui problems 
	 * so instead just add and don't connect it. this sounds serious but if we
	 * _need_ the connection it will be connected soon by meshmanager */
	RAIN::Connection* nc = new RAIN::Connection(globalCredentialManager->GetSSLContext(), addr, !::wxIsMainThread());
	nc->SetMessageCore(this->app->msgCore);
	wxMutexLocker locker(this->poolMutex);
	this->pool.push_back(nc);

	wxLogStatus("Added connection to %s", addr.c_str());
}

void RAIN::ConnectionPool::ConnectPeer(RAIN::PeerID *pid)
{
	/* check we're not connecting to ourselves */
	if (globalCredentialManager->GetMyPeerID()->HashMatches(pid->hash))
	{
		wxLogVerbose("RAIN::ConnectionPool: Tried to connect to ourselves - ignored");
		return;
	}

	/* check we're not duplicating connections */
	for (size_t i = 0; i < this->pool.size(); i++)
	{
		if (this->pool.at(i) && this->pool.at(i)->isConnected && this->pool.at(i)->pid->HashMatches(pid->hash))
		{
			wxLogVerbose("RAIN::ConnectionPool: Tried to connect to already connected peer - ignored");
			return;
		}
	}

	/* add default port */
	wxString addr = pid->address;
	if (!addr.Contains(":"))
	{
		addr = wxString::Format("%s:%d", addr.c_str(), DEFAULT_PORT);
	}

	this->ConnectPeer(addr);
}

/** Connects a single peer which was accepted by the ConnectionListener thread */
void RAIN::ConnectionPool::AddServedPeer(Connection *c)
{
	c->SetMessageCore(this->app->msgCore);
	wxMutexLocker locker(this->poolMutex);
	this->pool.push_back(c);
	wxLogVerbose("RAIN::ConnectionPool::AddServedPeer() added server peer from %s; now %d peers", c->pid->address.c_str(), this->pool.size());

	RAIN::Message *msg = new RAIN::Message();
	msg->status = MSG_STATUS_LOCAL;
	msg->subsystem = -1;
	msg->headers["Destination"] = new RAIN::BEnc::String("Broadcast");
	msg->headers["Event"] = new RAIN::BEnc::String("Connection-Accepted");
	msg->headers["From"] = c->pid->AsBValue();
	globalMessageCore->PushMessage(msg);

	wxLogStatus("Connection accepted from %s", c->pid->address.c_str());
}

/** Returns true if any of our connections are currently active (connected) */
bool RAIN::ConnectionPool::AnyConnections()
{
	wxMutexLocker locker(this->poolMutex);
	if (this->pool.size() == 0)
	{
		return false;
	} else {
		for (int i = 0; i < this->pool.size(); i++)
		{
			if (this->pool.at(i)->isConnected)
				return true;
		}
	}

	return false;
}

/** returns the number of connections in the pool
  * 
  * \param onlyConnected	if true, only fully connected peers will be counted */
size_t RAIN::ConnectionPool::Count(bool onlyConnected)
{
	wxMutexLocker locker(this->poolMutex);
	if (onlyConnected)
	{
		size_t c = 0;

		for (unsigned int i = 0; i < this->pool.size(); i++)
		{
			if (this->pool.at(i)->isConnected)
				c++;
		}

		return c;
	} else {
		return this->pool.size();
	}
}

/** returns the connection index <b>i</b> in the pool */
RAIN::Connection * RAIN::ConnectionPool::Get(size_t i)
{
	wxMutexLocker locker(this->poolMutex);
	if (i < this->pool.size())
		return this->pool.at(i);
	else
		return NULL;
}

/** returns the change in bandwidth counters since last call 
  * note wxPoint is used instead of a generic tuple, hackish :( */
wxPoint RAIN::ConnectionPool::GetBandwidthSample()
{	
	wxPoint totals, tmp;

	for (size_t i = 0; i < this->pool.size(); i++)
	{
		if (this->pool.at(i) && this->pool.at(i)->isConnected)
		{
			totals.x += this->pool.at(i)->GetBytesRecv();
			totals.y += this->pool.at(i)->GetBytesSent();
		}
	}

	tmp = totals;
	totals = totals - lastSample;
	lastSample = tmp;
	
	return totals;
}

/** Disconnects all current connections */
void RAIN::ConnectionPool::DisconnectAll()
{
	wxMutexLocker locker(this->poolMutex);
	for (int i = 0; i < this->pool.size(); i++)
	{
		delete this->pool.at(i);
		this->pool.erase(this->pool.begin() + i);
	}
}

/** tries to connect any unconnected connections which
  * have not been deemed to be unconnectable */
void RAIN::ConnectionPool::TryConnections()
{
	std::vector<RAIN::Connection*> todelete;

	wxMutexLocker locker(this->poolMutex);
	for (int i = 0; i < this->pool.size(); i++)
	{
		if (this->pool.at(i) && !this->pool.at(i)->isConnected && this->pool.at(i)->IsConnectable())
		{
			this->pool.at(i)->Connect();
		}

		if (this->pool.at(i) && !this->pool.at(i)->IsConnectable())
			todelete.push_back(this->pool.at(i));;
	}

	while (todelete.size() > 0)
	{
		for (int i = 0; i < this->pool.size(); i++)
		{
			if (todelete.at(0) == this->pool.at(i))
			{
				delete todelete.at(0);
				todelete.erase(todelete.begin());
				this->pool.erase(this->pool.begin() + i);
				break;
			}
		}
	}

}

/** Accepts an outgoing message and sends it through the best
  * connection it currently has.  Note that this WILL NOT create a
  * connection if one does not exist (for direct messages).  Instead
  * it will convert the message to be a routable message and send it
  * on a random connection. 
  *
  * \param msg The RAIN::Message object to be sent
  */
void RAIN::ConnectionPool::DispatchMessage(RAIN::Message *msg)
{
	wxLogVerbose("ConnectionPool::DispatchMessage");
	if (msg->status != MSG_STATUS_OUTGOING)
		return;

	BENC_SAFE_CAST(msg->headers["Destination"], msgdest, String);
	BENC_SAFE_CAST(msg->headers["Event"], msgevent, String);

	wxLogVerbose("ConnectionPool::DispatchMessage with msg for %s - %s", msgdest->getString().c_str(),
		msgevent->getString().c_str());

	if (msgdest->getString().Cmp("Broadcast") == 0)
	{
		this->app->router->ImportBroadcast(msg);

		BEnc::Value *via = msg->headers["Via"];
		BEnc::Dictionary *myval = NULL;
		wxString myhash = globalCredentialManager->GetMyPeerID()->hash;

		wxArrayString alreadyto;

		// we flatten the via to a list of peers already sent-to
		std::vector<BEnc::Value *> children;
		children.push_back(via);

		while (children.size() > 0)
		{
			BENC_SAFE_CAST(children.at(0), v, Dictionary);
			children.erase(children.begin());

			// via is always a dict of dicts
			std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = v->hash.begin();

			while (it != v->hash.end())
			{
				BENC_SAFE_CAST(it->second, h, Dictionary);

				alreadyto.Add(it->first);

				if (RAIN::SafeStringsEq(it->first, myhash))
				{
					myval = h;
				}

				if (h->hash.size() > 0)
				{
					children.push_back(h);
				}

				it++;
			}
		}

		if (myval == NULL)
		{
			/* this is an error, because it means we got a message
			 * without the sender modifying the message to indicate
			 * we were sent it */
			wxLogVerbose("Warning: Message received without our hash in Via");
			return;
		}

		std::vector <RAIN::Connection *> local_sendto;
		int i = 0, n = 0;

		wxLogVerbose("Rebroadcasting message already sent to %d peers", alreadyto.GetCount());

		// TODO: optimise this search
		/* now we search through our connected peers to determine if we can
		 * further distribute this message to peers who haven't yet seen it */
        for (i = 0; i < this->pool.size(); i++)
		{
			if (!this->pool.at(i)->isConnected)
				continue;
			
			wxString fp_hash = this->pool.at(i)->pid->hash;
			bool sendToThisPeer = true;

			/* has this message been seen by this peer? */
			for (n = 0; n < alreadyto.GetCount(); n++)
			{
				if (RAIN::SafeStringsEq(fp_hash, alreadyto[n]))
				{
					/* yes, so don't resend */
					sendToThisPeer = false;
					break;
				}
			}

			if (sendToThisPeer)
			{
				local_sendto.push_back(this->pool.at(i));
			}
		}

		if (local_sendto.size() == 0)
		{
			wxLogVerbose("we cannot further distribute this broadcast");
			/* we have nothing to do */
			return;
		}

		/* prepare the message */
		for (i = 0; i < local_sendto.size(); i++)
		{
			RAIN::Connection *c = local_sendto.at(i);

			/* myval is a pointer into msg's via hash at the node containing our hash */
			myval->hash[c->pid->hash] = new BEnc::Dictionary();
		}

		/* now send it */
		for (i = 0; i < local_sendto.size(); i++)
		{
			wxLogVerbose("dispatching to %d (%s)", i, local_sendto.at(i)->pid->address.c_str());
			local_sendto.at(i)->SendMessage(msg);
		}
	} else if (msgdest->getString().Cmp("Peer") == 0) {
		BENC_SAFE_CAST(msg->headers["To"], msgto, String);
		if (!msgto)
			return;
		wxString to = msgto->getString();

		wxLogVerbose("    peer routing - to %s - searching local connections", CryptoHelper::HashToHex(to, true).c_str());

		for (int i = 0; i < this->pool.size(); i++)
		{
			if (this->pool.at(i) && this->pool.at(i)->isConnected)
			{
				if (RAIN::SafeStringsEq(this->pool.at(i)->pid->hash, to))
				{
					wxLogVerbose("        found connection");
					this->pool.at(i)->SendMessage(msg);
					return;
				}
			}
		}

		/* we're still here :(
		 * this implies we're not connected to the recipient of this message
		 * so we need to route it */
		if (!msg->headers.count("Route-Preload"))
		{
			/* we need to consult our routing manager to see if we know
			 * of a route to this host */
			RoutePath *rp;

			if (rp = this->app->router->GetBestRoute(to))
			{
				rp->PreloadMessage(msg);
			} else {
				wxLogVerbose("Warning: route to %s unknown", CryptoHelper::HashToHex(to, true).c_str());
				msgdest->str = "Broadcast";
				this->DispatchMessage(msg);
				return;
			}
		}
		
		/* walk the route-preload looking for our hash */
		wxString myHash = globalCredentialManager->GetMyPeerID()->hash;
		wxString target = "";
		BENC_SAFE_CAST(msg->headers["Route-Preload"], preload, List);

		for (size_t i = 0; i < preload->getListSize(); i++)
		{
			BENC_SAFE_CAST(preload->getListItem(i), listitem, String);

			if (RAIN::SafeStringsEq(listitem->getString(), myHash))
			{
				if (i + 1 == preload->getListSize())
				{
					/* our target is actually To because we're last in the routelist */
					target = to;
				} else {
					BENC_SAFE_CAST(preload->getListItem(i + 1), nextlistitem, String);
					target = nextlistitem->getString();
				}

				break;
			}
		}

		if (target == "")
		{
			/* this should never happen - we got a message preloaded with a route
			 * not including us, or the message is to nowhere! */
			return;
		} else {
			for (size_t i = 0; i < this->pool.size(); i++)
			{
				if (this->pool.at(i) && this->pool.at(i)->isConnected)
				{
					if (RAIN::SafeStringsEq(this->pool.at(i)->pid->hash, to))
					{
						if (msg->headers.count("Via"))
						{
							BENC_SAFE_CAST(msg->headers["Via"], msgvia, List);
							msgvia->pushList(new BEnc::String(myHash));
						} else {
							BEnc::List *msgvia = new BEnc::List();
							msgvia->pushList(new BEnc::String(myHash));
							msg->headers["Via"] = msgvia;
						}

						this->pool.at(i)->SendMessage(msg);
						return;
					}
				}
			}

			/* we got a message with an old/invalid route - default to broadcast */
			msgdest->str = "Broadcast";
			this->DispatchMessage(msg);
			return;
		}
	} else if (msgdest->getString().Cmp("Random") == 0) {
		if (!this->AnyConnections())
		{
			wxLogVerbose("Warning: Tried to send message to random peer with no connections - ignored");
			return;
		}

		Connection *c = NULL;

		while (c == NULL)
		{
			size_t i = (size_t) this->pool.size() * (float) rand() / (float) RAND_MAX;
			wxLogVerbose("ConnectionPool: chose connection %d/%d for random dest", i, pool.size());

			if (i < this->pool.size())
				c = this->pool.at(i);

			if (!c->isConnected)
				c = NULL;
		}

		wxLogVerbose("Sending random message to connection %08x (%s)", c, c->pid->address.c_str());

		msgdest->str = "Peer";
		msg->headers["To"] = new BEnc::String(c->pid->hash);
		c->SendMessage(msg);
	}
}
