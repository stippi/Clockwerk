/*
 * Copyright 2005-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */
#ifndef SCREEN_MODE_H
#define SCREEN_MODE_H


#include <Screen.h>


struct screen_mode {
	int32			width;		// these reflect the corrected width/height,
	int32			height;		// taking the combine mode into account
	color_space		space;
	float			refresh;

	void SetTo(display_mode& mode);
	int32 BitsPerPixel() const;

	bool operator==(const screen_mode &otherMode) const;
	bool operator!=(const screen_mode &otherMode) const;
};


class ScreenMode {
	public:
		ScreenMode(BWindow* window);
		~ScreenMode();

		status_t Set(const screen_mode& mode, int32 workspace = ~0);
		status_t Get(screen_mode& mode, int32 workspace = ~0) const;
		status_t GetOriginalMode(screen_mode &mode, int32 workspace = ~0) const;

		status_t Revert();
		void UpdateOriginalModes();

		bool SupportsColorSpace(const screen_mode& mode, color_space space);
		status_t GetRefreshLimits(const screen_mode& mode, float& min, float& max);

		screen_mode ModeAt(int32 index);
		int32 CountModes();

	private:
		bool GetDisplayMode(const screen_mode& mode, display_mode& displayMode);

		BWindow*		fWindow;
		display_mode*	fModeList;
		uint32			fModeCount;

		bool			fUpdatedModes;
		display_mode	fOriginalDisplayMode[32];
		screen_mode		fOriginal[32];
};

#endif	/* SCREEN_MODE_H */
