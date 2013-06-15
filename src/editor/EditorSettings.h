/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EDITOR_SETTINGS_H
#define EDITOR_SETTINGS_H

#include <String.h>

class EditorSettings {
 public:
								EditorSettings();
	virtual						~EditorSettings();

			status_t			Init();


			const char*			Server() const
									{ return fServer.String(); }

			BString				MediaFolder() const
									{ return fMediaFolder; }

			BString				ClientID() const
									{ return fClientID; }

			int32				Revision() const
									{ return fRevision; }

 private:
			BString				fServer;
				// the Java server that runs the database

			BString				fMediaFolder;
				// where to put them locally

			BString				fClientID;
				// the identification of this client

			int32				fRevision;
};

#endif // EDITOR_SETTINGS_H
