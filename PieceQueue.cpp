#include "PieceQueue.h"

#include "RainSandboxApp.h"
#include "GUI-ChatMain.h"
#include "GUI-QueueWidget.h"
#include "ManifestService.h"
#include "PieceManager.h"
#include "BValue.h"
#include "CryptoHelper.h"
#include "CredentialManager.h"
#include "MessageCore.h"
#include "MeshCore.h"
#include "PeerID.h"
#include "ConnectionPool.h"
#include "SystemIDs.h"

using namespace RAIN;
extern RAIN::CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;

QueueWait::QueueWait(WaitingFor w, const wxString& phash, const wxString& hash)
{
	this->wait = w;
	this->pieceHash = phash;
	this->hash = hash;
	this->satisfied = false;
	this->start = wxDateTime::Now();
}

// ----------------------------------------------------------

PieceQueue::PieceQueue(RAIN::SandboxApp *app)
{
	this->app = app;
	this->state = QUEUE_STOPPED; 
	this->LoadQueue();
}

PieceQueue::~PieceQueue()
{
	this->state = QUEUE_STOPPED; 
	this->SaveQueue();
	this->EmptyQueue();
}

void PieceQueue::HandleMessage(RAIN::Message *msg)
{
	BENC_SAFE_CAST(msg->headers["Event"], msgevent, String);

	if (msg->status == MSG_STATUS_LOCAL)
	{
		if (msgevent->getString() == "Queue-Piece")
		{
			QueuedPiece *q = new QueuedPiece();

			BENC_SAFE_CAST(msg->headers["Piece-Hash"], piecehash, String);
			BENC_SAFE_CAST(msg->headers["Piece-Size"], piecesize, Integer);
			BENC_SAFE_CAST(msg->headers["Piece-Direction"], piecedir, String);

			q->hash = piecehash->getString();
			q->pieceSize = piecesize->getNum();
			q->direction = (piecedir->getString() == "Outgoing") ? QUEUE_DIR_DISTRIBUTING : QUEUE_DIR_REQUESTING;
			this->queue.push_back(q);
			this->app->gui->Queue->Update();
		} else if (msgevent->getString() == "Sent-Piece") {
			/* we successfully sent a piece, so remove the queue item and
			 * update the overlying user-interface */
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], piecehash, String);
			this->RemoveQueues(piecehash->getString(), QUEUE_DIR_DISTRIBUTING);
		} else if (msgevent->getString() == "Received-Piece") {
			/* we successfully received a piece, so ... */
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], piecehash, String);
			this->RemoveQueues(piecehash->getString(), QUEUE_DIR_REQUESTING);
		} else if (msgevent->getString() == "Retry-Piece") {
			/* we tried to send a piece, but the piecemanager had a problem
			 * sending it, so force a retry */
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], piecehash, String);
			this->ResetQueues(piecehash->getString(), QUEUE_DIR_DISTRIBUTING);
		}
	} else if (msg->status == MSG_STATUS_INCOMING) {
		if (msgevent && msgevent->getString() == "Got-Piece")
		{
			/* someone claims to have a piece we searched for, so verify 
			 * that we are waiting for it still and if so, initiate transfer
			 */
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], piecehash, String);

			if (this->SetWaitsSatisfied(WAITFOR_PIECESEARCH, piecehash->getString()))
			{
				/* we are waiting for this and now the waits are satisfied,
				 * transfer will begin*/
				wxLogMessage("PieceQueue::HandleMessage(): Received Got-Piece for a piece we searched for: %s", CryptoHelper::HashToHex(piecehash->getString(), true));

				/* we need to set the QueuedPiece's target to the originator of this
				 * Got-Piece message */
				for (size_t i = 0; i < this->current.size(); i++)
				{
					wxLogVerbose("PieceQueue::HandleMessage(): current[%d] = %s", i, CryptoHelper::HashToHex(current[i]->hash, true));

					if (RAIN::SafeStringsEq(current[i]->hash, piecehash->getString()) && current[i]->direction == QUEUE_DIR_REQUESTING)
					{
						current[i]->target = msg->GetFromHash();
						wxLogVerbose("PieceQueue::HandleMessage(): Set queued piece's target to %s", CryptoHelper::HashToHex(current[i]->target, true).c_str());
						break;
					}
				}
			} else {
				wxLogMessage("PieceQueue::HandleMessage(): Received Got-Piece for a piece we are not searching for");
			}
		}
	}
}

void PieceQueue::Tick()
{
	if (this->state == QUEUE_STOPPED)
	{
		current.erase(current.begin(), current.end());
		return;
	} else if (this->state == QUEUE_PAUSED) {
		return;
	}

	// run queue by moving pieces in the longterm queue into the current queue
	if (this->queue.size() > this->current.size() && this->current.size() < this->concurrency)
	{
		QueuedPiece *p = this->queue.at(this->current.size());
		this->AddWaits(p);
		this->current.push_back(p);
	}

	this->CheckWaits();

	this->app->gui->Queue->Update();
}

