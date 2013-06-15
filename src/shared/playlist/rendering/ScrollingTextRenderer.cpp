/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScrollingTextRenderer.h"

#include <new>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "ui_defines.h"

#include "AutoLocker.h"
#include "Painter.h"
#include "RenderPlaylistItem.h"
#include "ScrollingTextClip.h"

using std::nothrow;

static const char* kSeparatorString = "   +++   ";

struct ScrollingTextRenderer::text_item {
	BString		text;
	float		width;
};

ScrollingTextRenderer::ScrollOffsetManager
ScrollingTextRenderer::sScrollOffsetManager;

// #pragma mark - ScrollOffsetManager


ScrollingTextRenderer::ScrollOffsetManager::ScrollOffset::ScrollOffset(
		bigtime_t timeout)
	: offset(0.0)
	, lastUpdated(system_time())
	, timeout(timeout)
{
}


ScrollingTextRenderer::ScrollOffsetManager::ScrollOffsetManager()
	: fScrollOffsetMap()
	, fLock("ScrollOffsetManager")
{
}


ScrollingTextRenderer::ScrollOffsetManager::~ScrollOffsetManager()
{
	ScrollOffsetMap::Iterator iterator = fScrollOffsetMap.GetIterator();
	while (iterator.HasNext())
		delete iterator.Next().value;
}


float
ScrollingTextRenderer::ScrollOffsetManager::ScrollOffsetFor(
	const BString& clipID, bigtime_t resetTimeout)
{
	AutoLocker<BLocker> locker(fLock);
	if (!locker.IsLocked())
		return 0.0;

	if (!fScrollOffsetMap.ContainsKey(clipID.String())) {
		// add new offset
		ScrollOffset* offset = new (nothrow) ScrollOffset(resetTimeout);
		if (!offset) {
			print_error("failed to allocate ticker scroll offset\n");
			return 0.0;
		}
		if (fScrollOffsetMap.Put(clipID.String(), offset) < B_OK) {
			delete offset;
			print_error("failed to put ticker scroll offset into hash map\n");
		}
		return 0.0;
	}

	// reuse existing offset and update it
	ScrollOffset* offset = fScrollOffsetMap.Get(clipID.String());

	// reset offset if timeout expired
	bigtime_t now = system_time();
	if (now - offset->lastUpdated > offset->timeout)
		offset->offset = 0.0;

	offset->lastUpdated = system_time();
	return offset->offset;
}


void
ScrollingTextRenderer::ScrollOffsetManager::UpdateScrollOffset(
	const BString& clipID, float newOffset)
{
	AutoLocker<BLocker> locker(fLock);
	if (!locker.IsLocked())
		return;

	if (!fScrollOffsetMap.ContainsKey(clipID.String()))
		return;

	// find existing offset and update it
	ScrollOffset* offset = fScrollOffsetMap.Get(clipID.String());

	offset->lastUpdated = system_time();
	offset->offset = newOffset;
}


// #pragma mark - ScrollingTextRenderer

// constructor
ScrollingTextRenderer::ScrollingTextRenderer(ClipPlaylistItem* item,
											 ScrollingTextClip* clip)
	: ClipRenderer(item, clip)
	, fClip(clip)

	, fTextItems(16)

	, fText(clip ? clip->Text() : "")
	, fFont(clip ? clip->Font() : *be_bold_font)
	, fColor(clip ? clip->Color() : kWhite)
	, fUseOutline(clip ? clip->UseOutline() : false)
	, fOutlineColor(clip ? clip->OutlineColor() : kBlack)

	, fTextHeight()
	, fScrollingSpeed(kDefaultScrollingSpeed)
	, fTextPos(0.0)
	, fWidth(684.0)

	, fInitialScrollOffset(0.0)
	, fClipID("")
{
	if (fClip) {
		fClip->Acquire();

		fFont.SetSize(fClip->FontSize());
		fFont.SetSpacing(B_CHAR_SPACING);

		fFont.GetHeight(&fTextHeight);
		fScrollingSpeed = fClip->ScrollingSpeed();
		fWidth = fClip->Width();

		fClipID = fClip->ID();
		fInitialScrollOffset = sScrollOffsetManager.ScrollOffsetFor(fClipID,
			(bigtime_t)1000000 * fClip->ScrollOffsetResetTimeout());
	}
}

