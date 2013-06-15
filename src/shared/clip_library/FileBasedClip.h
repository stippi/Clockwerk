/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef FILE_BASED_CLIP_H
#define FILE_BASED_CLIP_H

#include <Entry.h>

#include "Clip.h"

class FileBasedClip : public Clip {
 public:
	virtual						~FileBasedClip();

	virtual	status_t			SetTo(const ServerObject* other);

	// ServerObject interface
	virtual	bool				IsMetaDataOnly() const;
	virtual	bool				IsExternalData() const;

	virtual	uint32				ChangeToken() const;
									// remember the token if
									// you need to become aware
									// of changes to the file
	// FileBasedClip
	static	Clip*				CreateClip(ServerObjectManager* library,
									const entry_ref* ref, status_t& error,
									bool import = false,
									bool allowLazyLoading = true);

	virtual	status_t			InitCheck() = 0;

			const entry_ref*	Ref() const
									{ return &fRef; }

			void				Reload();

 protected:
	virtual	void				HandleReload() = 0;

								FileBasedClip(const entry_ref* ref);
								FileBasedClip(const char* type,
											  const entry_ref* ref);

	virtual status_t			_Store(const char* className, BMessage& archive);

	static	status_t			_GetStored(const entry_ref& ref,
									const char*& className, BMessage& archive);

	static	status_t			_ImportFile(ServerObjectManager* library,
									entry_ref& ref, BString& serverID);

			entry_ref			fRef;
			uint32				fChangeToken;
};

#endif // FILE_BASED_CLIP_H