int PieceQueue::GetMessageHandlerID()
{
	return PIECEQUEUE_ID;
}

int PieceQueue::QueueMissingPieces(ManifestEntry *entry)
{
	BEnc::Dictionary *pieces = entry->getPieces();
	BENC_SAFE_CAST(pieces->getDictValue("hashes"), piecehashes, List);
	BENC_SAFE_CAST(pieces->getDictValue("size"), piecesizes, Integer);
	int queued = 0;

	for (size_t i = 0; i < piecehashes->getListSize(); i++)
	{
		BENC_SAFE_CAST(piecehashes->getListItem(i), hash, String);

		if (!globalMessageCore->app->pieces->HasGotPiece(hash->getString()))
		{
			QueuedPiece *q = new QueuedPiece;
			q->direction = QUEUE_DIR_REQUESTING;
			q->pieceSize = piecesizes->getNum();
			q->hash = hash->getString();
			q->heldBack = false;
			this->queue.push_back(q);
			queued++;
		}
	}

	if (queued)
		globalMessageCore->app->gui->Queue->Update();

	return queued;
}

void PieceQueue::AddWaits(QueuedPiece *p)
{
	if (p->target == "")
	{
		if (p->direction == QUEUE_DIR_DISTRIBUTING)
		{
			p->target = this->app->mesh->GetPieceTarget();

			if (this->app->connections->IsPeerConnected(p->target))
			{
				p->status = "Ready to send...";
				p->heldBack = false;
			} else {
				PeerID *pid = this->app->mesh->GetPeer(p->target);

				if (pid && pid->HasAddress())
				{
					this->app->connections->ConnectPeer(pid);
					this->waits.push_back(new QueueWait(WAITFOR_PEERCONNECT, p->hash, pid->hash));
					p->status = "Waiting for connection...";
					p->heldBack = false;
				} else {
					p->status = "Waiting for new peer selection...";
					p->heldBack = true;
				}
			}
		} else {
			// send find
			Message *find = new Message();
			find->PrepareBroadcast();
			find->status = MSG_STATUS_OUTGOING;
			find->subsystem = PIECEMAN_ID;
			find->headers["Event"] = new BEnc::String("Find-Piece");
			find->headers["Piece-Hash"] = new BEnc::String(p->hash);
			globalMessageCore->PushMessage(find);

			// add find wait
			this->waits.push_back(new QueueWait(WAITFOR_PIECESEARCH, p->hash, ""));
			p->heldBack = false;
			p->status = "Searching for piece...";
		}
	}
}

void PieceQueue::CheckWaits()
{
	std::vector<size_t> toDelete;

	for (size_t i = 0; i < this->waits.size(); i++)
	{
		if (this->waits[i] && this->waits[i]->satisfied)
		{
			toDelete.push_back(i);
			continue;
		}
	}

	while (toDelete.size() > 0)
	{
		size_t idx = toDelete.at(0);
		toDelete.erase(toDelete.begin());
		delete this->waits.at(idx);
		this->waits.erase(this->waits.begin() + idx);
	}

	for (size_t i = 0; i < this->current.size(); i++)
	{
		if (this->current[i] && !this->HasWaits(this->current[i]->hash) && !this->current[i]->heldBack)
		{
			// piece is ready to send
			this->InitiateTransfer(this->current[i]);
			this->current[i]->heldBack = true;
			this->current[i]->status = (this->current[i]->direction == QUEUE_DIR_DISTRIBUTING) ? "Checking target store..." : "Found. Requesting...";
		} else {
			// piece is waiting on an event
		}
	}
}

void PieceQueue::InitiateTransfer(QueuedPiece *q)
{
	RAIN::Message *m = new RAIN::Message();
	m->status = MSG_STATUS_OUTGOING;
	m->subsystem = PIECEMAN_ID;
	m->headers["To"] = new BEnc::String(q->target);
	m->headers["Destination"] = new BEnc::String("Peer");

	if (q->direction == QUEUE_DIR_REQUESTING)
	{
		m->headers["Event"] = new BEnc::String("Give-Piece");
		wxLogVerbose("Asking for piece %s from %s", CryptoHelper::HashToHex(q->hash, true).c_str(), CryptoHelper::HashToHex(q->target, true).c_str()); 
	} else {
		m->headers["Event"] = new BEnc::String("Can-Accept-Piece?");
		wxLogVerbose("Asking %s to accept %s from us", CryptoHelper::HashToHex(q->target, true).c_str(), CryptoHelper::HashToHex(q->hash, true).c_str());
	}

	m->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
	m->headers["Piece-Hash"] = new BEnc::String(q->hash);

	globalMessageCore->PushMessage(m);
}

