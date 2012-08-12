#include "PieceManager.h"
#include "PieceQueue.h"
#include "CredentialManager.h"
#include "BValue.h"
#include "PeerID.h"
#include "MessageCore.h"
#include "GUI-ChatMain.h"
#include "SystemIDs.h"

using namespace RAIN;
extern CredentialManager *globalCredentialManager;
extern RAIN::MessageCore *globalMessageCore;

// ------------------------------------------

PieceEntry::PieceEntry()
{
	this->size = 0;
}

PieceEntry::~PieceEntry()
{

}

void PieceEntry::UpdateSize()
{
	//todo: use wxstat
	wxFile *f = new wxFile(CredentialManager::GetCredentialFilename(RAIN::PIECE_DIR) + this->hash + ".rnp");

	if (!f->IsOpened())
	{
		this->size = 0;
		return;
	}

	f->SeekEnd();
	this->size = f->Tell();
	f->Close();
	delete f;
}

// ------------------------------------------

PieceManager::PieceManager()
{
	wxLogVerbose("PieceManager::PieceManager()");
	this->UpdateConfig();
	this->UpdateFromFS();
	this->ValidateSizeLimit();
}

PieceManager::~PieceManager()
{
	std::vector<PieceEntry*>::iterator it = this->cache.begin();

	while (it != this->cache.end())
	{
		delete *it;
		it++;
	}

	this->cache.erase(this->cache.begin(), this->cache.end());
}

int PieceManager::GetMessageHandlerID()
{
	return PIECEMAN_ID;
}

void PieceManager::HandleMessage(RAIN::Message *msg)
{
	Message *reply = new Message();

	switch (msg->status)
	{
	case MSG_STATUS_LOCAL:
		if (((BEnc::String*)msg->headers["Event"])->getString() == "Get-Status")
		{
			this->ValidateSizeLimit();
		} else if (((BEnc::String*)msg->headers["Event"])->getString() == "Update") {
			this->UpdateFromFS();
			this->ValidateSizeLimit();
		}

		break;
	case MSG_STATUS_INCOMING:
		BENC_SAFE_CAST(msg->headers["Event"], eventstr, String);

		if (eventstr && eventstr->getString() == "Can-Accept-Piece?")
		{
			this->ValidateSizeLimit();

			reply->status = MSG_STATUS_OUTGOING;
			reply->subsystem = PIECEMAN_ID;

			reply->headers["To"] = msg->GetReply();
			reply->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
			reply->headers["Destination"] = new BEnc::String("Peer");

			if (this->full)
			{
				reply->headers["Event"] = new BEnc::String("Cannot-Accept-Full");
				globalMessageCore->PushMessage(reply);
				return;
			} else {
				BENC_SAFE_CAST(msg->headers["Piece-Hash"], phashstr, String);

				reply->headers["Event"] = new BEnc::String("Give-Piece");
				reply->headers["Piece-Hash"] = new BEnc::String(*phashstr);
				globalMessageCore->PushMessage(reply);
				return;
			}
		} else if (eventstr && eventstr->getString() == "Piece") {
			this->ValidateSizeLimit();

			reply->status = MSG_STATUS_OUTGOING;
			reply->subsystem = PIECEMAN_ID;

			reply->headers["To"] = msg->GetReply();
			reply->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
			reply->headers["Destination"] = new BEnc::String("Peer");

			BENC_SAFE_CAST(msg->headers["Piece-Hash"], phashstr, String);

			reply->headers["Piece-Hash"] = new BEnc::String(*phashstr);
				
			if (this->ImportPiece(msg))
			{
				reply->headers["Event"] = new BEnc::String("Piece-Stored");
			} else {
				reply->headers["Event"] = new BEnc::String("Piece-Ignored");
			}
			
			globalMessageCore->PushMessage(reply);

			Message *localm = new Message();
			localm->status = MSG_STATUS_LOCAL;
			localm->subsystem = PIECEQUEUE_ID;
			localm->headers["Event"] = new BEnc::String("Received-Piece");
			localm->headers["Piece-Hash"] = new BEnc::String(*phashstr);
			globalMessageCore->PushMessage(localm);

			return;
		} else if (eventstr && eventstr->getString() == "Piece-Stored") {
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], phashstr, String);

			reply->status = MSG_STATUS_LOCAL;
			reply->subsystem = PIECEQUEUE_ID;
			reply->headers["Event"] = new BEnc::String("Sent-Piece");
			reply->headers["Piece-Hash"] = new BEnc::String(*phashstr);
			globalMessageCore->PushMessage(reply);
			return;
		} else if (eventstr && eventstr->getString() == "Piece-Ignored") {
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], phashstr, String);

			reply->status = MSG_STATUS_LOCAL;
			reply->subsystem = PIECEQUEUE_ID;
			reply->headers["Event"] = new BEnc::String("Retry-Piece");
			reply->headers["Piece-Hash"] = new BEnc::String(*phashstr);
			globalMessageCore->PushMessage(reply);
			return;
		} else if (eventstr && eventstr->getString() == "Give-Piece") {
			reply->status = MSG_STATUS_OUTGOING;
			reply->subsystem = PIECEMAN_ID;

			reply->headers["To"] = msg->GetReply();
			reply->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
			reply->headers["Destination"] = new BEnc::String("Peer");

			BENC_SAFE_CAST(msg->headers["Piece-Hash"], phashstr, String);

			reply->headers["Piece-Hash"] = new BEnc::String(*phashstr);

			if (this->ExportPiece(reply))
			{
				reply->headers["Event"] = new BEnc::String("Piece");
				globalMessageCore->PushMessage(reply, MSG_PRIORITY_LOW);
				globalMessageCore->app->pieceQueue->SentPiece(phashstr->getString());
				return;
			} else {
				reply->headers["Event"] = new BEnc::String("No-Piece");
				globalMessageCore->PushMessage(reply);
				return;
			}
		} else if (eventstr && eventstr->getString() == "Find-Piece") {
			/* search for their piece in our local store */
			BENC_SAFE_CAST(msg->headers["Piece-Hash"], phashstr, String);

			if (phashstr && this->HasGotPiece(phashstr->getString()))
			{
				/* we have their piece, send a message back to their queue
				 * subsystem so they can begin to request it */
				reply->status = MSG_STATUS_OUTGOING;
				reply->subsystem = PIECEQUEUE_ID;
				reply->headers["Destination"] = new BEnc::String("Peer");
				reply->headers["From"] = globalCredentialManager->GetMyPeerID()->AsBValue();
				reply->headers["To"] = msg->GetReply();
				reply->headers["Event"] = new BEnc::String("Got-Piece");
				reply->headers["Piece-Hash"] = new BEnc::String(phashstr);
				globalMessageCore->PushMessage(reply);
				return;
			} else {
				/* we don't have it, the message system will continue the
				 * broadcast */
			}
		}

		break;
	}

	delete reply;
}

