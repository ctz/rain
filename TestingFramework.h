#ifndef __FRAMEWORK_H
#define __FRAMEWORK_H

#include <wx/wx.h>

namespace RAIN
{
	class TestingFramework
	{
	public:
		virtual void RunTests() = 0;
	};

	void RunAllTests();
}

#endif//__FRAMEWORK_H