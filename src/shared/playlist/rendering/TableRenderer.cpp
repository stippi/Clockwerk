/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TableRenderer.h"

#include <stdio.h>
#include <stdlib.h>

#include "ui_defines.h"
#include "support.h"

#include "CommonPropertyIDs.h"
#include "Painter.h"

// constructor
TableRenderer::TableRenderer(ClipPlaylistItem* item,
							 TableClip* clip)
	: ClipRenderer(item, clip),
	  fClip(clip),
	  fTable(),

	  fColumnSpacing(4.0),
	  fRowSpacing(4.0),
	  fRoundCornerRadius(4.0),

	  fFadeInMode(TABLE_FADE_IN_EXPAND),
	  fFadeInFrames(10)
{
	if (fClip)
		fClip->Acquire();
}

// destructor
TableRenderer::~TableRenderer()
{
	if (fClip)
		fClip->Release();
}

// Generate
status_t
TableRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	status_t ret = fTable.InitCheck();
	if (ret < B_OK)
		return ret;

//	painter->SetSubpixelPrecise(true);

	// TODO: handle different fFadeInModes
	// TODO: handle different fFadeOutModes
	// ("frame" is clip local)

	// render table
	uint32 columns = fTable.CountColumns();
	uint32 rows = fTable.CountRows();

	int64 duration = Duration();

	BPoint cellLeftTop(B_ORIGIN);

	Font font;
	for (uint32 i = 0; i < columns; i++) {
		float width = fTable.ColumnWidth(i);
		for (uint32 j = 0; j < rows; j++) {
			// compute cell frame
			float height = fTable.RowHeight(j);
			BRect r(cellLeftTop.x, cellLeftTop.y,
					cellLeftTop.x + width - 1, cellLeftTop.y + height - 1);
			// inset for column/row spacing
			r.InsetBy(fColumnSpacing / 2.0, fRowSpacing / 2.0);

			// handle fade in/out animation
			bool popGraphicsStack = false;
			if (frame < fFadeInFrames) {
				// setup painter for fade-in mode
				painter->PushState();
				popGraphicsStack = true;
				_AnimateFadeIn(i, j, columns, rows, painter, r, frame);
			} else if (frame >= duration - fFadeOutFrames) {
				// setup painter for fade-in mode
				painter->PushState();
				popGraphicsStack = true;
				_AnimateFadeOut(i, j, columns, rows, painter, r, frame,
								duration);
			}

			painter->SetColor(fTable.CellBackgroundColor(i, j));
			// fill round rect for background
			painter->FillRoundRect(r, fRoundCornerRadius, fRoundCornerRadius);
			cellLeftTop.y += height;
			// content
			fTable.GetCellFont(i, j, &font);
			BString text = fTable.CellText(i, j);
			int32 length = text.Length();

			painter->SetFont(&font);
			float textWidth = painter->StringWidth(text.String(), length);

			font_height fh;
			font.GetHeight(&fh);

			BPoint textPos(B_ORIGIN);

			switch (fTable.CellHorizontalAlignment(i, j)) {
				default:
				case ALIGN_STRETCH:
					// TODO...
				case ALIGN_BEGIN:
					textPos.x = r.left;
					break;
				case ALIGN_END:
					textPos.x = r.right - textWidth - 1;
					break;
				case ALIGN_CENTER:
					textPos.x = r.left + (r.Width() + 1 - textWidth) / 2.0;
					break;
			}
			switch (fTable.CellVerticalAlignment(i, j)) {
				default:
				case ALIGN_STRETCH:
					// TODO...
				case ALIGN_BEGIN:
					textPos.y = r.top + fh.ascent - 1;
					break;
				case ALIGN_END:
					textPos.y = r.bottom - fh.descent;
					break;
				case ALIGN_CENTER:
					textPos.y = r.top + (r.Height() + 1
									- (fh.ascent + fh.descent)) / 2.0
									+ fh.ascent;
					break;
			}

			painter->SetColor(fTable.CellContentColor(i, j));
			painter->DrawString(text.String(), length, textPos);

			if (popGraphicsStack) {
				painter->PopState();
			}
		}
		cellLeftTop.y = 0;
		cellLeftTop.x += width;
	}

	return B_OK;
}

// Sync
void
TableRenderer::Sync()
{
	ClipRenderer::Sync();

	if (!fClip)
		return;

	fTable = fClip->Table();

	fColumnSpacing = fClip->ColumnSpacing();
	fRowSpacing = fClip->RowSpacing();
	fRoundCornerRadius = fClip->RoundCornerRadius();

	fFadeInMode = fClip->FadeInMode();
	fFadeInFrames = fClip->FadeInFrames();

	fFadeOutMode = fClip->FadeOutMode();
	fFadeOutFrames = fClip->FadeOutFrames();
}

// #pragma mark -

