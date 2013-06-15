/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "VideoView.h"

#include <stdio.h>

#include <Bitmap.h>

// constructor
VideoView::VideoView(BRect frame, const char* name)
	: BView(frame, name, B_FOLLOW_NONE,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE),
	  fOverlayMode(false)
{
	SetViewColor(0, 0, 0, 255);
		// will be reset to overlay key color
		// if overlays are used
}

// destructor
VideoView::~VideoView()
{
}

// Draw
void
VideoView::Draw(BRect updateRect)
{
	if (LockBitmap()) {
		BRect r(Bounds());
		if (const BBitmap* bitmap = GetBitmap()) {
			if (!fOverlayMode) {
				DrawBitmap(bitmap, bitmap->Bounds(), r);
			}
		}
		UnlockBitmap();
	}
}

// SetBitmap
void
VideoView::SetBitmap(const BBitmap* bitmap)
{
	VCTarget::SetBitmap(bitmap);
	// Attention: Don't lock the window, if the bitmap is NULL. Otherwise
	// we're going to deadlock when the window tells the node manager to
	// stop the nodes (Window -> NodeManager -> VideoConsumer -> VideoView
	// -> Window).
	if (bitmap && LockLooperWithTimeout(10000) == B_OK) {
		if (LockBitmap()) {
//			if (fOverlayMode || bitmap->Flags() & B_BITMAP_WILL_OVERLAY) {
			if (fOverlayMode || bitmap->ColorSpace() == B_YCbCr422) {
				if (!fOverlayMode) {
					// init overlay
					rgb_color key;
					SetViewOverlay(bitmap,
								   bitmap->Bounds(),
				                   Bounds(),
				                   &key, B_FOLLOW_ALL,
				                   B_OVERLAY_FILTER_HORIZONTAL
				                   | B_OVERLAY_FILTER_VERTICAL);
					SetViewColor(key);
					Invalidate();
					// use overlay from here on
					fOverlayMode = true;
				} else {
					// transfer overlay channel
					rgb_color key;
					SetViewOverlay(bitmap,
								   bitmap->Bounds(),
								   Bounds(),
								   &key, B_FOLLOW_ALL,
								   B_OVERLAY_FILTER_HORIZONTAL
								   | B_OVERLAY_FILTER_VERTICAL
								   | B_OVERLAY_TRANSFER_CHANNEL);
				}
			} else {
				DrawBitmap(bitmap, bitmap->Bounds(), Bounds());
			}
			UnlockBitmap();
		}
		UnlockLooper();
	}
}

