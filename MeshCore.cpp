#include "MeshCore.h"
#include "MessageCore.h"
#include "CredentialManager.h"
#include "Connection.h"
#include "ConnectionPool.h"
#include "PeerID.h"
#include "BValue.h"
#include "SystemIDs.h"

using namespace RAIN;
extern CredentialManager *globalCredentialManager;

/** a meshcore is responsible for trying to maintain the stability of the mesh,
  * and maintaining a list of the peers on the mesh
  * \param mc	the messagecore to be used by this meshcore */
MeshCore::MeshCore(MessageCore *mc)
{
	this->mc = mc;
	this->tickCount = 0;

	this->peerlist.push_back(globalCredentialManager->GetMyPeerID());
}

MeshCore::~MeshCore()
{
	while (this->peerlist.size() > 0)
	{
		delete this->peerlist.at(0);
		this->peerlist.erase(this->peerlist.begin());
	}
}

void MeshCore::Tick()
{
	this->tickCount--;
	
	if (this->tickCount <= 0)
	{
		this->CheckConnections();
		this->tickCount = CHECK_INTERVAL;	
	}
}

void MeshCore::CheckConnections()
{
	int n = this->mc->app->connections->Count(true);

	if (n < MIN_CONNECTIONS)
	{
		int n_all = this->mc->app->connections->Count(false);

		if (n_all > n)
		{
			this->mc->app->connections->TryConnections();
		}

		PeerID *p = this->GetRandomPeer();

		if (p != NULL)
		{
			this->mc->app->connections->ConnectPeer(p);
		}
	}
}

/** this is the callback used by messagecore when a message is received for the
  * meshcore
  *
  * \param msg	pointer to the message received */
void MeshCore::HandleMessage(Message *msg)
{
	BENC_SAFE_CAST(msg->headers["Event"], eventstr, String);

	switch (msg->status)
	{
	case MSG_STATUS_LOCAL:
		if (eventstr && eventstr->getString() == "Client-Connection-Made")
		{
			/* we just made a new connection successfully, so say hello */
			Message *hello = new Message();
			hello->status = MSG_STATUS_OUTGOING;
			hello->subsystem = MESHCORE_ID;
			hello->PrepareBroadcast();
			hello->headers["Event"] = new BEnc::String("Hello");
			hello->headers["Listening-On"] = new BEnc::Integer(DEFAULT_PORT);
			hello->headers["PeerList"] = this->GetPeerList();
			this->mc->PushMessage(hello);
		}

		break;

	case MSG_STATUS_INCOMING:
		if (eventstr && eventstr->getString() == "Hello")
		{
			if (msg->headers.count("PeerList"))
				this->MergePeerList(msg->headers["PeerList"]);

			/* we have received a hello, so we reply with our full peerlist */
			Message *pl = new Message();
			pl->status = MSG_STATUS_OUTGOING;
			pl->subsystem = MESHCORE_ID;
			pl->headers["Event"] = new BEnc::String("Peerlist");
			pl->headers["Destination"] = new BEnc::String("Peer");
			pl->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
			pl->headers["To"] = msg->GetReply();
			pl->headers["PeerList"] = this->GetPeerList();
			this->mc->PushMessage(pl);
		} else if (eventstr && eventstr->getString() == "Goodbye") {
			/* a peer said goodbye, so we unmerge them from our peerlist - they
			 * never actually do this yet */
			BENC_SAFE_CAST(msg->headers["Remote-UID"], remoteuid, String);
			if (remoteuid)
				this->UnmergePeer(remoteuid->getString());
		} else if (eventstr && eventstr->getString() == "Peerlist") {
			/* we got a peerlist from someone, so merge */
			this->MergePeerList(msg->headers["PeerList"]);
		}
	}
}

int MeshCore::GetMessageHandlerID()
{
	return MESHCORE_ID;
}

/** imports the identifiers in message <b>msg</b> into our peerlist 
  *
  * \param msg	the message from which to import identifiers */
void MeshCore::ImportIdentifiers(Message *msg)
{
	if (msg->headers.count("From"))
		this->MergeIdentifier(msg->headers["From"]);

	if (msg->headers.count("To"))
		this->MergeIdentifier(msg->headers["To"]);

	wxLogVerbose("MeshCore::ImportIdentifiers ended");
}

