/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "BitmapClip.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include <Bitmap.h>
#include <Message.h>
#include <TranslationUtils.h>

#include "AutoDeleter.h"
#include "BBitmapBuffer.h"
#include "CommonPropertyIDs.h"
#include "OptionProperty.h"
#include "Painter.h"

using std::nothrow;

// constructor
BitmapClip::BitmapClip(const entry_ref* ref)
	: FileBasedClip(ref)
	, fBitmap(NULL)
	, fIcon(NULL)

	, fFadeMode(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_FADE_MODE)))
{
	SetValue(PROPERTY_WIDTH, 0L);
	SetValue(PROPERTY_HEIGHT, 0L);

	fInitStatus = _LoadBitmap();
	if (fInitStatus == B_OK)
		_StoreArchive();

	delete fBitmap;
	fBitmap = NULL;
}

// constructor
BitmapClip::BitmapClip(const entry_ref* ref, const BMessage& archive)
	: FileBasedClip(ref)
	, fBitmap(NULL)
	, fIcon(NULL)

	, fFadeMode(dynamic_cast<OptionProperty*>(
			FindProperty(PROPERTY_FADE_MODE)))
{
	fInitStatus = archive.FindRect("bounds", &fBounds);

	BMessage bitmap;
	if (archive.FindMessage("icon", &bitmap) == B_OK) {
		fIcon = new (nothrow) BBitmap(&bitmap);
		if (fIcon->InitCheck() < B_OK) {
			delete fIcon;
			fIcon = NULL;
		}
	}

	SetValue(PROPERTY_WIDTH, fBounds.IntegerWidth() + 1);
	SetValue(PROPERTY_HEIGHT, fBounds.IntegerHeight() + 1);
}

// destructor
BitmapClip::~BitmapClip()
{
	delete fBitmap;
	delete fIcon;
}

// Duration
uint64
BitmapClip::Duration()
{
	// TODO: default duration for bitmaps from program
	// settings
	return 100;
}

// Bounds
BRect
BitmapClip::Bounds(BRect canvasBounds)
{
	if (fBitmap)
		return fBitmap->Bounds();
	return fBounds;
}

// GetIcon
bool
BitmapClip::GetIcon(BBitmap* icon)
{
	// if we already have fIcon, use that for the bitmap
	if (fIcon)
		return _GetIcon(icon);

	// if we don't have fIcon, we need fBitmap, try to load it in case
	// it is NULL
	if (fBitmap == NULL)
		_LoadBitmap();

	// if we still don't have fBitmap, we can't do anything anymore
	if (fBitmap == NULL)
		return false;

	// prepare fIcon for future GetIcon calls to be quick
	BBitmap* cachedIcon = new (nothrow) BBitmap(BRect(0, 0, 31, 31), B_RGBA32);
	if (cachedIcon && cachedIcon->InitCheck() >= B_OK) {
		// place the loaded bitmap in the icon
		if (_GetIcon(cachedIcon))
			fIcon = cachedIcon;
		else
			delete cachedIcon;
	} else
		delete cachedIcon;

	// at this point, we should be able to provide an icon
	bool success = _GetIcon(icon);

	// no use for the large bitmap anymore
	delete fBitmap;
	fBitmap = NULL;

	return success;
}

// InitCheck
status_t
BitmapClip::InitCheck()
{
	return fInitStatus;
}
// #pragma mark -

//// Bitmap
//const BBitmap*
//BitmapClip::Bitmap()
//{
//	if (fBitmap != NULL)
//		return fBitmap;
//
//	// initialize bitmap
//	status_t status = _LoadBitmap();
//	if (status < B_OK)
//		return NULL;
//
//	return fBitmap;
//}

// FadeMode
uint32
BitmapClip::FadeMode() const
{
	if (fFadeMode)
		return fFadeMode->Value();
	return FADE_MODE_ALPHA;
}

// #pragma mark -

