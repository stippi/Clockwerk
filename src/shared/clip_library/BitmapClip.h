/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef BITMAP_CLIP_H
#define BITMAP_CLIP_H

#include "FileBasedClip.h"

class BBitmap;
class BBitmapBuffer;
class OptionProperty;

class BitmapClip : public FileBasedClip {
 public:
								BitmapClip(const entry_ref* ref);
								BitmapClip(const entry_ref* ref,
									const BMessage& archive);
	virtual						~BitmapClip();

	// Clip interface
	virtual	uint64				Duration();

	virtual	BRect				Bounds(BRect canvasBounds);

	virtual	bool				GetIcon(BBitmap* icon);

	// FileBasedClip interface
	virtual	status_t			InitCheck();
		// TODO: there should be one more method
		// like "RecognizeFile()", so that
		// other FileBasedClip types are not
		// tried anymore

	// BitmapClip
			uint32				FadeMode() const;

	static	Clip*				CreateClip(const entry_ref* ref,
									status_t& error);

 protected:
	virtual	void				HandleReload();

			status_t			_LoadBitmap();
			status_t			_StoreArchive();
			bool				_GetIcon(BBitmap* icon);

 private:
			BBitmap*			fBitmap;
			BBitmap*			fIcon;
			BRect				fBounds;
			status_t			fInitStatus;

			OptionProperty*		fFadeMode;
};

#endif // BITMAP_CLIP_H
