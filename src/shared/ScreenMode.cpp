/*
 * Copyright 2005-2007, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */

// NOTE: This is the Haiku version from src/preferences/screen/, but
// all the TV/combine mode stuff has been removed

#include "ScreenMode.h"

#include <InterfaceDefs.h>

#include <stdlib.h>
#include <stdio.h>


static float
get_refresh_rate(display_mode& mode)
{
	// we have to be catious as refresh rate cannot be controlled directly,
	// so it suffers under rounding errors and hardware restrictions
	return rint(10 * float(mode.timing.pixel_clock * 1000) / 
		float(mode.timing.h_total * mode.timing.v_total)) / 10.0;
}


/** helper to sort modes by resolution */

static int
compare_mode(const void* _mode1, const void* _mode2)
{
	display_mode *mode1 = (display_mode *)_mode1;
	display_mode *mode2 = (display_mode *)_mode2;
	uint16 width1, width2, height1, height2;

	width1 = mode1->virtual_width;
	height1 = mode1->virtual_height;
	width2 = mode2->virtual_width;
	height2 = mode2->virtual_height;

	if (width1 != width2)
		return width1 - width2;

	if (height1 != height2)
		return height1 - height2;

	return (int)(10 * get_refresh_rate(*mode1)
		-  10 * get_refresh_rate(*mode2));
}


//	#pragma mark -


int32
screen_mode::BitsPerPixel() const
{
	switch (space) {
		case B_RGB32:	return 32;
		case B_RGB24:	return 24;
		case B_RGB16:	return 16;
		case B_RGB15:	return 15;
		case B_CMAP8:	return 8;
		default:		return 0;
	}
}


bool
screen_mode::operator==(const screen_mode &other) const
{
	return !(*this != other);
}


bool
screen_mode::operator!=(const screen_mode &other) const
{
	return width != other.width || height != other.height
		|| space != other.space || refresh != other.refresh;
}


void
screen_mode::SetTo(display_mode& mode)
{
	width = mode.virtual_width;
	height = mode.virtual_height;
	space = (color_space)mode.space;
	refresh = get_refresh_rate(mode);
}


//	#pragma mark -


ScreenMode::ScreenMode(BWindow* window)
	:
	fWindow(window),
	fUpdatedModes(false)
{
	BScreen screen(window);
	if (screen.GetModeList(&fModeList, &fModeCount) == B_OK) {
		// sort modes by resolution and refresh to make
		// the resolution and refresh menu look nicer
		qsort(fModeList, fModeCount, sizeof(display_mode), compare_mode);
	} else {
		fModeList = NULL;
		fModeCount = 0;
	}
}


ScreenMode::~ScreenMode()
{
	free(fModeList);
}


status_t
ScreenMode::Set(const screen_mode& mode, int32 workspace)
{
	if (!fUpdatedModes)
		UpdateOriginalModes();

	BScreen screen(fWindow);
	
	if (workspace == ~0)
		workspace = current_workspace();
	
	display_mode displayMode;
	if (!GetDisplayMode(mode, displayMode))
		return B_ENTRY_NOT_FOUND;

	return screen.SetMode(workspace, &displayMode, true);
}


status_t
ScreenMode::Get(screen_mode& mode, int32 workspace) const
{
	display_mode displayMode;
	BScreen screen(fWindow);
	
	if (workspace == ~0)
		workspace = current_workspace();
	
	if (screen.GetMode(workspace, &displayMode) != B_OK)
		return B_ERROR;
	
	mode.SetTo(displayMode);

	return B_OK;
}


status_t
ScreenMode::GetOriginalMode(screen_mode& mode, int32 workspace) const
{
	if (workspace == ~0)
		workspace = current_workspace();
	else if(workspace > 31)
		return B_BAD_INDEX;
	
	mode = fOriginal[workspace];
	
	return B_OK;
}


// this method assumes that you already reverted to the correct number of workspaces
status_t
ScreenMode::Revert()
{
	if (!fUpdatedModes)
		return B_ERROR;

	status_t result = B_OK;
	screen_mode current;
	for (int32 workspace = 0; workspace < count_workspaces(); workspace++) {
		if (Get(current, workspace) == B_OK && fOriginal[workspace] == current)
			continue;

		BScreen screen(fWindow);

		result = screen.SetMode(workspace, &fOriginalDisplayMode[workspace], true);
		if (result != B_OK)
			break;
	}
	
	return result;
}


void
ScreenMode::UpdateOriginalModes()
{
	BScreen screen(fWindow);
	for (int32 workspace = 0; workspace < count_workspaces(); workspace++) {
		if (screen.GetMode(workspace, &fOriginalDisplayMode[workspace]) == B_OK) {
			Get(fOriginal[workspace], workspace);
			fUpdatedModes = true;
		}
	}
}


bool
ScreenMode::SupportsColorSpace(const screen_mode& mode, color_space space)
{
	return true;
}


status_t
ScreenMode::GetRefreshLimits(const screen_mode& mode, float& min, float& max)
{
	uint32 minClock, maxClock;
	display_mode displayMode;
	if (!GetDisplayMode(mode, displayMode))
		return B_ERROR;

	BScreen screen(fWindow);
	if (screen.GetPixelClockLimits(&displayMode, &minClock, &maxClock) < B_OK)
		return B_ERROR;

	uint32 total = displayMode.timing.h_total * displayMode.timing.v_total;
	min = minClock * 1000.0 / total;
	max = maxClock * 1000.0 / total;

	return B_OK;
}


screen_mode
ScreenMode::ModeAt(int32 index)
{
	if (index < 0)
		index = 0;
	else if (index >= (int32)fModeCount)
		index = fModeCount - 1;

	screen_mode mode;
	mode.SetTo(fModeList[index]);

	return mode;
}


int32
ScreenMode::CountModes()
{
	return fModeCount;
}


bool
ScreenMode::GetDisplayMode(const screen_mode& mode, display_mode& displayMode)
{
	uint16 virtualWidth, virtualHeight;
	int32 bestIndex = -1;
	float bestDiff = 999;

	virtualWidth = mode.width;
	virtualHeight = mode.height;

	// try to find mode in list provided by driver
	for (uint32 i = 0; i < fModeCount; i++) {
		if (fModeList[i].virtual_width != virtualWidth
			|| fModeList[i].virtual_height != virtualHeight
			|| (color_space)fModeList[i].space != mode.space)
			continue;

		float refresh = get_refresh_rate(fModeList[i]);
		if (refresh == mode.refresh) {
			// we have luck - we can use this mode directly
			displayMode = fModeList[i];
			displayMode.h_display_start = 0;
			displayMode.v_display_start = 0;
			return true;
		}

		float diff = fabs(refresh - mode.refresh);
		if (diff < bestDiff) {
			bestDiff = diff;
			bestIndex = i;
		}
	}

	// we didn't find the exact mode, but something very similar?
	if (bestIndex == -1)
		return false;

	// now, we are better of using GMT formula, but
	// as we don't have it, we just tune the pixel
	// clock of the best mode.

	displayMode = fModeList[bestIndex];
	displayMode.h_display_start = 0;
	displayMode.v_display_start = 0;

	// after some fiddling, it looks like this is the formula
	// used by the original panel (notice that / 10 happens before
	// multiplying with refresh rate - this leads to different
	// rounding)
	displayMode.timing.pixel_clock = ((uint32)displayMode.timing.h_total
		* displayMode.timing.v_total / 10 * int32(mode.refresh * 10)) / 1000;

	return true;
}

