/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <new>
#include <stdlib.h>

#ifdef ALLOCATION_CHECK
#	include "AllocationChecker.h"
#endif
//#include "common_constants.h"
//#include "common_logging.h"
#include "EventQueue.h"
#include "FontManager.h"
#include "PlayerApp.h"
//#include "XMLSupport.h"

using std::nothrow;

// main
int
main(int argc, char** argv)
{
#ifdef ALLOCATION_CHECK
	AllocationChecker::CreateDefault();
#endif

//	init_logging("Player", kPlayerLogSettingsPath, true);

	EventQueue::CreateDefault();
	FontManager::CreateDefault();
//	init_xml();

	srand(time(NULL));

	// go
	{
		PlayerApp app;
		app.Run();
	}

//	uninit_xml();
	FontManager::DeleteDefault();
	EventQueue::DeleteDefault();

#ifdef ALLOCATION_CHECK
	AllocationChecker::DeleteDefault();
#endif

	return 0;
}
