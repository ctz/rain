#ifndef __BENC_TESTS_H
#define __BENC_TESTS_H

#include <wx/wx.h>

#include "TestingFramework.h"

class BEncTests : public RAIN::TestingFramework
{
public:
	void RunTests();

private:
	void RunIntegerTests();
	void RunStringTests();
	void RunListTests();
	void RunDictionaryTests();

	void RunDecoderTests();
};

#endif//__BENC_TESTS_H