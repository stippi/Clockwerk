/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

//#include <cryptlib.h>
#include <stdlib.h>

//#include "AllocationChecker.h"
//#include "common_constants.h"
//#include "common_logging.h"
#include "EditorApp.h"
#include "EventQueue.h"
#include "FontManager.h"
//#include "XMLSupport.h"

// main
int
main(int argc, char** argv)
{
//	init_logging("Clockwerk", kClockwerkLogSettingsPath, true);

//	AllocationChecker::CreateDefault(true);
	EventQueue::CreateDefault();
	FontManager::CreateDefault();

	// init cryptlib
//	if (cryptInit() != CRYPT_OK) {
//		fprintf(stderr, "Failed to init cryptlib!\n");
//		return 1;
//	}

//	init_xml();

	srand(time(NULL));

	// go
	EditorApp app;
	app.Run();

//	uninit_xml();

//	cryptEnd();

	FontManager::DeleteDefault();
	EventQueue::DeleteDefault();
//	AllocationChecker::DeleteDefault();

	return 0;
}
