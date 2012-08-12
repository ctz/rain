#ifndef _RAIN_CONNECTIONPOOL_H
#define _RAIN_CONNECTIONPOOL_H

#include <wx/wx.h>
#include <wx/thread.h>
#include <wx/arrstr.h>
#include <wx/tokenzr.h>
#include <openssl/ssl.h>
#include <vector>

#define DEFAULT_PORT 1322
// openssl likes ports as a string
#define DEFAULT_PORT_STR "1322"
// change the * to a local ip to restrict server binding
#define DEFAULT_ACCEPT_STR "*:1322"

namespace RAIN
{
	class Connection;
	class ConnectionPool;
	class SandboxApp;
	class Message;
	class PeerID;

	class ConnectionListener : public wxThreadHelper
	{
	public:
		ConnectionListener(RAIN::ConnectionPool *cp);

		wxThread::ExitCode Entry();
	private:
		RAIN::ConnectionPool *connectionPool;
	};

	class ConnectionPool
	{
	public:
		ConnectionPool(RAIN::SandboxApp *app);
		~ConnectionPool();

		void ConnectPeer(const wxString& addr);
		void ConnectPeer(RAIN::PeerID *pid);
		void DispatchMessage(RAIN::Message *msg);

		void DisconnectAll();
		void TryConnections();
		bool AnyConnections();
		wxPoint GetBandwidthSample();

		bool IsPeerConnected(const wxString& hash);
		bool IsAddressConnected(const wxString& addr);

		size_t Count(bool onlyConnected = true);
		RAIN::Connection * Get(size_t i);

		RAIN::SandboxApp *app;
		
	private:
		void AddServedPeer(Connection* c);

		RAIN::ConnectionListener *listener;
		friend class RAIN::ConnectionListener;
		friend class RAIN::SandboxApp;

		wxPoint lastSample;
		wxMutex poolMutex;
		std::vector<RAIN::Connection*> pool;
	};
};

#endif //_RAIN_CONNECTIONPOOL_H
