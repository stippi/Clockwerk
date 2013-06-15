/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PROPERTY_ANIMATOR_H
#define PROPERTY_ANIMATOR_H

#include <List.h>

#include "Observable.h"

class BMessage;
class KeyFrame;
class Property;

class PropertyAnimator : public Observable {
 public:
								PropertyAnimator(::Property* property);
								PropertyAnimator(::Property* property,
									const PropertyAnimator& other);
	virtual						~PropertyAnimator();

			status_t			Archive(BMessage* into) const;
			status_t			Unarchive(const BMessage* archive);

			::Property*			Property() const
									{ return fProperty; }

			void				DurationChanged(int64 duration);

			bool				SetToFrame(double frame) const;

			KeyFrame*			InsertKeyFrameAt(int64 frame);
			void				SetKeyFrameFrame(KeyFrame* key,
												 int64 frame);

	// KeyFrame list manipulation
			bool				AddKeyFrame(KeyFrame* key);

			KeyFrame*			RemoveKeyFrame(int32 index);
			bool				RemoveKeyFrame(KeyFrame* key);

			KeyFrame*			KeyFrameAt(int32 index) const;
			KeyFrame*			KeyFrameAtFast(int32 index) const;
			bool				HasKeyFrame(KeyFrame* key) const;
			int32				CountKeyFrames() const;
			int32				IndexOf(KeyFrame* key) const;

			void				MakeEmpty();

			KeyFrame*			KeyFrameBeforeOrAt(int64 frame) const;
			KeyFrame*			KeyFrameBehind(int64 frame) const;
			float				ScaleAt(int64 frame) const;
			float				ScaleAtFloat(float frame) const;

			bool				AnimatePropertyTo(::Property* property,
												   double frame) const;
 private:
			int32				_IndexForFrame(int64 frame) const;

//			void				_CleanUp(int64 fromOffset,
//										 int64 toOffset);

			::Property*			fProperty;

			BList				fKeyFrames;
};

#endif // PROPERTY_ANIMATOR_H