void PieceManager::Tick()
{

}

void PieceManager::UpdateConfig()
{
	wxConfig *cfg = (wxConfig*) wxConfig::Get();

	/* 1gb default */
	long d = 1024;
	cfg->Read("PieceCacheLimit", &d);
	this->cacheLimit = d * 1024 * 1024;

	this->basedir = CredentialManager::GetCredentialFilename(RAIN::PIECE_DIR);

	if (!wxDirExists(this->basedir))
	{
		wxFileName::Mkdir(this->basedir, 0755, wxPATH_MKDIR_FULL);

		if (!wxDirExists(this->basedir))
		{
			wxLogError("Cannot open/create piece directory");
		}
	}
}

void PieceManager::UpdateFromFS()
{
	// empty
	std::vector<PieceEntry*>::iterator it = this->cache.begin();

	while (it != this->cache.end())
	{
		delete *it;
		it++;
	}

	this->cache.erase(this->cache.begin(), this->cache.end());

	wxString filename = wxFindFirstFile(this->basedir + "*.rnp", wxFILE);

	wxLogVerbose("PieceManager::UpdateFromFS() searching for %s", wxString(this->basedir + "*.rnp").c_str());

	while (!filename.IsEmpty())
	{
		PieceEntry *p = new PieceEntry();
		p->hash = filename.Right(44).Left(40);
		p->UpdateSize();
		this->cache.push_back(p);
		
		filename = wxFindNextFile();
	}
}

