/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SPLIT_VIEW_H
#define SPLIT_VIEW_H


#include <View.h>


class Splitter;

class SplitView : public BView {
	public:
		SplitView(BRect frame, const char* name, orientation o = B_VERTICAL,
			bool proportional = false,
			uint32 resizingMode = B_FOLLOW_LEFT | B_FOLLOW_TOP);
		virtual ~SplitView();

		virtual void	AllAttached();
		virtual void	FrameResized(float width, float height);

		virtual void	SetOrientation(orientation o);
		orientation		Orientation() const;

		virtual void	SetProportionalMode(bool proportional);
		bool			ProportionalMode() const;

		virtual void	SetSplitterProportion(float proportion);
		float			SplitterProportion() const;

		virtual void	SetSplitterPosition(float position);
		float			SplitterPosition() const;

		virtual void	SetSplitterLimits(float min, float max);
		float			SplitterMinimum() const;
		float			SplitterMaximum() const;

		virtual void	Relayout();

	protected:
		virtual Splitter* NewSplitter() const;

	private:
		float			_Size() const;

		orientation		fOrientation;
		bool			fProportional;
		float			fMin, fMax, fPosition, fProportion, fPreviousSize;
		Splitter*		fSplitter;
};

class Splitter : public BView {
 public:
								Splitter(orientation o = B_VERTICAL);
	virtual						~Splitter();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
										   const BMessage* dragMessage);

	virtual	void				FrameMoved(BPoint parentPoint);

	// Splitter
	virtual	void				Move(float offset);
	virtual void				DoubleClickMove();

	virtual void				SetOrientation(orientation o);
			orientation			Orientation() const;

 private:
			bool				fTracking;
			bool				fIgnoreMouseMoved;
			BPoint				fMousePos;
			float				fInitialPosition;
			float				fPreviousPosition;
			orientation			fOrientation;
};

#endif // SPLIT_VIEW_H
