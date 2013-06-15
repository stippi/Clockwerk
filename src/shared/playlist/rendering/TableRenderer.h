/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TABLE_RENDERER_H
#define TABLE_RENDERER_H

#include <GraphicsDefs.h>

#include "ClipRenderer.h"
#include "TableClip.h"

class TableRenderer : public ClipRenderer {
 public:
								TableRenderer(ClipPlaylistItem* item,
											  TableClip* clip);
	virtual						~TableRenderer();

	// ClipRenderer interface
	virtual	status_t			Generate(Painter* painter, double frame,
									const RenderPlaylistItem* item);

	virtual	void				Sync();

 private:
			void				_AnimateFadeIn(uint32 column, uint32 row,
											   uint32 columns, uint32 rows,
											   Painter* painter,
											   BRect cellRect,
											   double frame);
			void				_AnimateFadeOut(uint32 column, uint32 row,
											   uint32 columns, uint32 rows,
											   Painter* painter,
											   BRect cellRect,
											   double frame,
											   int64 itemDuration);

			TableClip*			fClip;

			TableData			fTable;

			float				fColumnSpacing;
			float				fRowSpacing;
			float				fRoundCornerRadius;

			int32				fFadeInMode;
			int64				fFadeInFrames;

			int32				fFadeOutMode;
			int64				fFadeOutFrames;
};

#endif // TABLE_RENDERER_H
