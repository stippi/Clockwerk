/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCROLLING_TEXT_RENDERER_H
#define SCROLLING_TEXT_RENDERER_H

#include <List.h>
#include <Locker.h>
#include <String.h>

#include "ClipRenderer.h"
#include "Font.h"
#include "HashMap.h"
#include "HashString.h"

class ScrollingTextClip;

class ScrollingTextRenderer : public ClipRenderer {
 public:
								ScrollingTextRenderer(
									ClipPlaylistItem* item,
									ScrollingTextClip* clip);
	virtual						~ScrollingTextRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	void				Sync();

 private:
			struct text_item;

			text_item*			_AppendText();
			void				_RebuildTextItemWidth();

			ScrollingTextClip*	fClip;

			BList				fTextItems;

			BString				fText;
			Font				fFont;
			rgb_color			fColor;
			bool				fUseOutline;
			rgb_color			fOutlineColor;

			font_height			fTextHeight;
			float				fScrollingSpeed;
			float				fTextPos;
			float				fWidth;

			float				fInitialScrollOffset;
			BString				fClipID;

 private:
 	// a global instance class for managing scrolling offsets
 	// of various ScrollingTextClip objects with a timeout before
 	// resetting the offset for a particular clip
	class ScrollOffsetManager {
	public:
								ScrollOffsetManager();
		virtual					~ScrollOffsetManager();

				float			ScrollOffsetFor(const BString& clipID,
									bigtime_t resetTimeout);

				void			UpdateScrollOffset(const BString& clipID,
									float offset);

	private:
		struct ScrollOffset {
								ScrollOffset(bigtime_t timeout);
				ScrollOffset&	operator=(const ScrollOffset& other);

			float				offset;
			bigtime_t			lastUpdated;
			bigtime_t			timeout;
		};

		typedef HashMap<HashString, ScrollOffset*> ScrollOffsetMap;
			ScrollOffsetMap		fScrollOffsetMap;
			BLocker				fLock;
	};

	static	ScrollOffsetManager	sScrollOffsetManager;
};

#endif // SCROLLING_TEXT_RENDERER_H
