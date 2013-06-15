/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "FileBasedClip.h"

#include <new>
#include <stdio.h>

#include <File.h>
#include <fs_attr.h>
#include <Mime.h>
#include <NodeInfo.h>
#include <Path.h>
#include <OS.h>

#include "common.h"

#include "BitmapClip.h"
#include "CommonPropertyIDs.h"
#include "MediaClip.h"
#include "Property.h"
#include "ServerObjectManager.h"

using std::nothrow;

// constructor
FileBasedClip::FileBasedClip(const entry_ref* ref)
	: Clip("FileBasedClip", ref->name),
	  fRef(*ref),
	  fChangeToken(0)
{
}

// constructor
FileBasedClip::FileBasedClip(const char* type, const entry_ref* ref)
	: Clip(type, ref->name),
	  fRef(*ref),
	  fChangeToken(0)
{
}

// destructor
FileBasedClip::~FileBasedClip()
{
}

// SetTo
status_t
FileBasedClip::SetTo(const ServerObject* _other)
{
	const FileBasedClip* other = dynamic_cast<const FileBasedClip*>(_other);
	if (!other)
		return B_ERROR;

	Reload();

	return Clip::SetTo(_other);
}

// IsMetaDataOnly
bool
FileBasedClip::IsMetaDataOnly() const
{
	return false;
}

// IsExternalData
bool
FileBasedClip::IsExternalData() const
{
	return true;
}

// ChangeToken
uint32
FileBasedClip::ChangeToken() const
{
	return fChangeToken;
}

// #pragma mark -

// CreateClip
Clip*
FileBasedClip::CreateClip(ServerObjectManager* library, const entry_ref* ref,
	status_t& error, bool import, bool allowLazyLoading)
{
	if (!ref)
		return NULL;

	entry_ref importRef = *ref;
	BString serverID = 0;

	if (import) {
		error = _ImportFile(library, importRef, serverID);
		if (error < B_OK) {
			printf("FileBasedClip::_ImportFile() - %s\n", strerror(error));
			return NULL;
		}
	}

	// detect file...

	Clip* clip = NULL;

	if (allowLazyLoading) {
		const char* className;
		BMessage archive;
		if (_GetStored(importRef, className, archive) == B_OK) {
			if (!strcmp(className, "MediaClip"))
				clip = new MediaClip(&importRef, archive);
			else if (!strcmp(className, "BitmapClip"))
				clip = new BitmapClip(&importRef, archive);
		}
	}

	// try bitmap
	if (clip == NULL)
		clip = BitmapClip::CreateClip(&importRef, error);
	// try media file
	if (clip == NULL)
		clip = MediaClip::CreateClip(&importRef, error);

	if (clip) {
		// set the name for now (will be overridden by properties
		// if they exist)
		clip->SetName(ref->name);
		if (import) {
			// if this file was imported (copied into the object folder,
			// we use the generated id)
			clip->SetID(serverID);
			// we also use the mime type of the original file if it has one
			BPath path(ref);
			BNode node(ref);
			BNodeInfo nodeInfo(&node);
			char mimeType[B_MIME_TYPE_LENGTH];
			if (nodeInfo.GetType(mimeType) != B_OK) {
				// update mime info on original file, non-forced though
				bool recursive = false;
				bool synchronous = true;
				bool force = false;
				print_info("updating MIME type of '%s'\n", path.Path());
				update_mime_info(path.Path(), recursive, synchronous, force);
			}
			if (nodeInfo.GetType(mimeType) == B_OK)
				clip->SetValue(PROPERTY_MIME_TYPE, mimeType);
			else
				print_error("failed to get MIME type of '%s'\n", path.Path());
		}
	} else {
		printf("failed to find suitable clip for file\n");
	}

	return clip;
}

// Reload
void
FileBasedClip::Reload()
{
	SuspendNotifications(true);

	fChangeToken++;
	HandleReload();
	Notify();

	SuspendNotifications(false);
}

// #pragma mark -

// _Store
status_t
FileBasedClip::_Store(const char* className, BMessage& archive)
{
	status_t status = archive.ReplaceString("class", className);
	if (status != B_OK)
		status = archive.AddString("class", className);
	if (status < B_OK)
		return status;

	BMallocIO stream;
	status = archive.Flatten(&stream);
	if (status < B_OK)
		return status;

	BNode node;
	status = node.SetTo(Ref());
	if (status < B_OK)
		return status;

	ssize_t bytesWritten = node.WriteAttr("CLKW:stored_clip", B_MESSAGE_TYPE,
		0, stream.Buffer(), stream.BufferLength());
	if (bytesWritten < B_OK)
		return bytesWritten;
	if (bytesWritten != (ssize_t)stream.BufferLength())
		return B_ERROR;

	return B_OK;
}

// _GetStored
status_t
FileBasedClip::_GetStored(const entry_ref& ref, const char*& className,
	BMessage& archive)
{
	BNode node;
	status_t status = node.SetTo(&ref);
	if (status < B_OK)
		return status;

	attr_info info;
	status = node.GetAttrInfo("CLKW:stored_clip", &info);
	if (status < B_OK)
		return status;

	BMallocIO stream;
	status = stream.SetSize(info.size);
	if (status < B_OK)
		return status;

	ssize_t bytesRead = node.ReadAttr("CLKW:stored_clip", B_MESSAGE_TYPE,
		0, (void *)stream.Buffer(), stream.BufferLength());
	if (bytesRead < B_OK)
		return bytesRead;
	if (bytesRead != (ssize_t)stream.BufferLength())
		return B_ERROR;

	status = archive.Unflatten(&stream);
	if (status < B_OK)
		return status;

	return archive.FindString("class", &className);
}

// _ImportFile
status_t
FileBasedClip::_ImportFile(ServerObjectManager* library, entry_ref& ref,
	BString& serverID)
{
	// import file into object library
	BFile original(&ref, B_READ_ONLY);

	serverID = ServerObjectManager::NextID();

	library->GetRef(serverID, ref);
		// reuse ref for file at import location

	BFile imported(&ref, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);

	status_t error = original.InitCheck();
	if (error != B_OK || (error = imported.InitCheck()) != B_OK) {
		printf("failed to init files for copy\n");
		return error;
	}

	// copy files
	bool copySuccess = copy_data(original, imported) >= 0;

	original.Unset();
	imported.Unset();

	if (!copySuccess) {
		// clean up
		BEntry entry(&ref);
		entry.Remove();
		printf("failed to copy files for import\n");
		return B_ERROR;
	}

	return B_OK;
}
