/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef LOOP_MODE_H
#define LOOP_MODE_H

#include "Observable.h"

class LoopMode : public Observable {
 public:
								LoopMode();
	virtual						~LoopMode();

			void				SetMode(int32 mode);
			int32				Mode() const
									{ return fMode; }

 private:
			int32				fMode;
};

#endif // LOOP_MODE_H
