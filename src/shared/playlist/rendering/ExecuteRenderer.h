/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef EXECUTE_RENDERER_H
#define EXECUTE_RENDERER_H

#include <String.h>

#include "ClipRenderer.h"
#include "Font.h"

class ExecuteClip;

class ExecuteRenderer : public ClipRenderer {
 public:
								ExecuteRenderer(
									ClipPlaylistItem* item,
									ExecuteClip* clip);
	virtual						~ExecuteRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);
	virtual	void				Sync();

 private:
	static	int32				_CommandThread(void* cookie);

			ExecuteClip*		fClip;

			BString				fCommandPath;
			bool				fCommandExecuted;
};

#endif // EXECUTE_RENDERER_H
