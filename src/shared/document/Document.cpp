/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "Document.h"

#include <Entry.h>

#include "CommonPropertyIDs.h"
#include "Playlist.h"
#include "Property.h"
#include "PropertyObjectFactory.h"

// constructor
Document::Document()
	: RWLocker("document rw lock"),
	  fPlaylist(NULL),
	  fCommandStack(),
	  fCurrentFrame(),
	  fDisplayRange(),
	  fPlaybackRange(),
	  fLoopMode(),

	  fClipSelection(),
	  fPlaylistSelection(),
	  fVideoViewSelection(),

	  fRef(NULL)
{
}

// destructor
Document::~Document()
{
	if (fPlaylist)
		fPlaylist->Release();
}

// SetPlaylist
void
Document::SetPlaylist(::Playlist* playlist)
{
	if (fPlaylist == playlist)
		return;

	if (fPlaylist)
		fPlaylist->Release();

	fPlaylist = playlist;

	if (fPlaylist)
		fPlaylist->Acquire();
}

// SetName
void
Document::SetName(const char* name)
{
	if (!fPlaylist) {
		printf("Document::SetName() - no Playlist set!\n");
		return;
	}

	fPlaylist->SetValue(PROPERTY_NAME, name);
}

// Name
const char*
Document::Name() const
{
	if (!fPlaylist)
		return NULL;

	if (StringProperty* p = dynamic_cast<StringProperty*>(
			fPlaylist->FindProperty(PROPERTY_NAME)))
		return p->Value();

	return NULL;
}

// SetRef
void
Document::SetRef(const entry_ref& ref)
{
	if (!fRef)
		fRef = new (std::nothrow) entry_ref(ref);
	else
		*fRef = ref;
}

// MakeEmpty
void
Document::MakeEmpty()
{
	fCommandStack.MakeEmpty();
	
	fPlaylistSelection.DeselectAll();
	fVideoViewSelection.DeselectAll();
}


