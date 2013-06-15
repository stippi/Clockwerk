/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "RenderingBuffer.h"

#include <stdio.h>

#include "support_ui.h"

void
RenderingBuffer::PrintToStream() const
{
	printf("RenderingBuffer:\n");
	printf("  PixelFormat(): %s\n", string_for_color_space(
		(color_space)PixelFormat()));
	printf("         Bits(): %p\n", Bits());
	printf("  BytesPerRow(): %ld\n", BytesPerRow());
	printf("        Width(): %ld\n", Width());
	printf("       Height(): %ld\n", Height());
}
