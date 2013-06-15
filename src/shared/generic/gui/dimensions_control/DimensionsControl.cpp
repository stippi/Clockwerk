/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "DimensionsControl.h"

#include <stdio.h>
#include <stdlib.h>

#include <Handler.h>
#include <Looper.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Message.h>
#include <String.h>

#include <GridLayoutBuilder.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include "LockView.h"

// debugging
#include "Debug.h"
//#define ldebug	debug
#define ldebug	nodebug

enum {
	MSG_WIDTH_CHANGED		= 'wdtc',
	MSG_HEIGHT_CHANGED		= 'hgtc',
	MSG_COMMON_FORMAT		= 'cfmt',

	FORMAT_160_120,
	FORMAT_320_240,
	FORMAT_400_300,
	FORMAT_640_480,
	FORMAT_800_600,
	FORMAT_1024_768,
	FORMAT_1152_864,
	FORMAT_1280_800,
	FORMAT_1280_960,
	FORMAT_1280_1024,
	FORMAT_1400_1050,
	FORMAT_1600_1200,
	FORMAT_1920_1200,
	FORMAT_2048_1536,

	FORMAT_384_288,
	FORMAT_768_576,
	FORMAT_720_540,

	FORMAT_1280_720,
	FORMAT_1920_1080,

	FORMAT_352_240,
	FORMAT_768_524,
	FORMAT_720_492,

	FORMAT_720_480,
	FORMAT_720_576,

	FORMAT_384_216,
	FORMAT_768_432,
	FORMAT_720_405,

	FORMAT_384_164,
	FORMAT_768_327,
	FORMAT_720_306,
};

class ArrowPopup : public BMenuField {
public:
	ArrowPopup(BMenu* menu)
		:
		BMenuField("", menu != NULL ? menu : new BPopUpMenu("popup"))
	{
		Menu()->SetLabelFromMarked(false);
		if (BMenuItem* superItem = Menu()->Superitem())
			superItem->SetLabel("");
		Menu()->SetRadioMode(false);
	}

	virtual	~ArrowPopup() {}

	virtual	BSize MinSize()
	{
		BSize size(BMenuField::MinSize());
		// TODO: Don't hardcode...
		size.width = 21.0f;
		return size;
	}

	virtual	BSize MaxSize()
	{
		return MinSize();
	}
};

