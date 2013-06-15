/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RenderPlaylistItem.h"

#include <new>
#include <stdio.h>

#include <Region.h>

#include "support.h"

#include "AdvancedTransform.h"
#include "BitmapClip.h"
#include "BitmapRenderer.h"
#include "ClipPlaylistItem.h"
#include "ClipRenderer.h"
#include "ClipRendererCache.h"
#include "ClockClip.h"
#include "ClockRenderer.h"
#include "ColorClip.h"
#include "ColorRenderer.h"
#include "ExecuteClip.h"
#include "ExecuteRenderer.h"
#include "FileBasedClip.h"
#include "MediaClip.h"
#include "Painter.h"
#include "Playlist.h"
#include "PlaylistClipRenderer.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "ScrollingTextClip.h"
#include "ScrollingTextRenderer.h"
#include "StaticTextRenderer.h"
#include "TableClip.h"
#include "TableRenderer.h"
#include "TextClip.h"
#include "TimerClip.h"
#include "TimerRenderer.h"
#include "VideoRenderer.h"
#include "WeatherClip.h"
#include "WeatherRenderer.h"

using std::nothrow;

// constructor 
RenderPlaylistItem::RenderPlaylistItem(PlaylistItem* other, double frame,
		color_space format, ClipRendererCache* rendererCache)
	: PlaylistItem(*other)
	, fOriginalItem(other)
	, fRenderer(NULL)
	, fAlpha(1.0)
	, fTransformation()
{
	// get the animated values of the known properties at the "frame"
	frame -= StartFrame();
	fAlpha = _ValueAt(other->AlphaAnimator()->Property(), frame, 1.0);

	AdvancedTransform transform;
	transform.SetTransformation(
		BPoint(_ValueAt(other->PivotX(), frame, 0.0),
			   _ValueAt(other->PivotY(), frame, 0.0)),
		BPoint(_ValueAt(other->TranslationX(), frame, 0.0),
			   _ValueAt(other->TranslationY(), frame, 0.0)),
		_ValueAt(other->Rotation(), frame, 0.0),
		_ValueAt(other->ScaleX(), frame, 1.0),
		_ValueAt(other->ScaleY(), frame, 1.0));
	fTransformation = transform;

	// create a renderer if there is not already one
	if (fOriginalItem->HasVideo()) {
		ClipRenderer* renderer = rendererCache->RendererFor(fOriginalItem);
		if (!renderer || renderer->NeedsReload()) {
			if (renderer)
				rendererCache->RemoveRendererFor(fOriginalItem);
			_CreateRenderer(format, rendererCache);
		} else {
			fRenderer = renderer;
			fRenderer->Acquire();
			fRenderer->Sync();
		}
	}
}

// destructor
RenderPlaylistItem::~RenderPlaylistItem()
{
	if (fRenderer)
		fRenderer->Release();
}

// #pragma mark -

PlaylistItem*
RenderPlaylistItem::Clone(bool deep) const
{
	// NOTE: not supposed to be cloned
	return NULL;
}

// #pragma mark -

// HasVideo
bool
RenderPlaylistItem::HasVideo()
{
	return fOriginalItem->HasVideo();
}

// Generate
bool
RenderPlaylistItem::Generate(Painter* painter, double frame)
{
	if (!fRenderer)
		return false;

	frame -= StartFrame();
	if (frame >= 0 && (uint64)frame < Duration()) {
		return fRenderer->Generate(painter, frame + ClipOffset(), this) >= B_OK;
	}

	return false;
}

// HasAudio
bool
RenderPlaylistItem::HasAudio()
{
	return fOriginalItem->HasAudio();
}

// CreateAudioReader
AudioReader*
RenderPlaylistItem::CreateAudioReader()
{
	return fOriginalItem->CreateAudioReader();
}

// #pragma mark -

BRect
RenderPlaylistItem::Bounds(BRect canvasBounds, bool transformed)
{
	return fOriginalItem->Bounds(canvasBounds, transformed);
}

