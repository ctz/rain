#ifndef __NETWORKSERVICES_H
#define __NETWORKSERVICES_H

#include <wx/wx.h>

#include <vector>

#include "Message.h"

namespace RAIN
{
	class NetworkServices : public RAIN::MessageHandler
	{
	public:
		NetworkServices();
		~NetworkServices();
		
		void HandleMessage(RAIN::Message *msg);
		void Tick();
		int GetMessageHandlerID();

		void Ping(const wxString& hash);
		void Traceroute(const wxString& hash);
	};
}

#endif //__NETWORKSERVICES_H
