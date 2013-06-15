/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "NavigationInfoPanel.h"

#include <stdio.h>

#include <Alert.h>
#include <Autolock.h>
#include <Font.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <View.h>

#include "Clip.h"
#include "Playlist.h"
#include "TimelineMessages.h"

enum {
	MSG_INVOKE	= 'invk'
};

class NavigationInfoPanel::InfoView : public BView {
public:
	InfoView(BRect frame, NavigationInfoPanel* window)
		: BView(frame, "navigation info view", B_FOLLOW_ALL,
			B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE)
		, fAnticipateDrop(false)
		, fWindow(window)
	{
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	}

	virtual void Draw(BRect updateRect)
	{
		BRect r = Bounds();
		if (fAnticipateDrop) {
			SetHighColor(0, 150, 50);
			StrokeRect(r);
			r.InsetBy(1, 1);
			StrokeRect(r);
			r.InsetBy(1, 1);
		} else {
			rgb_color lighten2 = tint_color(LowColor(), B_LIGHTEN_2_TINT);
			rgb_color darken1 = tint_color(LowColor(), B_DARKEN_1_TINT);
			BeginLineArray(4);
				AddLine(BPoint(r.left, r.bottom),
					BPoint(r.left, r.top), lighten2);
				AddLine(BPoint(r.left + 1, r.top),
					BPoint(r.right, r.top), lighten2);
				AddLine(BPoint(r.right, r.top + 1),
					BPoint(r.right, r.bottom), darken1);
				AddLine(BPoint(r.right - 1, r.bottom),
					BPoint(r.left + 1, r.bottom), darken1);
			EndLineArray();
			r.InsetBy(1, 1);
		}
		FillRect(r, B_SOLID_LOW);
	}

	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage)
	{
		if (Bounds().Contains(where)) {
			_SetAnticipateDrop(dragMessage
				&& dragMessage->what == MSG_DRAG_CLIP);
		} else
			_SetAnticipateDrop(false);
	}

	virtual void MessageReceived(BMessage* message)
	{
		if (message->WasDropped())
			_SetAnticipateDrop(false);

		switch (message->what) {
			case MSG_DRAG_CLIP: {
				Clip* clip;
				if (message->FindPointer("clip", (void**)&clip) != B_OK)
					break;
				if (dynamic_cast<Playlist*>(clip) == NULL) {
					BAlert* alert = new BAlert("error", "The navigation "
						"target clip needs to be a playlist.\n", "Ok");
					alert->Go();
					break;
				}
				
				fWindow->SetTargetID(clip->ID().String());
				fWindow->PostMessage(MSG_INVOKE);
				break;
			}
			default:
				BView::MessageReceived(message);
		}
	}

private:
	void _SetAnticipateDrop(bool anticipate)
	{
		if (anticipate == fAnticipateDrop)
			return;
		fAnticipateDrop = anticipate;
		Invalidate();
	}

	bool					fAnticipateDrop;
	NavigationInfoPanel*	fWindow;
};

static const char* kLabel = "Define target for: ";

// constructor
NavigationInfoPanel::NavigationInfoPanel(BWindow* parent,
		const BMessage& message, const BMessenger& target)
	: BWindow(BRect(0, 0, 200, 30), "Navigation Info", B_FLOATING_WINDOW_LOOK,
		B_FLOATING_SUBSET_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE | B_NOT_V_RESIZABLE)
	, fMessage(message)
	, fTarget(target)
{
	// create the interface and resize to fit

	BRect frame = Bounds();
	frame.InsetBy(5, 5);
	frame.bottom = frame.top + 15;

	// label string view
	fLabelView = new BStringView(frame, "label", kLabel,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	fLabelView->ResizeToPreferred();
	frame = fLabelView->Frame();

	// target clip text control
	frame.OffsetBy(0, frame.Height() + 5);
	fTargetClipTC = new BTextControl(frame, "clip id",
		"Target Playlist ID", "", new BMessage(MSG_INVOKE),
		B_FOLLOW_TOP | B_FOLLOW_LEFT_RIGHT);
	fTargetClipTC->ResizeToPreferred();
	frame = fTargetClipTC->Frame();

	// help string view
	frame.OffsetBy(0, frame.Height() + 5);
	BStringView* helpView = new BStringView(frame, "help",
		"Drag and drop a playlist clip here.",
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_TOP);
	BFont font;
	helpView->GetFont(&font);
	font.SetFace(B_ITALIC_FACE);
	font.SetSize(font.Size() * 0.9);
	helpView->SetFont(&font);
	helpView->SetAlignment(B_ALIGN_CENTER);
	helpView->ResizeToPreferred();

	// parent view
	frame = fLabelView->Frame() | fTargetClipTC->Frame() | helpView->Frame();
	frame.InsetBy(-5, -5);
	fInfoView = new InfoView(frame, this);
	fInfoView->AddChild(fLabelView);
	fInfoView->AddChild(fTargetClipTC);
	fInfoView->AddChild(helpView);

	// resize to fit and adjust size limits
	ResizeTo(fInfoView->Frame().Width(), fInfoView->Frame().Height());
	AddChild(fInfoView);
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = Frame().Width();
	minHeight = maxHeight = Frame().Height();
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// modify the high color after the help view is attached to a window
	helpView->SetHighColor(tint_color(helpView->LowColor(),
		B_DISABLED_LABEL_TINT));
	helpView->SetFlags(helpView->Flags() | B_FULL_UPDATE_ON_RESIZE);
		// help the buggy BeOS BStringView (when text alignment != left...)

	fInfoView->SetEventMask(B_POINTER_EVENTS);

	// resize controls to the same (maximum) width
	float maxControlWidth = fLabelView->Frame().Width();
	maxControlWidth = max_c(maxControlWidth, fTargetClipTC->Frame().Width());
	maxControlWidth = max_c(maxControlWidth, helpView->Frame().Width());
	fLabelView->ResizeTo(maxControlWidth, fLabelView->Frame().Height());
	fTargetClipTC->ResizeTo(maxControlWidth, fTargetClipTC->Frame().Height());
	helpView->ResizeTo(maxControlWidth, helpView->Frame().Height());

	// center above parent window
	BAutolock _(parent);
	frame = Frame();
	BRect parentFrame = parent->Frame();
	MoveTo((parentFrame.left + parentFrame.right - frame.Width()) / 2,
		(parentFrame.top + parentFrame.bottom - frame.Height()) / 2);

	AddToSubset(parent);
}

// destructor
NavigationInfoPanel::~NavigationInfoPanel()
{
}

// QuitRequested
bool
NavigationInfoPanel::QuitRequested()
{
	_Invoke();
	Hide();
	return false;
}

// MessageReceived
void
NavigationInfoPanel::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_INVOKE:
			_Invoke();
			break;
		default:
			BWindow::MessageReceived(message);
	}
}

// SetLabel
void
NavigationInfoPanel::SetLabel(const char* text)
{
	BAutolock _(this);

	BString label(kLabel);
	label << text;
	fLabelView->SetText(label.String());
}

// SetTargetID
void
NavigationInfoPanel::SetTargetID(const char* targetID)
{
	BAutolock _(this);

	fTargetClipTC->SetText(targetID);
}

// SetMessage
void
NavigationInfoPanel::SetMessage(const BMessage& message)
{
	BAutolock _(this);

	fMessage = message;
}

// _Invoke
void
NavigationInfoPanel::_Invoke()
{
	BMessage message(fMessage);
	message.AddString("target id", fTargetClipTC->Text());
	fTarget.SendMessage(&message);
}

