/*
 * Copyright 2002-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef TRANSFORM_OBJECTS_COMMAND_H
#define TRANSFORM_OBJECTS_COMMAND_H

#include "TransformCommand.h"

class FloatProperty;
class PlaylistItem;
class PropertyObject;

class TransformObjectsCommand : public TransformCommand {
 public:
								TransformObjectsCommand(PlaylistItem** objects,
														int32 count,
														int64 frame,

														BPoint pivot,
														BPoint translation,
														double rotation,
														double xScale,
														double yScale,
		
														const char* actionName,
														int32 actionIndex);
	virtual						~TransformObjectsCommand();
	
	virtual	status_t			InitCheck();

 protected:
	virtual	status_t			_SetTransformation(BPoint pivotDiff,
												   BPoint translationDiff,
												   double rotationDiff,
												   double xScaleDiff,
												   double yScaleDiff) const;

			void				_SetProperty(PropertyObject* object,
											 FloatProperty* property,
											 float value, int64 frame) const;

			PlaylistItem**		fObjects;
			int32				fCount;
			int64				fFrame;
};

#endif // TRANSFORM_OBJECTS_COMMAND_H
