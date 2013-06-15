/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef KEY_FRAME_H
#define KEY_FRAME_H

#include "Observable.h"

class Property;

class KeyFrame : public Observable {
 public:
								KeyFrame(::Property* property,
										 int64 frame,
										 bool locked = false);
								KeyFrame(const KeyFrame& other);
	virtual						~KeyFrame();

			void				SetFrame(int64 frame,
										 bool force = false);
	inline	int64				Frame() const
									{ return fFrame; }

			void				SetLocked(bool locked);
	inline	bool				IsLocked() const
									{ return fLocked; }

	inline	::Property*			Property() const
									{ return fProperty; }
			bool				SetScale(float scale);
			float				Scale() const;

 private:
			int64				fFrame;
			bool				fLocked;

			::Property*			fProperty;
};

#endif // KEY_FRAME_H
