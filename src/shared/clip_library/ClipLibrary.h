/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_LIBRARY_H
#define CLIP_LIBRARY_H

#include <List.h>

#include "Observable.h"
#include "RWLocker.h"

class Clip;
class ServerObject;

class ClipLibrary : public Observable,
					public RWLocker {
 public:
								ClipLibrary();
	virtual						~ClipLibrary();

			bool				AddClip(Clip* clip);
			bool				RemoveClip(Clip* clip);
			Clip*				RemoveClip(int32 index);

			int32				CountClips() const;
			bool				HasClip(Clip* clip) const;

			void				MakeEmpty();

			Clip*				ClipAt(int32 index) const;
			Clip*				ClipAtFast(int32 index) const;

			Clip*				FindClip(const BString& id) const;

//			Clip*				InstantiateClip(ServerObject* fromObject);

 private:
			void				_MakeEmpty();

			BList				fClips;
};

#endif // CLIP_LIBRARY_H
