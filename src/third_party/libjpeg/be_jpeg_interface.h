/*
 * Copyright 2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 *		Marcin Konicki
 */

#ifndef JPEG_INTERFACE_H
#define JPEG_INTERFACE_H

//---------------------------------------------------
//	"Initializers" for jpeglib
//	based on default ones,
//	modified to work on BPositionIO instead of FILE
//---------------------------------------------------

// from "be_jdatasrc.cpp"
EXTERN (void)be_jpeg_stdio_src(j_decompress_ptr cinfo, BPositionIO* infile);
// from "be_jdatadst.cpp"
EXTERN (void)be_jpeg_stdio_dest(j_compress_ptr cinfo, BPositionIO* outfile);

//---------------------------------------------------
//	Error output functions
//	based on the one from jerror.c
//	modified to use settings
//	(so user can decide to show dialog-boxes or not)
//---------------------------------------------------
// from "be_jerror.cpp"
EXTERN (struct jpeg_error_mgr*)be_jpeg_std_error(struct jpeg_error_mgr* err);

#endif // JPEG_INTERFACE_H
