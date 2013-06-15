/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EDITOR_VIDEO_VIEW_H
#define EDITOR_VIDEO_VIEW_H

#include "Observer.h"
#include "Scrollable.h"
#include "StateView.h"
#include "VCTarget.h"


class BMessageRunner;
class MultipleManipulatorState;
class ObjectSelection;
class Playlist;
class Selection;
class StageTool;
class VideoViewSelection;

class EditorVideoView : public StateView, public Observer, public Scrollable,
	public VCTarget {
public:
								EditorVideoView(BRect frame, const char* name);
	virtual						~EditorVideoView();

	// BackBufferedStateView interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	void				AttachedToWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	virtual	bool				MouseWheelChanged(BPoint where,
												  float x, float y);

	virtual	void				UpdateStateCursor();
	virtual	void				ViewStateBoundsChanged();

	virtual	void				ConvertFromCanvas(BPoint* point) const;
	virtual	void				ConvertToCanvas(BPoint* point) const;

	virtual	void				ConvertFromCanvas(BRect* rect) const;
	virtual	void				ConvertToCanvas(BRect* rect) const;

	// Scrollable interface
protected:
	virtual	void				SetScrollOffset(BPoint offset);

	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);
	virtual	void				VisibleSizeChanged(float oldWidth,
												   float oldHeight,
												   float newWidth,
												   float newHeight);
	// VCTarget interface
public:
	virtual	void				SetBitmap(const BBitmap* bitmap);

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// EditorVideoView
			double				NextZoomInLevel(double zoom) const;
			double				NextZoomOutLevel(double zoom) const;
			void				SetZoomLevel(double zoomLevel,
										 bool mouseIsAnchor = true);
	virtual	double				ZoomLevel() const
									{ return fZoomLevel; }
			void				SetAutoZoomToAll(bool autoZoom);

			void				SetAutoScrolling(bool scroll);

			void				SetVideoSize(uint32 width, uint32 height);
			void				DisableOverlay();

			void				SetTool(StageTool* tool);
			void				SetSelection(Selection* playlistSelection,
											 VideoViewSelection* videoViewSelection);
			void				SetPlaylist(::Playlist* playlist);
			void				SetCurrentFrame(int64 frame);

			void				GetOverlayScaleLimits(float* minScale,
									float* maxScale) const;

			bool				HandlesAllKeyEvents() const;

protected:
	// StateView interface
	virtual	bool				_HandleKeyDown(const StateView::KeyEvent& event,
									BHandler* originalHandler);
	virtual	bool				_HandleKeyUp(const StateView::KeyEvent& event,
									BHandler* originalHandler);

	// EditorVideoView
private:
			BRect				_CanvasRect() const;
			BRect				_LayoutCanvas();
			void				_ZoomToAll(float width, float height);

			void				_RebuildManipulators();
			void				_UpdateManipulatorsWithCurrentFrame();

			void				_SynchronousDraw();
			void				_DrawNoVideoMessage(BView* into,
									BRect updateRect);

			uint32				_OverlayFilterFlags();

			bool				fOverlayMode;

			Selection*			fPlaylistSelection;
			VideoViewSelection*	fVideoViewSelection;
			::Playlist*			fPlaylist;
			StageTool*			fTool;

			MultipleManipulatorState*	fViewState;
			int64				fCurrentFrame;

			uint32				fVideoWidth;
			uint32				fVideoHeight;

			double				fZoomLevel;
			bool				fAutoZoomToAll;

			bool				fSpaceHeldDown;
			bool				fScrollTracking;
			bool				fInScrollTo;
			BPoint				fScrollTrackingStart;
			BPoint				fScrollOffsetStart;

			BMessageRunner*		fAutoScroller;

			overlay_restrictions fOverlayRestrictions;
			rgb_color			fOverlayKeyColor;
};


#endif // EDITOR_VIDEO_VIEW_H
