/*
 * Copyright 2007-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef VIDEO_VIEW_SELECTION_H
#define VIDEO_VIEW_SELECTION_H

#include "Selection.h"

class VideoViewSelection : public Selection {
 public:
								VideoViewSelection();
	virtual						~VideoViewSelection();

	// this is meant to be able to associate a playlist item
	// (selection in playlist) to the video selection, only if the
	// selection in the playlist is the associated selectable, then
	// the VideoViewSelection is allowed to contain anything
			void				SetAssociatedSelectable(Selectable* selectable);
	inline	Selectable*			AssociatedSelectable() const
									{ return fAssociatedSelectable; }

 private:
			Selectable*			fAssociatedSelectable;

};

#endif // VIDEO_VIEW_SELECTION_H
