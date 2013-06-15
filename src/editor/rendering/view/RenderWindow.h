/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef RENDER_WINDOW_H
#define RENDER_WINDOW_H

#include <Window.h>

class BButton;
class BCheckBox;
class BStatusBar;
class BStringView;
class CurrentFrameView;

enum {
	MSG_RENDER_WINDOW_PAUSE			= 'rwnp',
	MSG_RENDER_WINDOW_CANCEL		= 'rwnc',
};

class RenderWindow : public BWindow {
public:
								RenderWindow(BRect frame, const char* title,
									BWindow* window, BLooper* target,
									float aspectRatio,
									bool enableOpenMovie = true);
	virtual						~RenderWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage *msg);

	// RenderWindow
			void				SetStatusBarInfo(const char* leftLabel,
									const char* rightLabel, float maxValue,
									float currentValue);
			void				DrawCurrentFrame(const BBitmap* currentFrame);
			void				SetTimeText(const char* text);
			void				SetCurrentFileInfo(const char* message);
			void				SetPaused(bool paused);

			bool				BeepWhenDone();
			bool				OpenMovieWhenDone();


private:
			BStatusBar*			fStatusBar;
			BStringView*		fFileText;
			BLooper*			fTarget;
			BCheckBox*			fBeepCB;
			BCheckBox*			fOpenCB;
			CurrentFrameView*	fCurrentFrameView;
			BStringView*		fTimeText;
			BButton*			fPauseB;
};

#endif // RENDER_WINDOW_H
