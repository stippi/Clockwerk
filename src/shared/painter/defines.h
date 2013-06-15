/*
 * Copyright 2005-2006, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * global definitions for the Painter frame work, mainly types for the
 * AGG pipelines
 *
 */

#ifndef DEFINES_H
#define DEFINES_H

#include <agg_pixfmt_rgba.h>
#include <agg_rasterizer_scanline_aa.h>
#include <agg_renderer_primitives.h>
#include <agg_renderer_scanline.h>
#include <agg_scanline_bin.h>
#include <agg_scanline_p.h>
#include <agg_scanline_u.h>
#include <agg_renderer_base.h>
#include <agg_rendering_buffer.h>

#include "agg_pixfmt_ycbcr422.h"
#include "agg_pixfmt_ycbcr444.h"
#include "agg_pixfmt_ycbcra.h"

#define ALIASED_DRAWING 0

	typedef agg::pixfmt_bgra32									pixfmt;
	typedef agg::renderer_base<pixfmt>							renderer_base;
	typedef agg::pixfmt_bgra32_pre								pixfmt_pre;
	typedef agg::renderer_base<pixfmt_pre>						renderer_base_pre;

	typedef agg::pixfmt_ycbcr444								pixfmt_ycc;
	typedef agg::renderer_base<pixfmt_ycc>						renderer_base_ycc;
	typedef agg::pixfmt_ycbcr444_pre							pixfmt_pre_ycc;
	typedef agg::renderer_base<pixfmt_pre_ycc>					renderer_base_pre_ycc;

#if ALIASED_DRAWING
	typedef agg::scanline_bin									scanline_unpacked_type;
	typedef agg::scanline_bin									scanline_packed_type;
	typedef agg::renderer_scanline_bin_solid<renderer_base>		renderer_type;
	typedef agg::renderer_scanline_bin_solid<renderer_base_ycc>	renderer_type_ycc;
#else
	typedef agg::scanline_u8									scanline_unpacked_type;
	typedef agg::scanline_p8									scanline_packed_type;
	typedef agg::renderer_scanline_aa_solid<renderer_base>		renderer_type;
	typedef agg::renderer_scanline_aa_solid<renderer_base_ycc>	renderer_type_ycc;
#endif

	typedef agg::renderer_scanline_aa_solid<renderer_base>		font_renderer_solid_type;
	typedef agg::renderer_scanline_bin_solid<renderer_base>		font_renderer_bin_type;
	typedef agg::renderer_scanline_aa_solid<renderer_base_ycc>	font_renderer_solid_type_ycc;
	typedef agg::renderer_scanline_bin_solid<renderer_base_ycc>	font_renderer_bin_type_ycc;

	typedef agg::rasterizer_scanline_aa<>						rasterizer_type;

#endif // DEFINES_H


