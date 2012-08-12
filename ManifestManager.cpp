#include "ManifestManager.h"

#include "BValue.h"
#include "PeerID.h"
#include "Message.h"
#include "MessageCore.h"
#include "CredentialManager.h"
#include "ManifestService.h"
#include "SystemIDs.h"

using namespace RAIN;

extern RAIN::MessageCore *globalMessageCore;
extern RAIN::CredentialManager *globalCredentialManager;

ManifestManager::ManifestManager()
{
	this->tickCount = 0;
}

ManifestManager::~ManifestManager()
{
}

void ManifestManager::HandleMessage(RAIN::Message *msg)
{
	BENC_SAFE_CAST(msg->headers["Event"], msgevent, String);

	switch (msg->status)
	{
		case MSG_STATUS_INCOMING:
			if (msgevent->getString() == "Manifest-Update")
			{
				this->MergeManifests(msg);	
			} else if (msgevent->getString() == "Synchronise") {
				this->CheckSynchronise(msg);	
			}
			break;
		case MSG_STATUS_LOCAL:
			if (msgevent->getString() == "Force-Update")
			{
				this->Synchronise();	
			}
			break;
		default:
			// ignore
			break;
	}
}

void ManifestManager::Tick()
{
	this->tickCount--;
	
	if (this->tickCount <= 0)
	{
		this->Synchronise();
		this->tickCount = SYNC_INTERVAL;	
	}
}

int ManifestManager::GetMessageHandlerID()
{
	return MANIFEST_ID;	
}

/** merges all the manifests in message <b>m</b> into our local manifest store,
 *  unless they already exist */
void ManifestManager::MergeManifests(RAIN::Message *m)
{
	BENC_SAFE_CAST(m->headers["Manifests"], manifests, List);

	if (!manifests)
		return;

	size_t size = manifests->getListSize();

	wxLogVerbose("ManifestManager::MergeManifests with %d manifests", size);

	for (size_t i = 0; i < size; i++)
	{
		BEnc::Value *v = manifests->getListItem(i);
		ManifestEntry e(v);

		if (e.Verify())
		{
			wxLogVerbose("manifest %d verified, saving", i);
			e.Save();
		} else {
			wxLogVerbose("manifest %d bad, ignoring", i);
		}
	}
}

/** sends a message to a random connected peer with our manifest timestamp,
 *  the remote peer should check this against their manifest timestamp.
 *
 *  if they are newer, they are required to send as many of their manifest entries
 *  as they like (minimum of 1, no maximum), which are then merged on our end and
 *  bring us closer up-to-date
 * 
 *  if they are later, they simply reply with a synchronise of their own, and we
 *  do the opposite
 *
 *  if we're the same, they do nothing */
void ManifestManager::Synchronise()
{
	Message *m = new Message();
	m->status = MSG_STATUS_OUTGOING;
	m->subsystem = MANIFEST_ID;
	m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
	m->headers["Destination"] = new BEnc::String("Random");
	m->headers["Event"] = new BEnc::String("Synchronise");
	m->headers["Timestamp"] = new BEnc::Integer(this->getLatestTimestamp());
	globalMessageCore->PushMessage(m);
}

/** returns the manifest directory, after ensuring it exists */
wxString ManifestManager::getManifestDir()
{
	wxString dir(CredentialManager::GetCredentialFilename(RAIN::MANIFEST_DIR));

	if (!wxDirExists(dir))
	{
		wxFileName::Mkdir(dir, 0755, wxPATH_MKDIR_FULL);

		if (!wxDirExists(dir))
		{
			wxLogError("Cannot open/create manifest directory");
			return "";
		}
	}

	return dir;
}

/** returns the latest timestamp of any manifest entry */
time_t ManifestManager::getLatestTimestamp()
{
	time_t latest = 0;
	wxDir dir(this->getManifestDir());
	wxString filename;

	bool cont = dir.GetFirst(&filename, "*.rnm", wxDIR_FILES);

	if (cont)
	{
		wxFileName fn(CredentialManager::GetCredentialFilename(RAIN::MANIFEST_DIR), filename);
		ManifestEntry ent(fn);

		if (latest < ent.getTimestamp())
		{
			latest = ent.getTimestamp();
		}
		
		cont = dir.GetNext(&filename);
	}

	wxLogVerbose("ManifestManager::getLatestTimestamp() returning %ld", latest);
	return latest;
}

