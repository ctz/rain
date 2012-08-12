#ifndef _RAIN_MESHCORE_H
#define _RAIN_MESHCORE_H

#include <wx/wx.h>
#include <vector>

#include "Message.h"
#include "BValue.h"

/* the number of ticks to wait between checking connections */
#define CHECK_INTERVAL 128

/* the number of hard connections to keep */
#define MIN_CONNECTIONS 5

namespace RAIN
{
	//fwd refs
	class PeerID;

	enum PEER_TYPE
	{
		PEER_TYPE_ALL,
		PEER_TYPE_CONNECTABLE,
		PEER_TYPE_UNKNOWN,
		PEER_TYPE_KNOWN,
	};

	/** Tries to maintain the mesh.
	 * Collects the peer list from hellos and hello replies, maintains core
	 * connections for mesh stability and answers hello requests
	 */
	class MeshCore : public RAIN::MessageHandler
	{
	public:
		MeshCore(RAIN::MessageCore *mc);
		~MeshCore();

		void HandleMessage(RAIN::Message *msg);
		int GetMessageHandlerID();

		void Tick();
		void CheckConnections();

		void ImportIdentifiers(RAIN::Message *msg);
		void MergeIdentifier(BEnc::Value *v);
		int GetPeerCount(RAIN::PEER_TYPE t = RAIN::PEER_TYPE_ALL, bool selfok = false);
		RAIN::PeerID * GetRandomPeer(PEER_TYPE t = RAIN::PEER_TYPE_CONNECTABLE, bool selfok = false);
		RAIN::PeerID * GetPeer(const wxString& hash);
		RAIN::PeerID * GetPeer(size_t i);
		RAIN::PeerID * GetPeerByCommonName(const wxString& cn);
		wxString GetPieceTarget();

	private:
		off_t tickCount;

		RAIN::MessageCore *mc;

		bool MergePeerList(RAIN::BEnc::Value *v);
		bool UnmergePeer(const wxString &p);
		BEnc::Value * GetPeerList();
		
		std::vector<RAIN::PeerID*> peerlist;
	};
};

#endif//_RAIN_MESHCORE_H
