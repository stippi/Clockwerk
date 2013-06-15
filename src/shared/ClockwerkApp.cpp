/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ClockwerkApp.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <fs_attr.h>

#include <Directory.h>
#include <Entry.h>
#include <File.h>

#include "common.h"

#include "AttributeServerObjectManager.h"
#include "CommonPropertyIDs.h"
#include "Document.h"
#include "Playlist.h"
#include "ClipObjectFactory.h"
#include "XMLImporter.h"


using std::nothrow;

// constructor
ClockwerkApp::ClockwerkApp(const char* appSig)
	: BApplication(appSig)
	, fObjectLibrary(NULL)
	, fObjectFactory(new (nothrow) ClipObjectFactory(true))
	, fDocument(new (nothrow) ::Document())
{
}

// destructor
ClockwerkApp::~ClockwerkApp()
{
	delete fDocument;
	delete fObjectLibrary;
}

// ArgvReceived
void
ClockwerkApp::ArgvReceived(int32 argc, char** argv)
{
}

// _ReInitClipsFromDisk
status_t
ClockwerkApp::_ReInitClipsFromDisk(bool lockEntireOperation,
	bool loadRemovedObjects)
{
	// create a temporary "persistent" object library
	// and init it from the object library file, then
	// sync fObjectLibrary to the temporary library

	// init a temporary object manager, but be careful not to resolve
	// the dependencies, so that the objects it contains do not
	// cross reference each other. This is important when adopting
	// the objects into the real ServerObjectManager instance.
	const char* directory = kObjectFilePath;
	if (fObjectLibrary->Directory()
		&& strlen(fObjectLibrary->Directory()) > 0)
		directory = fObjectLibrary->Directory();
	print_info("reloading object library from '%s'\n", directory);

	AttributeServerObjectManager objectManager;
	objectManager.SetLoadRemovedObjects(loadRemovedObjects);
	status_t ret = objectManager.Init(directory, fObjectFactory, false);

	if (ret == B_ENTRY_NOT_FOUND)
		ret = B_OK;

	// the temporary object library is read-only
	objectManager.SetIgnoreStateChanges(true);

	if (lockEntireOperation && !fDocument->WriteLock())
		return B_ERROR;

	print_info("updating object instances\n");

	// remove no longer needed clips
	int32 count = fObjectLibrary->CountObjects();
	for (int32 i = 0; i < count; i++) {
		AutoWriteLocker locker(fDocument);
		ServerObject* clip = fObjectLibrary->ObjectAtFast(i);
		if (!objectManager.FindObject(clip->ID())
			&& fObjectLibrary->RemoveObject(clip)) {

//			print_info("removed no longer needed clip '%s' (%s, version %ld)\n",
//				clip->Name().String(), clip->ID().String(), clip->Version());

			clip->Release();
			count--;
			i--;
		}
	}

	// update existing and add new clips
	count = objectManager.CountObjects();
	for (int32 i = count - 1; i >= 0; i--) {
		AutoWriteLocker locker(fDocument);
		ServerObject* newClip = objectManager.ObjectAtFast(i);
		ServerObject* oldClip = fObjectLibrary->FindObject(newClip->ID());
		if (oldClip) {
			int32 oldVersion = oldClip->Version();
			int32 newVersion = newClip->Version();
			if (oldVersion == newVersion) {
				// the object have the same version. If both
				// objects are "published", don't do anything
				if (oldClip->Status() == SYNC_STATUS_PUBLISHED
					&& newClip->Status() == SYNC_STATUS_PUBLISHED) {
					continue;
				}
			}
			// sync the old clip to the new one
			ret = oldClip->SetTo(newClip);
			if (ret != B_OK) {
				// any error
				if (ret != B_NO_MEMORY) {
					// ignore error
					print_warning("error updating clip '%s' (%s, version %ld) "
						"with version %ld, ignoring error: %s\n",
						oldClip->Name().String(), oldClip->ID().String(),
						oldVersion, newVersion, strerror(ret));
					ret = B_OK;
				} else {
					// serious error
					print_error("no memory for updating clip '%s' "
						"(%s, version %ld) with version %ld, aboring\n",
						oldClip->Name().String(), oldClip->ID().String(),
						oldVersion, newVersion);
					break;
				}
			} else {
				// success
//				print_info("updated clip '%s' (%s, version %ld) to "
//					"version %ld\n", oldClip->Name().String(),
//					oldClip->ID().String(), oldVersion, newVersion);
			}
		} else {
			// transfer ownership of the object to our own
			if (!objectManager.RemoveObject(newClip)) {
				ret = B_ERROR;
				print_error("failed to remove clip from temporary library '%s' "
					"(%s, version %ld), aboring\n",
					newClip->Name().String(), newClip->ID().String(),
					newClip->Version());
				break;
			}
			if (!fObjectLibrary->AddObject(newClip)) {
				ret = B_NO_MEMORY;
				print_error("no memory for adding new clip '%s' "
					"(%s, version %ld), aboring\n",
					newClip->Name().String(), newClip->ID().String(),
					newClip->Version());
				newClip->Release();
				break;
			}
//			print_info("added new clip '%s' (%s, version %ld)\n",
//				newClip->Name().String(), newClip->ID().String(),
//				newClip->Version());
		}
	}

	if (ret >= B_OK) {
		ret = fObjectLibrary->ResolveDependencies();
		if (ret < B_OK) {
			print_error("error resolving object library dependencies after "
				" reloading from disk: %s\n", strerror(ret));
		}
	}

	if (lockEntireOperation)
		fDocument->WriteUnlock();

	return ret;
}

// _ValidatePlaylistLayouts
void
ClockwerkApp::_ValidatePlaylistLayouts()
{
	// search for "Playlist" objects
	int32 count = fObjectLibrary->CountObjects();
	for (int32 i = 0; i < count; i++) {
		Playlist* playlist = dynamic_cast<Playlist*>(
			fObjectLibrary->ObjectAtFast(i));
		if (playlist)
			playlist->ValidateItemLayout();
	}
}

