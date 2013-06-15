/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLIP_H
#define CLIP_H

#include <Rect.h>
#include <String.h>

#include "ServerObject.h"
#include "Selectable.h"

class BBitmap;
class Painter;
class AudioReader;

class Clip : public ServerObject,
			 public Selectable {
 public:
								Clip(const char* type,
									 const char* name = NULL);
								Clip(const Clip& other, bool deep);
	virtual						~Clip();

	// Selectable
	virtual	void				SelectedChanged();

	// Clip interface
	virtual	uint64				Duration() = 0;
	virtual	uint64				MaxDuration();
			bool				HasMaxDuration();

	virtual	bool				HasVideo();

	virtual	bool				HasAudio();
	virtual	AudioReader*		CreateAudioReader();

			bool				LogPlayback() const;

	virtual	BRect				Bounds(BRect canvasBounds) = 0;

			void				SetTemplateName(const char* templateName);
			const char*			TemplateName() const;
			bool				IsTemplate() const;

	virtual	bool				GetIcon(BBitmap* icon);

	virtual	uint32				ChangeToken() const;
		// currently used to indicate changes of
		// FileBasedClips where the actual content
		// of the clip was reloaded from disk, and
		// some other object needs to update, for
		// example an icon

 protected:
			bool				GetBuiltInIcon(BBitmap* icon,
											   const uchar* iconData) const;

 private:
	static	vint32				sUnamedClipCount;
};

#endif // CLIP_H
