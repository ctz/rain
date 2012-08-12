#ifndef __ROUTINGMANAGER_H
#define __ROUTINGMANAGER_H

#include <wx/wx.h>

#include <vector>
#include <map>

#include "BValue.h"

namespace RAIN
{
	class PeerID;
	class Message;

	class RoutePath
	{
	public:
		RoutePath(const wxString& target, const wxArrayString& path);
		
		size_t GetPathLength() const;
		void PreloadMessage(RAIN::Message *m);

		static bool IsValidRoute(const wxString& target, const wxArrayString& path);
		void DumpRoute();

		wxString target;

	private:
		wxArrayString path;
	};

	class RoutingManager
	{
	public:
		RoutingManager();
		~RoutingManager();

		void ImportBroadcast(RAIN::Message *m);

		void DumpKnownRoutes();

		bool HasRoute(const wxString& target);
		RAIN::RoutePath * GetBestRoute(const wxString& target);

		/* path discovery helpers */
		static BEnc::Value * findParentNode(BEnc::Value *v, const wxString& key);
		static bool nodeContainsKey(BEnc::Value *v, const wxString& key);
		static wxArrayString pathFromMeToDist(BEnc::Value *p, BEnc::Value *dist, BEnc::Value *via, wxArrayString& path);

	private:
		std::vector<RAIN::RoutePath*> knownRoutes;
	};
}

#endif//__ROUTINGMANAGER_H