// destructor
ScrollingTextRenderer::~ScrollingTextRenderer()
{
	if (fClip)
		fClip->Release();
}

// Generate
status_t
ScrollingTextRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* playlistItem)
{
	BRect bounds = painter->Bounds();
	float bufferWidth = bounds.Width();

	painter->SetFont(&fFont);
	painter->SetSubpixelPrecise(true);
	painter->SetColor(fColor);

	// calculate scrolling offset from initial offset and frame
	fTextPos = fInitialScrollOffset
		- (frame * fScrollingSpeed / playlistItem->VideoFramesPerSecond());
	// cut of text items at the beginning if they scrolled out of view
	while (text_item* item = (text_item*)fTextItems.FirstItem()) {
		if (fTextPos + item->width < 0.0 && fTextItems.RemoveItem(item)) {
			fTextPos += item->width;
			delete item;
		} else
			break;
	}

	sScrollOffsetManager.UpdateScrollOffset(fClipID, fTextPos);

	// calculate constrain rect
	float height = fTextHeight.ascent + fTextHeight.descent;

	BRect constrainRect(0.0, 0.0, fWidth - 1.0, height - 1.0);
	BRect outlineConstrainRect(constrainRect.InsetByCopy(1.0, 0.0));
		// since outline gylphs are one pixel wider, they need to
		// disappear "earlier", in order to disappear at the same
		// time as the regular glyphs

	// draw text items
	BPoint offset;
	offset.y = floorf((constrainRect.top + constrainRect.bottom
		- height) / 2.0 + fTextHeight.ascent + 0.5);
//rgb_color contrast = fColor.red + fColor.green + fColor.blue > 128 * 3 ?
//	kBlack : kWhite;
	rgb_color contrast = fOutlineColor;
contrast.alpha = 120;

	float pos = fTextPos;
	int32 i = 0;
	while (pos < bufferWidth) {
		text_item* item = (text_item*)fTextItems.ItemAt(i++);
		if (item) {
			// use existing text items as long as there are some
		} else {
			// append new items as long as we need to fill up
			// the canvas width
			item = _AppendText();
			if (!item)
				break;
		}

		offset.x = pos;

		if (fUseOutline) {
			painter->SetColor(contrast);
			painter->SetFalseBoldWidth(1.0);
			painter->DrawString(item->text.String(), item->text.Length(),
				offset, NULL, outlineConstrainRect);
			
			painter->SetColor(fColor);
			painter->SetFalseBoldWidth(0.0);
		}
		painter->DrawString(item->text.String(), item->text.Length(), offset,
			NULL, constrainRect);

		pos += item->width;
	}

	return B_OK;
}

// Sync
void
ScrollingTextRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip) {
		fText = fClip->Text();
		Font font = fClip->Font();
		font.SetSize(fClip->FontSize());
		font.SetSpacing(B_CHAR_SPACING);
		if (font != fFont) {
			fFont = font;
			_RebuildTextItemWidth();
			fFont.GetHeight(&fTextHeight);
		}

		fColor = fClip->Color();
		fUseOutline = fClip->UseOutline();
		fOutlineColor = fClip->OutlineColor();
		fScrollingSpeed = fClip->ScrollingSpeed();
		fWidth = fClip->Width();
	}
}

// _AppendText
ScrollingTextRenderer::text_item*
ScrollingTextRenderer::_AppendText()
{
	// create a new text item and configure it
	text_item* item = new(nothrow) text_item;
	if (!item || !fTextItems.AddItem(item)) {
		delete item;
		return NULL;
	}

	item->text = fText;
	item->text << kSeparatorString;

	item->width = fFont.StringWidth(item->text.String()) + 10.0;

	return item;
}

// _RebuildTextItemWidth
void
ScrollingTextRenderer::_RebuildTextItemWidth()
{
	int32 count = fTextItems.CountItems();
	for (int32 i = 0; i < count; i++) {
		text_item* item = (text_item*)fTextItems.ItemAtFast(i);

		item->width = fFont.StringWidth(item->text.String()) + 10.0;
	}
}

