/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CLOCK_RENDERER_H
#define CLOCK_RENDERER_H

#include <String.h>

#include "ClipRenderer.h"
#include "Font.h"

class ClockClip;

#define CLOCK_RENDER_TIMING 0

class ClockRenderer : public ClipRenderer {
 public:
								ClockRenderer(
									ClipPlaylistItem* item,
									ClockClip* clip);
	virtual						~ClockRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	void				Sync();

 private:
			void				_RenderDigitalClock(Painter* painter,
									double frame);
			void				_RenderAnalogClock(Painter* painter,
									double frame);

			ClockClip*			fClip;

			bool				fAnalogFace;
			time_t				fTime;
			BString				fText;
			Font				fFont;
			rgb_color			fColor;

	#if CLOCK_RENDER_TIMING
			bigtime_t			fRenderTime;
			bigtime_t			fSetFontTime;
			int64				fGeneratedFrames;
	#endif
};

#endif // CLOCK_RENDERER_H
