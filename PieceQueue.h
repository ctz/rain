#ifndef __PIECEQUEUE_H
#define __PIECEQUEUE_H

#include <wx/wx.h>
#include <wx/datetime.h>

#include "_wxconfig.h"

#include <vector>

#include "Message.h"

namespace RAIN
{
	class ManifestEntry;

	enum QueuedPieceDirection
	{
		QUEUE_DIR_REQUESTING,
		QUEUE_DIR_DISTRIBUTING
	};

	enum QueueState
	{
		QUEUE_RUNNING,
		QUEUE_STOPPED,
		QUEUE_PAUSED,
	};

	class QueuedPiece
	{
	public:
		wxString hash;
		size_t pieceSize;
		QueuedPieceDirection direction;
		wxString target, status;
		bool heldBack;
	};

	enum WaitingFor
	{
		WAITFOR_PEERCONNECT,
		WAITFOR_PIECESEARCH
	};

	class QueueWait
	{
	public:
		QueueWait(WaitingFor w, const wxString& phash, const wxString& hash);

		WaitingFor wait;
		wxDateTime start;
		wxString pieceHash, hash;

		bool satisfied;
	};

	class SandboxApp;

	namespace GUI
	{
		class QueueWidget;
	};

	class PieceQueue : public RAIN::MessageHandler
	{
	public:
		PieceQueue(RAIN::SandboxApp *app);
		~PieceQueue();
		
		void HandleMessage(RAIN::Message *msg);
		void Tick();
		int GetMessageHandlerID();

		int QueueMissingPieces(ManifestEntry *entry);

		void EmptyQueue();
		void LoadQueue();
		void SaveQueue();

		void SentPiece(const wxString& hash);
		bool RemoveQueues(const wxString& hash, QueuedPieceDirection d);
		void ResetQueues(const wxString& hash, QueuedPieceDirection d);

		void AddWaits(QueuedPiece *p);
		void CheckWaits();
		bool HasWaits(const wxString& hash);
		bool SetWaitsSatisfied(WaitingFor w, const wxString& hash);

		bool InCurrentQueue(QueuedPiece *p);
		void InitiateTransfer(QueuedPiece *q);

	private:
		RAIN::SandboxApp *app;

		std::vector<QueuedPiece*> queue;
		std::vector<QueuedPiece*> current;
		std::vector<QueueWait*> waits;
		QueueState state;
		friend class RAIN::GUI::QueueWidget;

		static const int concurrency = 3;
	};
}

#endif //__PIECEQUEUE_H
