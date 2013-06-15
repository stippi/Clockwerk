/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DISPLAY_SETTINGS_H
#define DISPLAY_SETTINGS_H

#include "ServerObject.h"

class IntProperty;

class DisplaySettings : public ServerObject {
 public:
								DisplaySettings();
	virtual						~DisplaySettings();

	// DisplaySettings
			void				SetWidth(int32 width);
			int32				Width() const;

			void				SetHeight(int32 height);
			int32				Height() const;

			int32				InputSource() const;
			int32				DisplayWidth() const;
			int32				DisplayHeight() const;
			float				DisplayFrequency() const;

 private:
			IntProperty*		fWidth;
			IntProperty*		fHeight;
};

#endif // DISPLAY_SETTINGS_H
