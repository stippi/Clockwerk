/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClipLibrary.h"

#include <new>
#include <stdio.h>

#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <String.h>

#include "common.h"

#include "CommonPropertyIDs.h"
#include "Playlist.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "XMLImporter.h"

using std::nothrow;

// constructor
ClipLibrary::ClipLibrary()
	: Observable()
{
}

// destructor
ClipLibrary::~ClipLibrary()
{
	_MakeEmpty();
}

// AddClip
bool
ClipLibrary::AddClip(Clip* clip)
{
	if (!clip)
		return false;

	// prevent adding the same *pointer* twice
	if (HasClip(clip))
		return false;

	bool success = fClips.AddItem((void*)clip);
	if (success)
		Notify();
	else
		printf("ClipLibrary::AddClip() - out of memory!\n");

	return success;
}

// RemoveClip
bool
ClipLibrary::RemoveClip(Clip* clip)
{
	bool success = fClips.RemoveItem((void*)clip);
	if (success)
		Notify();

	return success;
}

// RemoveClip
Clip*
ClipLibrary::RemoveClip(int32 index)
{
	Clip* clip = (Clip*)fClips.RemoveItem(index);
	if (clip)
		Notify();

	return clip;
}

// #pragma mark -

// CountClips
int32
ClipLibrary::CountClips() const
{
	return fClips.CountItems();
}

// HasClip
bool
ClipLibrary::HasClip(Clip* clip) const
{
	return fClips.HasItem((void*)clip);
}

// MakeEmpty
void
ClipLibrary::MakeEmpty()
{
	_MakeEmpty();
	Notify();
}

// ClipAt
Clip*
ClipLibrary::ClipAt(int32 index) const
{
	return (Clip*)fClips.ItemAt(index);
}

// ClipAtFast
Clip*
ClipLibrary::ClipAtFast(int32 index) const
{
	return (Clip*)fClips.ItemAtFast(index);
}

// FindClip
Clip*
ClipLibrary::FindClip(const BString& id) const
{
	// TODO: the clip library should be sorted by clip id!
	int32 count = CountClips();
	for (int32 i = 0; i < count; i++) {
		Clip* clip = ClipAtFast(i);
		if (clip->ID() == id)
			return clip;
	}

//	printf("ClipLibrary::FindClip() - "
//		   "didn't find clip by id %s\n", id.String());
	return NULL;
}

// #pragma mark -

// _MakeEmpty
void
ClipLibrary::_MakeEmpty()
{
	int32 count = CountClips();
	for (int32 i = 0; i < count; i++)
		ClipAtFast(i)->Release();
	fClips.MakeEmpty();
}