/** merges a single identifier into the peerlist
  * 
  * \param v	the benc value with an encoded peerid */
void MeshCore::MergeIdentifier(BEnc::Value *v)
{
	/* unwrap the peerid */
	PeerID *pid = new PeerID(v);

	/* we always need the hash, and can safely discard
	 * the peerid if we don't have it because it's useless */
	if (pid->hash.IsEmpty())
	{
		delete pid;
		return;
	}
	
	/* see if we have it already */
	PeerID *current = this->GetPeer(pid->hash);

	if (current)
	{
		/* we do, so merge in any extra data from the received id */
		wxLogVerbose("MeshCore::MergeIdentifier: merging %s", CryptoHelper::HashToHex(pid->hash, true).c_str());
		current->Merge(pid);
		delete pid;
	} else {
		/* we don't (this is a new peerid to us) so add it to the peerlist */
		wxLogVerbose("MeshCore::MergeIdentifier: adding %s", CryptoHelper::HashToHex(pid->hash, true).c_str());
		this->peerlist.push_back(pid);
	}
}

/** returns the number of peers known by the peerlist with certain characteristics
  * 
  * \param t		a member of the PEER_TYPE enum
  * \param selfok	if true, this function will include us in the result
  */
int MeshCore::GetPeerCount(PEER_TYPE t, bool selfok)
{
	if (t == PEER_TYPE_ALL)
		return this->peerlist.size();

	std::vector<PeerID*>::iterator it = this->peerlist.begin();
	int total = 0;

	while (it != this->peerlist.end())
	{
		PeerID *pid = *it;

		if (!selfok && pid->HashMatches(globalCredentialManager->GetMyPeerID()->hash))
		{
			it++;
			continue;
		}
		
		switch (t)
		{
			case PEER_TYPE_CONNECTABLE:	if (pid->address != "")
											total++;
										break;

			case PEER_TYPE_KNOWN:		if (pid->certificate != NULL)
											total++;
										break;

			case PEER_TYPE_UNKNOWN:		if (pid->certificate == NULL)
											total++;
										break;
		}

		it++;
	}

	return total;
}

/** similar to getpeercount, except it returns a random peerid with certain
 *  characteristics, or NULL if impossible to fulfill
 *
 *  if selfok is true, this might return our own peerid */
//TODO: rewrite because this is really stupid
PeerID * MeshCore::GetRandomPeer(PEER_TYPE t, bool selfok)
{
	int n = this->GetPeerCount(t, selfok);

	if (n == 0)
		return NULL;

	size_t i = -1;
	bool ok = false;

	while (!ok)
	{
		i = 1 + this->peerlist.size() * float(rand() / RAND_MAX);
		
		if (i < 0 || i >= this->peerlist.size())
			continue;

		switch (t)
		{
			case PEER_TYPE_CONNECTABLE:	ok = (this->peerlist.at(i)->address != "");
										break;

			case PEER_TYPE_KNOWN:		ok = (this->peerlist.at(i)->certificate != NULL);
										break;

			case PEER_TYPE_UNKNOWN:		ok = (this->peerlist.at(i)->certificate == NULL);
										break;
		}

		if (ok && this->peerlist.at(i)->HashMatches(globalCredentialManager->GetMyPeerID()->hash))
			ok = selfok;
	}

	return this->peerlist.at(i);
}

/** returns the hash of a peer that piecequeue can send a piece to */
wxString RAIN::MeshCore::GetPieceTarget()
{
	RAIN::PeerID *p = this->GetRandomPeer(RAIN::PEER_TYPE_CONNECTABLE);

	if (p)
		return p->hash;
	else
		return "";
}

/** imports the given peerlist (usually taken from a reply to a hello message) into the current peerlist
 * incoming peerlist is a hash of hashes:
 *	pkhash => 'addr' => address string
 *	          'cert' => certificate string
 * or
 *	pkhash => 'addr' => address string
 * or
 *	pkhash => 'cert' => certificate string
 * or
 *  pkhash => (empty hash)
 * where the values are not known */