unsigned long long PieceManager::GetCurrentUsage()
{
	unsigned long long cacheTotal = 0;

	wxLogVerbose("PieceManager::GetCurrentUsage(): %d in cache", this->cache.size());

	for (size_t i = 0; i < this->cache.size(); i++)
	{
		if (this->cache.at(i))
			cacheTotal += this->cache.at(i)->size;
	}
	
	return cacheTotal;
}

void PieceManager::ValidateSizeLimit()
{
	unsigned long long cacheTotal = this->GetCurrentUsage();

	full = (cacheTotal > this->cacheLimit);

	if (full)
	{
		Message *fm = new Message();
		fm->status = MSG_STATUS_LOCAL;
		fm->subsystem = -1;
		fm->headers["Destination"] = new BEnc::String("Broadcast");
		fm->headers["Event"] = new BEnc::String("Piece cache full");
		
		BEnc::Integer *limit = new BEnc::Integer(this->cacheLimit),
			*total = new BEnc::Integer(cacheTotal);

		fm->headers["Limit"] = limit;
		fm->headers["Total"] = total;
		globalMessageCore->PushMessage(fm);
	} else {
		Message *um = new Message();
		um->status = MSG_STATUS_LOCAL;
		um->subsystem = -1;
		um->headers["Destination"] = new BEnc::String("Broadcast");
		um->headers["Event"] = new BEnc::String("Update cache status");
		
		BEnc::Integer *limit = new BEnc::Integer(this->cacheLimit),
			*total = new BEnc::Integer(cacheTotal);

		um->headers["Limit"] = limit;
		um->headers["Total"] = total;
		globalMessageCore->PushMessage(um);
	}
}

bool PieceManager::ImportPiece(RAIN::Message *pm)
{
	this->UpdateFromFS();
	this->ValidateSizeLimit();

	BENC_SAFE_CAST(pm->headers["Piece-Hash"], phashstr, String);
	BENC_SAFE_CAST(pm->headers["Piece-Data"], pdatastr, String);

	wxLogVerbose("Piece-Hash: 0x%08x; Piece-Data: 0x%08x", phashstr, pdatastr);

	if (!phashstr || !pdatastr)
		return false;

	wxString nicehash = CryptoHelper::HashToHex(phashstr->getString());

	if (this->full || nicehash.Length() != 40)
		return false;

	wxFileName fn(this->basedir, nicehash, "rnp");
	wxFFile *f = new wxFFile(fn.GetFullPath(), "wb");
	
	if (f->IsOpened())
	{
		f->Write(pdatastr->getString().GetData(), pdatastr->getString().Length());
		f->SeekEnd();
		wxLogVerbose("Wrote incoming piece %s, size %d bytes", CryptoHelper::HashToHex(phashstr->getString().c_str(), true), f->Tell());
		f->Close();
		delete f;
		return true;
	} else {
		wxLogVerbose("Warning: Cannot write to %s, dropping incoming piece!", fn.GetFullPath().c_str());
		delete f;
		return false;
	}
}

bool PieceManager::ExportPiece(RAIN::Message *pm)
{
	BENC_SAFE_CAST(pm->headers["Piece-Hash"], phashstr, String);

	if (!phashstr)
		return false;

	wxString nicehash = CryptoHelper::HashToHex(phashstr->getString());

	if (this->full || nicehash.Length() != 40)
		return false;

	wxFileName fn(this->basedir, nicehash, "rnp");
	wxFFile *f = new wxFFile(fn.GetFullPath(), "rb");

	if (f->IsOpened())
	{
		wxString buff, b;

		while (!f->Eof())
		{
			wxChar *bc = b.GetWriteBuf(1024);
			size_t read = f->Read(bc, 1024);
			b.UngetWriteBuf(1024);
			b.Truncate(read);
			buff.Append(b);
		}

		if (buff.Length() > 0)
		{
			BEnc::String *data = new BEnc::String(buff);
			pm->headers["Piece-Data"] = data;
			f->Close();
			delete f;

			return true;
		}
	}
	
	f->Close();
	delete f;
	return false;
}

bool PieceManager::HasGotPiece(const wxString& hash)
{
	wxString nicehash = CryptoHelper::HashToHex(hash).Upper();

	for (size_t i = 0; i < this->cache.size(); i++)
	{
		if (this->cache.at(i) && this->cache.at(i)->hash.Upper() == nicehash)
			return true;
	}

	return false;
}
