/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef INFO_VIEW_H
#define INFO_VIEW_H

#include <View.h>

class InfoView : public BView {
 public:
								InfoView(BRect framem, const char* name);
	virtual						~InfoView();

	// BView interface
	virtual	void				Draw(BRect updateRect);
};

#endif // INFO_VIEW_H