void PieceQueue::SentPiece(const wxString& hash)
{
	for (size_t i = 0; i < queue.size(); i++)
	{
		if (queue[i] && RAIN::SafeStringsEq(queue[i]->hash, hash) && queue[i]->direction == QUEUE_DIR_DISTRIBUTING)
			queue[i]->status = "Sending...";
	}
}

bool PieceQueue::RemoveQueues(const wxString& hash, QueuedPieceDirection d)
{
	for (size_t i = 0; i < current.size(); i++)
	{
		if (RAIN::SafeStringsEq(current[i]->hash, hash) && current[i]->direction == d)
		{
			current.erase(current.begin() + i);
			break;
		}
	}

	for (size_t i = 0; i < queue.size(); i++)
	{
		if (RAIN::SafeStringsEq(queue[i]->hash, hash) && queue[i]->direction == d)
		{
			delete queue[i];
			queue.erase(queue.begin() + i);
			this->app->gui->Queue->Update();
			return true;
		}
	}

	this->app->gui->Queue->Update();
	return false;
}

void PieceQueue::ResetQueues(const wxString& hash, QueuedPieceDirection d)
{
	/* remove from current */
	for (size_t i = 0; i < current.size(); i++)
	{
		if (RAIN::SafeStringsEq(current[i]->hash, hash) && current[i]->direction == d)
		{
			current.erase(current.begin() + i);
			break;
		}
	}

	/* reset target */
	for (size_t i = 0; i < queue.size(); i++)
	{
		if (RAIN::SafeStringsEq(queue[i]->hash, hash) && queue[i]->direction == d)
		{
			queue[i]->target = "";
			break;
		}
	}

	/* remove any waits */
	for (size_t i = 0; i < waits.size(); i++)
	{
		if (RAIN::SafeStringsEq(waits[i]->pieceHash, hash))
		{
			delete waits[i];
			waits.erase(waits.begin() + i);
			break;
		}
	}

	this->app->gui->Queue->Update();
}

bool PieceQueue::HasWaits(const wxString& hash)
{
	for (size_t i = 0; i < this->waits.size(); i++)
	{
		if (this->waits[i] && this->waits[i]->pieceHash == hash)
			return true;
	}

	return false;
}

bool PieceQueue::SetWaitsSatisfied(WaitingFor w, const wxString& hash)
{
	for (size_t i = 0; i < this->waits.size(); i++)
	{
		if (this->waits[i] && this->waits[i]->pieceHash == hash && this->waits[i]->wait == w)
		{
			this->waits[i]->satisfied = true;
			return true;
		}
	}

	return false;
}

bool PieceQueue::InCurrentQueue(QueuedPiece *p)
{
	for (size_t i = 0; i < this->current.size(); i++)
	{
		if (this->current[i] == p)
			return true;
	}

	return false;
}

void PieceQueue::EmptyQueue()
{
	while (this->queue.size() > 0)
	{
		delete this->queue.at(0);
		this->queue.erase(this->queue.begin());
	}
}

void PieceQueue::LoadQueue()
{
	this->EmptyQueue();

	wxConfig *cfg = (wxConfig *) wxConfig::Get();

	cfg->SetPath("/Queue/Hashes/");

	size_t entries = cfg->GetNumberOfEntries();
	
	for (size_t i = 0; i < entries; i++)
	{
		QueuedPiece *q = new QueuedPiece();
		wxString hexhash;
		cfg->Read(wxString::Format("/Queue/Hashes/%d", i), &hexhash);
		q->hash = CryptoHelper::HexToHash(hexhash);
		cfg->Read(wxString::Format("/Queue/Directions/%d", i), (long *) &q->direction);
		cfg->Read(wxString::Format("/Queue/Sizes/%d", i), (long *) &q->pieceSize);
		this->queue.push_back(q);
	}

	wxLogVerbose("PieceQueue::LoadQueue(): Loaded %d items into queue", entries);
}

void PieceQueue::SaveQueue()
{
	wxConfig *cfg = (wxConfig *) wxConfig::Get();

	wxLogVerbose("PieceQueue::SaveQueue(): Saving %d infos in piecequeue", this->queue.size());

	cfg->DeleteGroup("/Queue");

	for (size_t i = 0; i < this->queue.size(); i++)
	{
		if (this->queue.at(i))
		{
			cfg->Write(wxString::Format("/Queue/Hashes/%d", i), CryptoHelper::HashToHex(this->queue.at(i)->hash));
			cfg->Write(wxString::Format("/Queue/Directions/%d", i), (long) this->queue.at(i)->direction);
			cfg->Write(wxString::Format("/Queue/Sizes/%d", i), (long) this->queue.at(i)->pieceSize);
		}
	}
}
