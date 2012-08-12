#include "RoutingManager.h"
#include "Message.h"
#include "PeerID.h"
#include "BValue.h"
#include "CredentialManager.h"
#include "CryptoHelper.h"

using namespace RAIN;
extern RAIN::CredentialManager *globalCredentialManager;

/** a routepath stores a single possible path to another node
  * 
  * \param target the eventual destination of this path 
  * \param path a list of nodes comprising the path
  */
RoutePath::RoutePath(const wxString& target, const wxArrayString& path)
{
	this->target = target;
	this->path = path;
}

size_t RoutePath::GetPathLength() const
{
	return this->path.GetCount();
}

void RoutePath::PreloadMessage(RAIN::Message *m)
{
	m->headers["To"] = new BEnc::String(this->target);
	BEnc::List *preload = new BEnc::List();
	m->headers["Route-Preload"] = preload;

	for (size_t i = 0; i < this->path.GetCount(); i++)
	{
		preload->pushList(new BEnc::String(this->path.Item(i)));
	}
}

void RoutePath::DumpRoute()
{
	wxLogVerbose("route to %s (%s) -> ", CryptoHelper::HashToHex(this->target, true).c_str(), this->target.c_str());

	for (size_t i = 0; i < this->path.GetCount(); i++)
	{
		wxLogVerbose("\t%s (%s), ", CryptoHelper::HashToHex(this->path.Item(i), true).c_str(), this->path.Item(i).c_str());
	}
}

bool RoutePath::IsValidRoute(const wxString& target, const wxArrayString& path)
{
	/* the route is invalid if the target is contained within the path somewhere */
	if (path.GetCount() == 0)
		return false;

	for (size_t i = 0; i < path.GetCount(); i++)
	{
		if (SafeStringsEq(path.Item(i), target))
			return false;
	}

	return true;
}

/** the routing manager is passed all seen broadcast messages, and forms
  * a conclusive list of paths to each node. these can be consulted to
  * preload a route into a message */
RoutingManager::RoutingManager()
{
}

/** tidies up all the known routes */
RoutingManager::~RoutingManager()
{
	for (size_t i = 0; i < this->knownRoutes.size(); i++)
	{
		delete this->knownRoutes.at(i);
	}
}

/** given a certain path from a message <b>v</b>, this returns the "distributor"
  * node of the node with hash <b>key</b>
  * \param v	pointer to the path from a received message 
  * \param key	hash of the required node */
BEnc::Value * RoutingManager::findParentNode(BEnc::Value *v, const wxString& key)
{
	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it;
	std::vector<BEnc::Value *> roots;

	roots.push_back(v);

	while (roots.size() > 0)
	{
		BENC_SAFE_CAST(roots.at(0), n, Dictionary);
		roots.erase(roots.begin());
		it = n->hash.begin();

		while (it != n->hash.end())
		{
			it->second->key = it->first;
			BENC_SAFE_CAST(it->second, itdict, Dictionary);

			if (SafeStringsEq(it->first, key))
			{
				return n;
			} else if (itdict->hash.size() > 0) {
				roots.push_back(it->second);
			}

			it++;
		}
	}

	return NULL;
}

/** returns true if the route path <b>v</b> contains the hash <b>key</b> */
bool RoutingManager::nodeContainsKey(BEnc::Value *v, const wxString& key)
{
	BENC_SAFE_CAST(v, vd, Dictionary);

	if (!vd)
		return false;

	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it = vd->hash.begin();

	while (it != vd->hash.end())
	{
		if (SafeStringsEq(it->first, key))
		{
			return true;
		}

		it++;
	}

	return false;
}

/** returns a wxArrayString containing the path (in terms of a list of hashes)
  * from us to a certain distributor
  * 
  * \param p	the start node
  * \param dist	the required distributors
  * \param via	the entire list, from the path of a message
  * \param path	the list of the nodes traverse so far */
