/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>

#include <Bitmap.h>
#include <Button.h>
#include <CheckBox.h>
#include <ControlLook.h>
#include <GroupLayoutBuilder.h>
#include <LayoutUtils.h>
#include <StatusBar.h>
#include <String.h>
#include <StringView.h>

#include "RenderWindow.h"

// ycrcb_to_rgb
inline void
ycbcr_to_rgb(uint8 y, uint8 cb, uint8 cr, uint8& r, uint8& g, uint8& b)
{
	r = (uint8)max_c( 0, min_c( 255, 1.164 * ( y - 16 ) + 1.596 * ( cr - 128 ) ) );
	g = (uint8)max_c( 0, min_c( 255, 1.164 * ( y - 16 ) - 0.813 * ( cr - 128 )
								- 0.391 * ( cb - 128 ) ) );
	b = (uint8)max_c( 0, min_c( 255, 1.164 * ( y - 16 ) + 2.018 * ( cb - 128 ) ) );
}

// #pragma mark - CurrentFrameView

class CurrentFrameView : public BView {
public:
								CurrentFrameView(float aspectRatio);
	virtual						~CurrentFrameView();

	virtual	void				Draw(BRect updateRect);

	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();

			void				DrawCurrentFrame(const BBitmap* currentFrame);

private:
			float				fRatio;
};

// constructor
CurrentFrameView::CurrentFrameView(float aspectRatio)
	:
	BView("current frame", B_WILL_DRAW),
	fRatio(aspectRatio)
{
	SetViewColor(0, 0, 0, 255);
}

// destructor
CurrentFrameView::~CurrentFrameView()
{
}

// Draw
void
CurrentFrameView::Draw(BRect updateRect)
{
	BRect rect(Bounds());
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawTextControlBorder(this, rect, updateRect, base);
}

// GetPreferredSize
void
CurrentFrameView::GetPreferredSize(float* width, float* height)
{
	if (height != NULL)
		*height = 70.0 + 4.0;
	if (width != NULL)
		*width = ceilf(70.0 * fRatio) + 4.0;
}

// MinSize
BSize
CurrentFrameView::MinSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), size);
}

// MaxSize
BSize
CurrentFrameView::MaxSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), size);
}

// PreferredSize
BSize
CurrentFrameView::PreferredSize()
{
	BSize size;
	GetPreferredSize(&size.width, &size.height);
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), size);
}

// DrawCurrentFrame
void
CurrentFrameView::DrawCurrentFrame(const BBitmap* currentFrame)
{
	uint32 flags;
	color_space format = currentFrame->ColorSpace();
	bitmaps_support_space(format, &flags);

	BRect r(Bounds());
	r.InsetBy(2.0f, 2.0f);

	if (flags & B_VIEWS_SUPPORT_DRAW_BITMAP) {
		DrawBitmap(currentFrame, currentFrame->Bounds(), r);
		return;
	}

	if (format != B_RGB24 && format != B_YCbCr422) {
		// inform user that preview is not supported
		SetLowColor(0, 0, 0, 255);
		SetHighColor(255, 255, 255, 255);
		const char* message = "Preview not supported";
		float stringWidth = StringWidth(message);
		font_height fh;
		GetFontHeight(&fh);
		BPoint p;
		p.x = (r.left + r.right) / 2.0f - stringWidth / 2.0f;
		p.y = (r.top + r.bottom) / 2.0f + fh.ascent;
		FillRect(r, B_SOLID_LOW);
		DrawString(message, p);
	}

	r.OffsetTo(0.0f, 0.0f);
	BBitmap temp(r, 0, B_RGB32);
	if (!temp.IsValid())
		return;

	uint8* src = (uint8*)currentFrame->Bits();
	uint8* dst = (uint8*)temp.Bits();
	uint32 srcBPR = currentFrame->BytesPerRow();
	uint32 dstBPR = temp.BytesPerRow();
	uint32 srcWidth = currentFrame->Bounds().IntegerWidth() + 1;
	uint32 srcHeight = currentFrame->Bounds().IntegerHeight() + 1;
	uint32 dstWidth = temp.Bounds().IntegerWidth() + 1;
	uint32 dstHeight = temp.Bounds().IntegerHeight() + 1;
	if (format == B_RGB24) {
		for (uint32 dstY = 0; dstY < dstHeight; dstY++) {
			uint8* dstHandle = dst;
			uint32 srcY = (dstY * srcHeight) / dstHeight;
			uint8* srcLine = src + srcY * srcBPR;
			for (uint32 dstX = 0; dstX < dstWidth; dstX++) {
				uint32 srcX = (dstX * srcWidth) / dstWidth;
				// B_RGB24 has 3 bytes per pixel
				uint8* srcHandle = srcLine + srcX * 3;
				dstHandle[0] = srcHandle[0];
				dstHandle[1] = srcHandle[1];
				dstHandle[2] = srcHandle[2];
				dstHandle += 4;
			}
			dst += dstBPR;
		}
	} else if (format == B_YCbCr422) {
		// Y0[7:0]  Cb0[7:0]  Y1[7:0]  Cr0[7:0]
		// Y2[7:0]  Cb2[7:0]  Y3[7:0]  Cr2[7:0]
		for (uint32 dstY = 0; dstY < dstHeight; dstY++) {
			uint8* dstHandle = dst;
			uint32 srcY = (dstY * srcHeight) / dstHeight;
			uint8* srcLine = src + srcY * srcBPR;
			for (uint32 dstX = 0; dstX < dstWidth; dstX++) {
				uint32 srcX = (dstX * srcWidth) / dstWidth;
				// B_YCbCr422 has 2 bytes per pixel
				uint8* srcHandle = srcLine + srcX * 2;
				if (srcX & 1) {
					// odd pixel has Cr component
					ycbcr_to_rgb(srcHandle[0],
								 srcHandle[-1],
								 srcHandle[1],
								 dstHandle[2],
								 dstHandle[1],
								 dstHandle[0]);
				} else {
					// even pixel has Cb component
					if (srcX < srcWidth) {
						ycbcr_to_rgb(srcHandle[0],
									 srcHandle[1],
									 srcHandle[3],
									 dstHandle[2],
									 dstHandle[1],
									 dstHandle[0]);
					} else {
						// don't access beyond last pixel
						ycbcr_to_rgb(srcHandle[0],
									 srcHandle[1],
									 srcHandle[-1],
									 dstHandle[2],
									 dstHandle[1],
									 dstHandle[0]);
					}
				}
				dstHandle += 4;
			}
			dst += dstBPR;
		}
	}
	DrawBitmap(&temp, BPoint(2.0f, 2.0f));
}


