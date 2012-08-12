#ifndef _RAINSANDBOXAPP_H
#define _RAINSANDBOXAPP_H

#include <wx/wx.h>
#include <wx/image.h>
#include <wx/dynload.h>
#include <wx/timer.h>
#include <wx/textctrl.h>

#include "_wxconfig.h"

namespace RAIN
{
	class ConnectionPool;
	class MessageCore;
	class MeshCore;
	class RoutingManager;
	class ManifestManager;
	class PieceQueue;
	class PieceManager;
	class NetworkServices;

	namespace GUI
	{
		class ChatMain;
	};

	class SandboxApp : public wxApp, public wxThreadHelper
	{
	public:
		virtual bool OnInit();
		int OnExit();
		void OnFatalException();

		void * Entry();
		void PollCore(wxTimerEvent& event);

		/* internals */
		RAIN::ConnectionPool	*connections;
		RAIN::MessageCore		*msgCore;
		RAIN::MeshCore			*mesh;
		RAIN::RoutingManager	*router;
		RAIN::ManifestManager	*manifest;
		RAIN::PieceQueue		*pieceQueue;
		RAIN::PieceManager		*pieces;
		RAIN::NetworkServices	*networkServices;

		RAIN::GUI::ChatMain		*gui;

		DECLARE_EVENT_TABLE();

	private:
		wxTimer *coreTimer;
		wxSemaphore *stop;
	};
}

enum
{
	ID_SBA_CORETIMER
};

#endif //_RAINSANDBOXAPP_H