bool MeshCore::MergePeerList(BEnc::Value *v)
{
	/* if the merge was useful, i will be nonzero at the end of this */
	int i = 0;

	// TODO: consolidate with peerid changes

	if (v == NULL)
		return false;

	if (v->type == BENC_VAL_DICT)
	{
		BENC_SAFE_CAST(v, vd, Dictionary);
		std::map <wxString, BEnc::Value*, BEnc::Cmp>::iterator it = vd->hash.begin();

		while (it != vd->hash.end())
		{
			wxString hash = it->first;

			if (hash.Length() == 0)
			{
				it++;
				continue;
			}

			BENC_SAFE_CAST(it->second, t, Dictionary);
			PeerID *pid = this->GetPeer(hash);

			if (pid)
			{
				PEERID_WANTS wants = pid->GetWants();
				
				/* the stored peer wants a certificate (or more) and we have a cert */
				if ((wants == PEERID_WANTS_CERT || wants == PEERID_WANTS_ALL) && t->hasDictValue("cert"))
				{
					BENC_SAFE_CAST(t->getDictValue("cert"), certstr, String);
					wxString x509_str = certstr->getString();
					unsigned char *x509_b = (unsigned char *) x509_str.c_str();
					pid->certificate = d2i_X509(NULL, &x509_b, x509_str.Length());
					i++;
				}

				/* same sort of thing with address */
				if ((wants == PEERID_WANTS_ADDR || wants == PEERID_WANTS_ALL) && t->hasDictValue("addr"))
				{
					BENC_SAFE_CAST(t->getDictValue("addr"), addrstr, String);
					pid->address = addrstr->getString();
					i++;
				}
			} else {
				/* we've never seen this peer hash before, so create a new
				 * peerid for it */
				PeerID *newpid = new PeerID();
				newpid->hash = hash;

				if (t->hasDictValue("addr"))
				{
					BENC_SAFE_CAST(t->getDictValue("addr"), addrstr, String);
					newpid->address = addrstr->getString();
				}

				if (t->hasDictValue("cert"))
				{
					BENC_SAFE_CAST(t->getDictValue("cert"), certstr, String);
					wxString x509_str = certstr->getString();
					unsigned char *x509_b = (unsigned char *) x509_str.c_str();
					newpid->certificate = d2i_X509(NULL, &x509_b, x509_str.Length());
				}

				this->peerlist.push_back(newpid);
				i++;
			}

			it++;
		}
	}

	/* return true if we actually added/merged any new peers */
	return (i > 0);
}

/** removes a peer based on a keyhash 'p', returning true if we did anything */
bool MeshCore::UnmergePeer(const wxString &p)
{
	std::vector<PeerID*>::iterator it = this->peerlist.begin();
	while (it != this->peerlist.end())
	{
		if ((*it)->HashMatches(p))
		{
			delete *it;
			this->peerlist.erase(it);
			return true;
		}

		it++;
	}

	return false;
}

/* returns a benc::value containing our known peers */
BEnc::Value * MeshCore::GetPeerList()
{
	BEnc::Dictionary *root = new BEnc::Dictionary();

	std::vector<PeerID*>::iterator it = this->peerlist.begin();
	while (it != this->peerlist.end())
	{
		PeerID *pid = *it;

		BEnc::Dictionary *child = new BEnc::Dictionary();
		
		if (pid->address != "")
			child->hash["addr"] = new BEnc::String(pid->address);

		if (pid->certificate)
			child->hash["cert"] = new BEnc::String(pid->CertificateAsString());

		root->hash[pid->hash] = child;

		it++;
	}

	return root;
}

/** returns the peerid of the peer with hash <b>hash</b>, or null if unknown */
PeerID * MeshCore::GetPeer(const wxString& hash)
{
	std::vector<PeerID*>::iterator it = this->peerlist.begin();

	while (it != this->peerlist.end())
	{
		PeerID *pid = *it;

		if (pid->HashMatches(hash))
		{
			return pid;
		}

		it++;
	}

	return NULL;
}

/** similar to above, but by index instead of hash */
PeerID * MeshCore::GetPeer(size_t i)
{
	if (i < this->peerlist.size())
		return this->peerlist.at(i);
	else
		return NULL;
}

PeerID * MeshCore::GetPeerByCommonName(const wxString& cn)
{
	std::vector<PeerID*>::iterator it = this->peerlist.begin();

	while (it != this->peerlist.end())
	{
		PeerID *pid = *it;

		if (pid->GetCertificateField(NID_commonName) == cn)
		{
			return pid;
		}

		it++;
	}

	return NULL;
}