/** handles a "Synchronise" message as outlined in the commentry for Synchronise() */
void ManifestManager::CheckSynchronise(RAIN::Message *m)
{
	wxLogVerbose("ManifestManager::CheckSynchronise()");

	BENC_SAFE_CAST(m->headers["Timestamp"], theirtimestamp, Integer);
	if (!theirtimestamp)
		return;

	time_t ours = this->getLatestTimestamp();
	time_t theirs = theirtimestamp->getNum();

	if (ours == theirs)
	{
		/* we're syncd, do nothing */
		wxLogVerbose("ManifestManager::CheckSynchronise() - %d == %d so we are doing no sync", ours, theirs);
		return;
	} else if (ours > theirs) {
		wxLogVerbose("ManifestManager::CheckSynchronise() - %d > %d so we are sending %d manifests", ours, theirs, MANIFEST_CHUNKS);

		/* send a few manifests */
		std::vector<RAIN::ManifestEntry*> v;
		size_t got = this->getManifestsAfter(v, theirs, MANIFEST_CHUNKS);

		/* make a new message */
		Message *msg = new Message();
		msg->status = MSG_STATUS_OUTGOING;
		msg->subsystem = MANIFEST_ID;
		msg->headers["To"] = m->GetReply();
		msg->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
		msg->headers["Destination"] = new BEnc::String("Peer");
		msg->headers["Event"] = new BEnc::String("Manifest-Update");
		BEnc::List *mans = new BEnc::List();
		msg->headers["Manifests"] = mans;

		for (size_t i = 0; i < got; i++)
		{
			mans->list.push_back(v.at(i)->getBenc());
			delete v.at(i);
		}

		globalMessageCore->PushMessage(msg);
	} else if (theirs > ours) {
		wxLogVerbose("ManifestManager::CheckSynchronise() - %d > %d so we are doing sync right now", theirs, ours);
		/* we are out of sync - let's do it right now! */
		this->Synchronise();
	}
}

/** fills <b>v</b> with <b>n</b> ManifestEntry's immediately after time <b>after</b>
 *  and returns how many were put into v */
size_t ManifestManager::getManifestsAfter(std::vector<RAIN::ManifestEntry*>& v, time_t after, int n)
{
	wxLogVerbose("ManifestManager::getManifestsAfter() called: %d manifests after %d", n, after);
	wxDir dir(this->getManifestDir());
	wxString filename;

	bool cont = dir.GetFirst(&filename, "*.rnm", wxDIR_FILES);
	bool enough = false;
	bool lateEnough = false;

	while (cont && !enough)
	{
		wxFileName fn(CredentialManager::GetCredentialFilename(RAIN::MANIFEST_DIR), filename);
		ManifestEntry ent(fn);

		if (!lateEnough && ent.getTimestamp() > after)
		{
			lateEnough = true;
		}

		if (lateEnough && !enough)
		{
			v.push_back(new ManifestEntry(fn));

			if (v.size() >= n && n != -1)
				enough = true;
		}
		
		cont = dir.GetNext(&filename);
	}

	wxLogVerbose("ManifestManager::getManifestsAfter() returning %d manifests", v.size());

	return v.size();
}

/** fills <b>v</b> with all manifests and returns how many were put into v */
size_t ManifestManager::GetAllManifests(std::vector<RAIN::ManifestEntry*>& v)
{
	wxDir dir(this->getManifestDir());
	wxString filename;

	bool cont = dir.GetFirst(&filename, "*.rnm", wxDIR_FILES);

	while (cont)
	{
		wxFileName fn(CredentialManager::GetCredentialFilename(RAIN::MANIFEST_DIR), filename);
		v.push_back(new ManifestEntry(fn));
		
		cont = dir.GetNext(&filename);
	}

	wxLogVerbose("ManifestManager::GetAllManifests() returning %d manifests", v.size());

	return v.size();
}
