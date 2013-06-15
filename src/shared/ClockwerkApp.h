/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLOCKWERK_APP_H
#define CLOCKWERK_APP_H

#include <Application.h>

class Document;
class ServerObjectFactory;
class ServerObjectManager;

class ClockwerkApp : public BApplication {
 public:
								ClockwerkApp(const char* appSig);
	virtual						~ClockwerkApp();

	// BApplication interface
	virtual	void				ArgvReceived(int32 argc, char** argv);

	// ClockwerkApp
			ServerObjectManager* ObjectLibrary() const
									{ return fObjectLibrary; }
			ServerObjectManager* ObjectManager() const
									{ return fObjectLibrary; }
			ServerObjectFactory* ObjectFactory() const
									{ return fObjectFactory; }

 protected:
			status_t			_ReInitClipsFromDisk(
									bool lockEntireOperation = true,
									bool loadRemovedObjects = true);
			void				_ValidatePlaylistLayouts();

			ServerObjectManager* fObjectLibrary;
			ServerObjectFactory* fObjectFactory;

			Document*			fDocument;

};

#endif // CLOCKWERK_APP_H