// _AnimateFadeIn
void
TableRenderer::_AnimateFadeIn(uint32 column, uint32 row,
							  uint32 columns, uint32 rows,
							  Painter* painter,
							  BRect cellRect, double frame)
{
	// basically, this function sets up painter for the specific
	// fade-in mode (global alpha and/or transformation)
	switch (fFadeInMode) {
		case TABLE_FADE_IN_EXPAND_ROWS: {
			float fadeInDuration = fFadeInFrames / 2.0;
			float delay = fadeInDuration * (float)row / (rows - 1);
			float size = 1.0 - gauss(min_c(1.0, max_c(0.0,
							(frame - delay) / fadeInDuration)));
			AffineTransform transform;
			transform.ScaleBy(BPoint(0.0, cellRect.top), 1.0, size);
			painter->SetTransformation(transform);
			break;
		}
		case TABLE_FADE_IN_EXPAND_COLUMNS: {
			float fadeInDuration = fFadeInFrames / 2.0;
			float delay = fadeInDuration * (float)column / (columns - 1);
			float size = 1.0 - gauss(min_c(1.0, max_c(0.0,
							(frame - delay) / fadeInDuration)));
			AffineTransform transform;
			transform.ScaleBy(BPoint(0.0, cellRect.top), 1.0, size);
			painter->SetTransformation(transform);
			break;
		}
		case TABLE_FADE_IN_SLIDE: {
			float fadeInDuration = fFadeInFrames / 2.0;
			float delay = fadeInDuration * ((float)(column + row)
								/ ((columns - 1) + (rows - 1)));
			float dist = gauss(min_c(1.0, max_c(0.0,
							(frame - delay) / fadeInDuration)));
			AffineTransform transform;
			transform.TranslateBy(BPoint(dist * 1000, 0.0));
			painter->SetTransformation(transform);
			break;
		}
		case TABLE_FADE_IN_EXPAND:
		default: {
			float fadeInDuration = fFadeInFrames / 2.0;
			float delay = fadeInDuration * ((float)(column + row)
								/ ((columns - 1) + (rows - 1)));
			float alpha = 1.0 - gauss(min_c(1.0, max_c(0.0,
							(frame - delay) / fadeInDuration)));
			painter->SetAlpha((uint8)(alpha * 255.0 + 0.5));
			break;
		}
	}
}


// _AnimateFadeOut
void
TableRenderer::_AnimateFadeOut(uint32 column, uint32 row,
							   uint32 columns, uint32 rows,
							   Painter* painter,
							   BRect cellRect, double frame,
							   int64 itemDuration)
{
	// basically, this function sets up painter for the specific
	// fade-out mode (global alpha and/or transformation)
	switch (fFadeOutMode) {
		case TABLE_FADE_COLLAPSE_ROWS: {
			float fadeOutDuration = fFadeOutFrames / 2.0;
			double fadeFrame = frame - (itemDuration - fFadeOutFrames + 1);

			float delay = fadeOutDuration * (float)row / (rows - 1);
			float size = gauss(min_c(1.0, max_c(0.0,
							(fadeFrame - delay) / fadeOutDuration)));
			AffineTransform transform;
			transform.ScaleBy(BPoint(0.0, cellRect.top), 1.0, size);
			painter->SetTransformation(transform);
			break;
		}
		case TABLE_FADE_COLLAPSE_COLUMNS: {
			float fadeOutDuration = fFadeOutFrames / 2.0;
			double fadeFrame = frame - (itemDuration - fFadeOutFrames + 1);

			float delay = fadeOutDuration * (float)column / (columns - 1);
			float size = gauss(min_c(1.0, max_c(0.0,
							(fadeFrame - delay) / fadeOutDuration)));
			AffineTransform transform;
			transform.ScaleBy(BPoint(0.0, cellRect.top), 1.0, size);
			painter->SetTransformation(transform);
			break;
		}
		case TABLE_FADE_OUT_SLIDE: {
			float fadeOutDuration = fFadeOutFrames / 2.0;
			double fadeFrame = frame - (itemDuration - fFadeOutFrames + 1);

			float delay = fadeOutDuration * ((float)(column + row)
								/ ((columns - 1) + (rows - 1)));
			float dist = 1.0 - gauss(min_c(1.0, max_c(0.0,
							(fadeFrame - delay) / fadeOutDuration)));
			AffineTransform transform;
			transform.TranslateBy(BPoint(dist * -1000, 0.0));
			painter->SetTransformation(transform);
			break;
		}
		case TABLE_FADE_OUT_COLLAPSE:
		default: {
			float fadeOutDuration = fFadeOutFrames / 2.0;
			double fadeFrame = frame - (itemDuration - fFadeOutFrames + 1);

			float delay = fadeOutDuration * ((float)(column + row)
								/ ((columns - 1) + (rows - 1)));
			float alpha = gauss(min_c(1.0, max_c(0.0,
							(fadeFrame - delay) / fadeOutDuration)));
			painter->SetAlpha((uint8)(alpha * 255.0 + 0.5));
			break;
		}
	}
}
			


