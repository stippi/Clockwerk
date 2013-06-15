/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef VIDEO_VIEW_H
#define VIDEO_VIEW_H

#include <View.h>

#include "VCTarget.h"

class VideoView : public BView,
				  public VCTarget {
 public:
								VideoView(BRect frame,
										  const char* name);
	virtual						~VideoView();

	// BView interface
	virtual	void				Draw(BRect updateRect);

	// VCTarget interface
	virtual	void				SetBitmap(const BBitmap* bitmap);

 private:
			bool				fOverlayMode;
};


#endif // VIDEO_VIEW_H