// RemoveSolidRegion
void
RenderPlaylistItem::RemoveSolidRegion(BRegion* cleanBG, Painter* painter, double frame)
{
	if (!fRenderer->IsSolid(frame))
		return;

	// get the untransformed bounds (this function will only be called
	// with a painter already setup for this items transformation)
	BRect bounds = Bounds(painter->Bounds(), false);

	BPoint lt = bounds.LeftTop();
	BPoint rt = bounds.RightTop();
	BPoint lb = bounds.LeftBottom();
	BPoint rb = bounds.RightBottom();

	AffineTransform transform = painter->Transformation();

	transform.Transform(&lt);
	transform.Transform(&rt);
	transform.Transform(&lb);
	transform.Transform(&rb);

	BRect solid;

	if (lt.y == rt.y || lt.x == rt.x) {
		// the transformed bounds is "straight"
		// round towards the "inside" pixels
		solid.left = ceilf(min4(lt.x, rt.x, lb.x, rb.x));
		solid.top = ceilf(min4(lt.y, rt.y, lb.y, rb.y));
		solid.right = floorf(max4(lt.x, rt.x, lb.x, rb.x));
		solid.bottom = floorf(max4(lt.y, rt.y, lb.y, rb.y));
	} else {
		// TODO: add a couple of smaller "straight" rectangles within the
		// transformed bounds rectangle
		return;
	}

	cleanBG->Exclude(solid);
}

// Name
BString
RenderPlaylistItem::Name() const
{
	return fOriginalItem->Name();
}

// #pragma mark -

// Transformation
AffineTransform
RenderPlaylistItem::Transformation() const
{
	return fTransformation;
}

// Alpha
float
RenderPlaylistItem::Alpha() const
{
	return fAlpha;
}

// #pragma mark -

void
RenderPlaylistItem::_CreateRenderer(color_space format,
	ClipRendererCache* rendererCache)
{
	ClipPlaylistItem* clipItem = dynamic_cast<ClipPlaylistItem*>(fOriginalItem);
	if (!clipItem)
		return;

	Clip* clip = clipItem->Clip();
	if (!clip)
		return;

	ClipRenderer* renderer;

	if (ColorClip* colorClip
		= dynamic_cast<ColorClip*>(clip)) {
		renderer = new (nothrow) ColorRenderer(clipItem, colorClip);
	} else if (BitmapClip* bitmapClip
		= dynamic_cast<BitmapClip*>(clip)) {
		renderer = new (nothrow) BitmapRenderer(clipItem,
											    bitmapClip, format);
	} else if (MediaClip* mediaClip
		= dynamic_cast<MediaClip*>(clip)) {
		renderer = new (nothrow) VideoRenderer(clipItem,
											   mediaClip, format);
	} else if (Playlist* playlist
		= dynamic_cast<Playlist*>(clip)) {
		renderer = new (nothrow) PlaylistClipRenderer(clipItem, playlist,
			rendererCache);
	} else if (ScrollingTextClip* tickerClip
		= dynamic_cast<ScrollingTextClip*>(clip)) {
		renderer = new (nothrow) ScrollingTextRenderer(clipItem,
													   tickerClip);
	} else if (TextClip* textClip
		= dynamic_cast<TextClip*>(clip)) {
		renderer = new (nothrow) StaticTextRenderer(clipItem, textClip);
	} else if (TableClip* tableClip
		= dynamic_cast<TableClip*>(clip)) {
		renderer = new (nothrow) TableRenderer(clipItem, tableClip);
	} else if (ClockClip* clockClip
		= dynamic_cast<ClockClip*>(clip)) {
		renderer = new (nothrow) ClockRenderer(clipItem, clockClip);
	} else if (TimerClip* timerClip
		= dynamic_cast<TimerClip*>(clip)) {
		renderer = new (nothrow) TimerRenderer(clipItem, timerClip);
	} else if (WeatherClip* weatherClip
		= dynamic_cast<WeatherClip*>(clip)) {
		renderer = new (nothrow) WeatherRenderer(clipItem, weatherClip);
	} else if (ExecuteClip* executeClip
		= dynamic_cast<ExecuteClip*>(clip)) {
		renderer = new (nothrow) ExecuteRenderer(clipItem, executeClip);
	} else {
		// fall back to using a dummy ClipRenderer (doesn't render anything)
		renderer = new (nothrow) ClipRenderer(clipItem, clip);
	}

	if (!renderer
		|| !rendererCache->AddRenderer(renderer, clipItem)) {
		printf("RenderPlaylistItem::_CreateRenderer() - "
			"no memory to add renderer\n");
		delete renderer;
		return;
	}

	renderer->Acquire();
	renderer->Sync();

	fRenderer = renderer;
}

// #pragma mark -

// _ValueAt
float
RenderPlaylistItem::_ValueAt(const Property* property, double frame,
							 float defaultValue) const
{
	const FloatProperty* floatProperty
		= dynamic_cast<const FloatProperty*>(property);

	if (!floatProperty)
		return defaultValue;

	if (PropertyAnimator* animator = property->Animator()) {
		FloatProperty f(*floatProperty, false);
		animator->AnimatePropertyTo(&f, frame);
		return f.Value();
	}

	return floatProperty->Value();
}