// CreateClip
/*static*/ Clip*
BitmapClip::CreateClip(const entry_ref* ref, status_t& error)
{
	BitmapClip* bitmapClip = new (nothrow) BitmapClip(ref);
	if (!bitmapClip) {
		error = B_NO_MEMORY;
		printf("FileBasedClip::_BitmapClipFor() - "
			   "no memory to create BitmapClip\n");
		return NULL;
	}

	error = bitmapClip->InitCheck();
	if (error >= B_OK) {
		// clip is a bitmap
		return bitmapClip;
	}

	// clip is not a bitmap
	delete bitmapClip;

	return NULL;
}

// #pragma mark -

// HandleReload
void
BitmapClip::HandleReload()
{
	// we need to do this unfortunately, since we don't
	// know at this point if the bitmap changed, and at least
	// for the correct dimensions, we need to load it again
	// TODO: think of some way to avoid this. The purpose of
	// this function is to reload the bitmap for example when
	// it was edited in an external editor like WonderBrush,
	// or when a new version has been downloaded.
	if (_LoadBitmap() < B_OK)
		return;

	// no use for the large bitmap anymore
	delete fBitmap;
	fBitmap = NULL;

	if (Status() == SYNC_STATUS_PUBLISHED)
		SetStatus(SYNC_STATUS_MODIFIED);
}

// _LoadBitmap
status_t
BitmapClip::_LoadBitmap()
{
	delete fBitmap;
	delete fIcon;
	fIcon = NULL;

	// TODO: find out if the file *should* be
	// a bitmap, and if loading fails, report
	// to the user
	try {
		fBitmap = BTranslationUtils::GetBitmap(&fRef);
		if (!fBitmap || (fBitmap->ColorSpace() != B_RGB32
						&& fBitmap->ColorSpace() != B_RGBA32)) {
			// it appears the file is no bitmap,
			// or the translator is a smart ass
			delete fBitmap;
			fBitmap = NULL;
			return B_ERROR;
		}
	} catch (...) {
		printf("BitmapClip::_LoadBitmap() - caught exception\n");
		fBitmap = NULL;
		return B_ERROR;
	}

	// final check for success
	status_t ret = fBitmap->InitCheck();
	if (ret < B_OK) {
		printf("BitmapClip::_LoadBitmap() - BBitmap init error: %s\n",
			   strerror(ret));
		return ret;
	}

	SetValue(PROPERTY_WIDTH, fBitmap->Bounds().IntegerWidth() + 1);
	SetValue(PROPERTY_HEIGHT, fBitmap->Bounds().IntegerHeight() + 1);

	return B_OK;
}

// _StoreArchive
status_t
BitmapClip::_StoreArchive()
{
	if (fBitmap == NULL)
		return B_NO_INIT;

	BBitmap icon(BRect(0, 0, 31, 31), B_RGBA32);
	_GetIcon(&icon);

	BMessage bitmap;
	status_t status = icon.Archive(&bitmap, false);
	if (status < B_OK)
		return status;

	BMessage archive('clip');
	status = archive.AddRect("bounds", fBitmap->Bounds());
	if (status == B_OK)
		status = archive.AddMessage("icon", &bitmap);
	if (status == B_OK)
		status = FileBasedClip::_Store("BitmapClip", archive);

	return status;
}

// _GetIcon
bool
BitmapClip::_GetIcon(BBitmap* icon)
{
	BBitmap* source = fBitmap != NULL ? fBitmap : fIcon;
	if (source == NULL)
		return false;

	// wrap a RenderingBuffer around the BBitmap
	BBitmapBuffer srcBuffer(source);

	Painter painter;
	BBitmapBuffer dstBuffer(icon);
	painter.AttachToBuffer(&dstBuffer);
	painter.ClearBuffer();
	painter.DrawBitmap(&srcBuffer, srcBuffer.Bounds(), dstBuffer.Bounds());
	painter.FlushCaches();

	return true;
}
