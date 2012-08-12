#ifndef _RAIN_MESSAGECORE_H
#define _RAIN_MESSAGECORE_H

#include <wx/wx.h>
#include <map>
#include <vector>

#include "RainSandboxApp.h"

#define QUIETNESS_THRESHOLD 5

namespace RAIN
{
	class SandboxApp;
	class Message;
	class MessageHandler;
	class MeshCore;

	enum MSG_PRIORITY
	{
		MSG_PRIORITY_NORMAL,
		MSG_PRIORITY_LOW
	};

	class MessageCore
	{
	public:
		MessageCore(RAIN::SandboxApp *app);
		~MessageCore();

		void PushMessage(RAIN::Message *msg, MSG_PRIORITY priority = MSG_PRIORITY_NORMAL);
		void Pump();
		void HandleMessage(RAIN::Message *msg);

		/* registering message handlers */
		bool RegisterMessageHandler(RAIN::MessageHandler *hdlr);

		bool HasSeenSerial(const wxString &ser);

		RAIN::SandboxApp *app;

	private:
		int quietness;
		wxMutex messageQueueMutex;
		wxMutex messagelpQueueMutex;
		std::vector<RAIN::Message*> queue;
		std::vector<RAIN::Message*> lpQueue;
		std::vector<wxString> seenSerials;
		std::map<int, RAIN::MessageHandler*> handlers;
	};
};

#endif//_RAIN_MESSAGECORE_H
