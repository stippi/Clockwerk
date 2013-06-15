/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef LOCK_VIEW_H
#define LOCK_VIEW_H

#include <View.h>

class BBitmap;

class LockView : public BView {
public:
								LockView();
	virtual						~LockView();

	// BView interface
	virtual	void				Draw(BRect update);
	virtual	void				MouseDown(BPoint where);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

	// LockView
	virtual	void				SetEnabled(bool enabled);
	virtual	bool				IsEnabled();
	virtual	void				SetLocked(bool locked);
	virtual	bool				IsLocked();

private:
			bool				fEnabled;
			bool				fLocked;
			BBitmap*			fOpenEnabledBitmap;
			BBitmap*			fOpenDisabledBitmap;
			BBitmap*			fClosedEnabledBitmap;
			BBitmap*			fClosedDisabledBitmap;
};

#endif // LOCK_VIEW_H