wxArrayString RoutingManager::pathFromMeToDist(BEnc::Value *p, BEnc::Value *dist,
	BEnc::Value *via, wxArrayString& path)
{
	if (p == dist)
	{
		/* we are at the distributor, so add the distributor to the path and return */
		path.Add(p->key);
		return path;
	} else {
		/* we're above the distributor, so add this node as a via and descend */
		path.Add(p->key);
		
		BEnc::Value *newp = findParentNode(via, p->key);
		path = pathFromMeToDist(newp, dist, via, path);
		return path;
	}
}

/** this inspects the passed broadcast message and derives what route
  * the message took and stores it for future reference
  *
  * \param m A broadcast message
  */
void RoutingManager::ImportBroadcast(RAIN::Message *m)
{
	BEnc::Value *via = m->headers["Via"];

	if (via == NULL || via->type != BENC_VAL_DICT)
		return;

	/* here is how this works:
	 * 
	 * 1) we find ourselves somewhere near the end of the via tree
	 * 2) we store the routes to peers at the same level of the tree
	 * 3) we find the parent of this level, then go back to 1
	 */
	wxString myHash = globalCredentialManager->GetMyPeerID()->hash;
	BEnc::Value *me = NULL, *myParent = NULL;
	std::map<wxString, RAIN::BEnc::Value*, BEnc::Cmp>::iterator it;

	std::vector<BEnc::Value *> roots, distributors;
	roots.push_back(via);
	via->key = "";

	while (roots.size() > 0)
	{
		BENC_SAFE_CAST(roots.at(0), n, Dictionary);
		roots.erase(roots.begin());
		it = n->hash.begin();

		while (it != n->hash.end())
		{
			it->second->key = it->first;
			BENC_SAFE_CAST(it->second, itdict, Dictionary);

			if (SafeStringsEq(it->first, myHash))
			{
				me = it->second;
				myParent = n;
			} else if (itdict->hash.size() > 0) {
				roots.push_back(it->second);
				distributors.push_back(it->second);
			}

			it++;
		}
	}

	for (size_t i = 0; i < distributors.size(); i++)
	{
		wxLogVerbose("dist %d: %s", i, distributors.at(i)->key.c_str());
	}

	/* now look through the distributing nodes and find paths from us
	 * to them and then from them to each target */
	for (size_t i = 0; i < distributors.size(); i++)
	{
		/* from us to them */
		wxArrayString dummy;
		wxArrayString path = pathFromMeToDist((BEnc::Value*)myParent, (BEnc::Value*)distributors.at(i), (BEnc::Value*)via, dummy);

		BENC_SAFE_CAST(distributors.at(i), thisdict, Dictionary);
		it = thisdict->hash.begin();

		while (it != thisdict->hash.end())
		{
			if (!SafeStringsEq(it->first, myHash))
			{
				if (RoutePath::IsValidRoute(it->first, path))
				{
					RoutePath *rp = new RoutePath(it->first, path);
					this->knownRoutes.push_back(rp);
				}
			}

			it++;
		}
	}
}

/** returns true if the routingmanager knows of a route to this node
  * 
  * \param target the public key hash of the node
  */
bool RoutingManager::HasRoute(const wxString& target)
{
	return (this->GetBestRoute(target) != NULL);
}

/** a simple debugging aid - dumps the routing table */
void RoutingManager::DumpKnownRoutes()
{
	for (size_t i = 0; i < this->knownRoutes.size(); i++)
	{
		this->knownRoutes.at(i)->DumpRoute();
	}
}

/** returns the best routepath to this node, if known. NULL otherwise
  * 
  * \param target the public key hash of the node
  */
RoutePath * RoutingManager::GetBestRoute(const wxString& target)
{
	RoutePath *best = NULL;

	for (size_t i = 0; i < this->knownRoutes.size(); i++)
	{
		if (SafeStringsEq(this->knownRoutes.at(i)->target, target))
		{
			if (best == NULL || best->GetPathLength() > this->knownRoutes.at(i)->GetPathLength())
			{
				best = this->knownRoutes.at(i);
			}
		}
	}

	return best;
}
