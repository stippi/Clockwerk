/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */
#ifndef PLAYBACK_FRAME_VIEW_H
#define PLAYBACK_FRAME_VIEW_H


#include <View.h>
#include <String.h>


enum {
	TIME_MODE,
	FRAME_MODE,
};

class BBitmap;


class PlaybackFrameView : public BView {
public:
								PlaybackFrameView(int32 mode = TIME_MODE);
	virtual						~PlaybackFrameView();

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

	virtual	void				FrameResized(float width, float height);

	virtual	void				Draw(BRect updateRect);
	virtual	void				MouseDown(BPoint where);

			void				SetFramesPerSecond(float fps);
			void				SetTime(int32 frame);

			void				SetMode(int32 mode);
			int32				Mode() const
									{ return fMode; }

private:
			BString				fTimeText;
			float				fTextY;
			int32				fMode;
			int32				fFrame;
			float				fFPS;
};


#endif	// PLAYBACK_FRAME_VIEW_H