// #pragma mark - RenderWindow

// constructor
RenderWindow::RenderWindow(BRect frame, const char* title, BWindow* window,
		BLooper* target, float aspectRatio, bool enableOpenMovie)
	:
	BWindow(frame, title, B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_V_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS),
	fTarget(target)
{
	fFileText = new BStringView(NULL, "");
	fFileText->SetAlignment(B_ALIGN_CENTER);
	fTimeText = new BStringView(NULL, "");
	fTimeText->SetAlignment(B_ALIGN_RIGHT);
	fBeepCB = new BCheckBox("Beep when done", NULL);
	fOpenCB = new BCheckBox("Open movie when done", NULL);
	fOpenCB->SetValue(enableOpenMovie);

	BButton* cancelButton;
	const float spacing = 8.0f;

	BGroupLayout* layout = new BGroupLayout(B_VERTICAL);
	SetLayout(layout);

	AddChild(BGroupLayoutBuilder(B_VERTICAL, spacing)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(fFileText)
		.Add(fStatusBar = new BStatusBar(NULL, ""))
		.AddStrut(spacing * 2.0f)
		.AddGroup(B_HORIZONTAL, spacing)
			.Add(fCurrentFrameView = new CurrentFrameView(aspectRatio))
			.AddStrut(spacing * 2.0f)
			.AddGroup(B_VERTICAL, spacing)
				.Add(fTimeText)
				.AddGlue(spacing)
				.AddGroup(B_HORIZONTAL, spacing)
					.AddGroup(B_VERTICAL, spacing)
						.Add(fBeepCB)
						.Add(fOpenCB)
					.End()
					.AddGroup(B_VERTICAL, spacing)
						.AddGlue(5.0f)
						.Add(fPauseB = new BButton("Continue",
							new BMessage(MSG_RENDER_WINDOW_PAUSE)))
						.AddStrut(spacing * 3.0f)
						.Add(cancelButton = new BButton("Cancel",
							new BMessage(MSG_RENDER_WINDOW_CANCEL)))
					.End()
				.End()
			.End()
		.End()
	);

	fPauseB->SetTarget(fTarget);
	cancelButton->SetTarget(fTarget);
	fOpenCB->SetEnabled(enableOpenMovie);

	SetPaused(false);

	AddToSubset(window);

	Hide();
	Show();
}

// destructor
RenderWindow::~RenderWindow()
{
}

// MessageReceived
void
RenderWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		default:
			BWindow::MessageReceived(msg);
	}
}

// SetStatusBarInfo
void
RenderWindow::SetStatusBarInfo(const char* leftLabel, const char* rightLabel,
							   float maxValue, float currentValue)
{
	fStatusBar->SetMaxValue(maxValue);
	currentValue -= fStatusBar->CurrentValue();
	fStatusBar->Update(currentValue, leftLabel, rightLabel);
}

// DrawCurrentFrame
void
RenderWindow::DrawCurrentFrame(const BBitmap* currentFrame)
{
	if (Lock()) {
		fCurrentFrameView->DrawCurrentFrame(currentFrame);
		Unlock();
	}
}

// SetTimeText
void
RenderWindow::SetTimeText(const char* text)
{
	fTimeText->SetText(text);
}

// SetCurrentFileInfo
void
RenderWindow::SetCurrentFileInfo(const char* text)
{
	fFileText->SetText(text);
}

// SetPaused
void
RenderWindow::SetPaused(bool paused)
{
	if (paused)
		fPauseB->SetLabel("Continue");
	else
		fPauseB->SetLabel("Pause");
}

// BeepWhenDone
bool
RenderWindow::BeepWhenDone()
{
	return fBeepCB->Value() == B_CONTROL_ON;
}

// OpenMovieWhenDone
bool
RenderWindow::OpenMovieWhenDone()
{
	return fOpenCB->Value() == B_CONTROL_ON;
}