// constructor
DimensionsControl::DimensionsControl(BMessage* message, BHandler* target,
		float horizontalSpacing, float verticalSpacing)
	:
	BGridView(horizontalSpacing, verticalSpacing),
	fMessage(message),
	fTarget(target),
	fWidthTC(NULL),
	fHeightTC(NULL),
	fLockView(NULL),
	fCommonFormatsPU(NULL),
	fPreviousWidth(0),
	fPreviousHeight(0),
	fMinWidth(0),
	fMaxWidth(0),
	fMinHeight(0),
	fMaxHeight(0)
{
	// build common formats menu
	fCommonFormatsPU = new ArrowPopup(NULL);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_160_120);
	BMenuItem* item = new BMenuItem("160 x 120", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_320_240);
	item = new BMenuItem("320 x 240", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_400_300);
	item = new BMenuItem("400 x 300", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_640_480);
	item = new BMenuItem("640 x 480", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_800_600);
	item = new BMenuItem("800 x 600", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1024_768);
	item = new BMenuItem("1024 x 768", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1152_864);
	item = new BMenuItem("1152 x 864", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1280_800);
	item = new BMenuItem("1280 x 800", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1280_960);
	item = new BMenuItem("1280 x 960 (4:3)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1280_1024);
	item = new BMenuItem("1280 x 1024 (5:4)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1400_1050);
	item = new BMenuItem("1400 x 1050", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1600_1200);
	item = new BMenuItem("1600 x 1200", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1920_1200);
	item = new BMenuItem("1920 x 1200", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_2048_1536);
	item = new BMenuItem("2048 x 1536", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	fCommonFormatsPU->Menu()->AddSeparatorItem();

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_384_288);
	item = new BMenuItem("384 x 288 (1/4 PAL)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_768_576);
	item = new BMenuItem("768 x 576 (PAL)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_720_540);
	item = new BMenuItem("720 x 540 (PAL TV cropped)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	fCommonFormatsPU->Menu()->AddSeparatorItem();

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1280_720);
	item = new BMenuItem("1280 x 720 (HDTV 16:9)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_1920_1080);
	item = new BMenuItem("1920 x 1080 (HDTV 16:9)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

/*	fCommonFormatsPU->Menu()->AddSeparatorItem();

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_352_240);
	item = new BMenuItem("352 x 240 (1/4 NTSC)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_768_524);
	item = new BMenuItem("768 x 524 (NTSC)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_720_492);
	item = new BMenuItem("720 x 492 (TV cropped NTSC)", message);
	fCommonFormatsPU->Menu()->AddItem(item);*/

	fCommonFormatsPU->Menu()->AddSeparatorItem();

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_720_576);
	item = new BMenuItem("720 x 576 (DV PAL)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_720_480);
	item = new BMenuItem("720 x 480 (DV NTSC)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	fCommonFormatsPU->Menu()->AddSeparatorItem();

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_384_216);
	item = new BMenuItem("384 x 216 (1/4 PAL 16:9)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_768_432);
	item = new BMenuItem("768 x 432 (PAL 16:9)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_720_405);
	item = new BMenuItem("720 x 405 (PAL 16:9 TV cropped)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	fCommonFormatsPU->Menu()->AddSeparatorItem();

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_384_164);
	item = new BMenuItem("384 x 164 (1/4 PAL 2.35:1)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_768_327);
	item = new BMenuItem("768 x 327 (PAL 2.35:1)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	message = new BMessage(MSG_COMMON_FORMAT);
	message->AddInt32("format", FORMAT_720_306);
	item = new BMenuItem("720 x 306 (PAL 2.35:1 TV cropped)", message);
	fCommonFormatsPU->Menu()->AddItem(item);

	fWidthTC = new BTextControl("Width:", "",
		new BMessage(MSG_WIDTH_CHANGED));
	fHeightTC = new BTextControl("Height:", "",
		new BMessage(MSG_HEIGHT_CHANGED));

	fLockView = new LockView();

	BGridLayoutBuilder(this)
		.Add(fWidthTC->CreateLabelLayoutItem(), 0, 0)
		.Add(fWidthTC->CreateTextViewLayoutItem(), 1, 0)
		.Add(fHeightTC->CreateLabelLayoutItem(), 0, 1)
		.Add(fHeightTC->CreateTextViewLayoutItem(), 1, 1)
		.Add(fLockView, 2, 0, 1, 2)
		.Add(fCommonFormatsPU, 3, 0)
	;

	// only accept numbers in text views
	for (uint32 i = 0; i < '0'; i++) {
		fWidthTC->TextView()->DisallowChar(i);
		fHeightTC->TextView()->DisallowChar(i);
	}
	for (uint32 i = '9' + 1; i < 255; i++) {
		fWidthTC->TextView()->DisallowChar(i);
		fHeightTC->TextView()->DisallowChar(i);
	}
	// set bubble help texts
//	helper->SetHelp(fWidthTC, "");
//	helper->SetHelp(fHeightTC, "");
//	helper->SetHelp(fLockView, "Lock proportion of Width and Height.");
//	helper->SetHelp(fCommonFormatsPU, "Pick one of many common formats.");

	// put something in the textviews
	_SetDimensions(fMinWidth, fMinHeight, false);
}

// destructor
DimensionsControl::~DimensionsControl()
{
	delete fMessage;
}

// AttachedToWindow
void
DimensionsControl::AttachedToWindow()
{
	fWidthTC->SetTarget(this);
	fHeightTC->SetTarget(this);
	fCommonFormatsPU->Menu()->SetTargetForItems(this);
}

// MessageReceiced
void
DimensionsControl::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case MSG_WIDTH_CHANGED:
			SetWidth(Width());
			break;
		case MSG_HEIGHT_CHANGED:
			SetHeight(Height());
			break;
		case MSG_COMMON_FORMAT: {
			int32 format;
			if (message->FindInt32("format", &format) == B_OK) {
				uint32 width;
				uint32 height;
				switch (format) {
					case FORMAT_160_120:
						width = 160;
						height = 120;
						break;
					case FORMAT_320_240:
						width = 320;
						height = 240;
						break;
					case FORMAT_400_300:
						width = 400;
						height = 300;
						break;
					case FORMAT_640_480:
						width = 640;
						height = 480;
						break;
					case FORMAT_800_600:
						width = 800;
						height = 600;
						break;

					case FORMAT_1024_768:
						width = 1024;
						height = 768;
						break;
					case FORMAT_1152_864:
						width = 1152;
						height = 864;
						break;
					case FORMAT_1280_800:
						width = 1280;
						height = 800;
						break;
					case FORMAT_1280_960:
						width = 1280;
						height = 960;
						break;
					case FORMAT_1280_1024:
						width = 1280;
						height = 1024;
						break;
					case FORMAT_1400_1050:
						width = 1400;
						height = 1050;
						break;
					case FORMAT_1600_1200:
						width = 1600;
						height = 1200;
						break;
					case FORMAT_1920_1200:
						width = 1920;
						height = 1200;
						break;
					case FORMAT_2048_1536:
						width = 2048;
						height = 1536;
						break;

					case FORMAT_384_288:
						width = 384;
						height = 288;
						break;
					case FORMAT_768_576:
						width = 768;
						height = 576;
						break;
					case FORMAT_720_540:
						width = 720;
						height = 540;
						break;

					case FORMAT_1280_720:
						width = 1280;
						height = 720;
						break;
					case FORMAT_1920_1080:
						width = 1920;
						height = 1080;
						break;

					case FORMAT_352_240:
						width = 352;
						height = 240;
						break;
					case FORMAT_768_524:
						width = 768;
						height = 524;
						break;
					case FORMAT_720_492:
						width = 720;
						height = 492;
						break;
					case FORMAT_720_480:
						width = 720;
						height = 480;
						break;
					case FORMAT_720_576:
						width = 720;
						height = 576;
						break;
					case FORMAT_384_216:
						width = 384;
						height = 216;
						break;
					case FORMAT_768_432:
						width = 768;
						height = 432;
						break;
					case FORMAT_720_405:
						width = 720;
						height = 405;
						break;
					case FORMAT_384_164:
						width = 384;
						height = 164;
						break;
					case FORMAT_768_327:
						width = 768;
						height = 327;
						break;
					case FORMAT_720_306:
						width = 720;
						height = 306;
						break;
					default:
						width = 384;
						height = 288;
						break;
				}
				_SetDimensions(width, height);
			}
			break;
		}
		default:
			BGridView::MessageReceived(message);
			break;
	}
}

// SetDimensions
void
DimensionsControl::SetDimensions(uint32 width, uint32 height)
{
	_SetDimensions(width, height, false);
}

// SetWidth
void
DimensionsControl::SetWidth(uint32 width)
{
	uint32 height = Height();
	if (fLockView->IsLocked())
		height = GetLockedHeightFor(width);
	_SetDimensions(width, height);
}

// SetHeight
void
DimensionsControl::SetHeight(uint32 height)
{
	uint32 width = Width();
	if (fLockView->IsLocked())
		width = GetLockedWidthFor(height);
	_SetDimensions(width, height);
}

// Width
uint32
DimensionsControl::Width() const
{
	return atoi(fWidthTC->Text());
}

// Height
uint32
DimensionsControl::Height() const
{
	return atoi(fHeightTC->Text());
}

// SetWidthLimits
void
DimensionsControl::SetWidthLimits(uint32 min, uint32 max)
{
	if (max < min)
		max = min;
	fMinWidth = min;
	fMaxWidth = max;
	_SetDimensions(Width(), Height(), false);
}

// SetHeightLimits
void
DimensionsControl::SetHeightLimits(uint32 min, uint32 max)
{
	if (max < min)
		max = min;
	fMinHeight = min;
	fMaxHeight = max;
	_SetDimensions(Width(), Height(), false);
}

// SetProportionsLocked
void
DimensionsControl::SetProportionsLocked(bool lock)
{
	fLockView->SetLocked(lock);
}

// IsProportionsLocked
bool
DimensionsControl::IsProportionsLocked() const
{
	return fLockView->IsLocked();
}

// SetEnabled
void
DimensionsControl::SetEnabled(bool enabled)
{
	fWidthTC->SetEnabled(enabled);
	fHeightTC->SetEnabled(enabled);
	fLockView->SetEnabled(enabled);
	fCommonFormatsPU->SetEnabled(enabled);
}

// IsEnabled
bool
DimensionsControl::IsEnabled() const
{
	return fLockView->IsEnabled();
}

// SetLabels
void
DimensionsControl::SetLabels(const char* width, const char* height)
{
	fWidthTC->SetLabel(width);
	fHeightTC->SetLabel(height);
}

// WidthControl
BTextControl*
DimensionsControl::WidthControl() const
{
	return fWidthTC;
}

// HeightControl
BTextControl*
DimensionsControl::HeightControl() const
{
	return fHeightTC;
}

// GetLockedWidthFor
uint32
DimensionsControl::GetLockedWidthFor(uint32 newHeight)
{
	float proportion = (float)fPreviousHeight / (float)fPreviousWidth;
		// using old height
	return (uint32)((float)newHeight / proportion);
}

// GetLockedHeightFor
uint32
DimensionsControl::GetLockedHeightFor(uint32 newWidth)
{
	float proportion = (float)fPreviousWidth / (float)fPreviousHeight;
		// using old width
	return (uint32)((float)newWidth / proportion);
}

// _SetDimensions
void
DimensionsControl::_SetDimensions(uint32 width, uint32 height,
	bool sendMessage)
{
	// check limits
	if (width < fMinWidth)
		width = fMinWidth;
	if (width > fMaxWidth)
		width = fMaxWidth;
	if (height < fMinHeight)
		height = fMinHeight;
	if (height > fMaxHeight)
		height = fMaxHeight;
	// put this into the text controls
	BString helper("");
	helper << width;
	fWidthTC->SetText(helper.String());
	if (fWidthTC->TextView()->IsFocus())
		fWidthTC->TextView()->SelectAll();
	helper.SetTo("");
	helper << height;
	fHeightTC->SetText(helper.String());
	if (fHeightTC->TextView()->IsFocus())
		fHeightTC->TextView()->SelectAll();
	// reset previous width and height
	fPreviousWidth = width;
	fPreviousHeight = height;
	// notify target
	if (sendMessage && fTarget && fMessage) {
		if (BLooper* looper = fTarget->Looper()) {
			BMessage message(*fMessage);
			message.AddPointer("source", this);
			message.AddInt64("when", system_time());
			message.AddInt32("width", (int32)Width());
			message.AddInt32("height", (int32)Height());
			looper->PostMessage(&message, fTarget);
		}
	}
}

