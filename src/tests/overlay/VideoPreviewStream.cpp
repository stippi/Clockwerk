/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "VideoPreviewStream.h"

#include <new>
#include <stdio.h>

#include <Bitmap.h>
#include <Entry.h>
#include <File.h>
#include <String.h>

//#include "common.h"

#include <jpeglib.h>
#include <be_jpeg_interface.h>

//include "VideoRenderer.h"
#include "VideoClip.h"

using std::nothrow;

struct stream_header {
	uint32		width;
	uint32		height;
	color_space	format;
	int64		frameCount;
};

// constructor
VideoPreviewStream::VideoPreviewStream()
	: fFormat(),
	  fCurrentFrame(-1),
	  fFrameCount(0),
	  fStream(NULL),
	  fFrameOffsetMap(NULL),
	  fDecodedBitmap(NULL)
{
}

// destructor
VideoPreviewStream::~VideoPreviewStream()
{
	_MakeEmpty();
}

// Init
status_t
VideoPreviewStream::Init(const BString& id)
{
	// clear
	_MakeEmpty();

	// construct filename and get entry_ref
	entry_ref ref;
	status_t ret = _GetRefForID(id, &ref);
	if (ret < B_OK)
		return ret;

	// open file
	fStream = new BFile(&ref, B_READ_ONLY);
	ret = fStream->InitCheck();
	if (ret < B_OK) {
		_MakeEmpty();
		return ret;
	}

	// read stream header (format, frame count)
	stream_header header;
	if (fStream->Read(&header, sizeof(header)) != sizeof(header)) {
		_MakeEmpty();
		return B_IO_ERROR;
	}

	if (header.width == 0 || header.width > 1920
		|| header.height == 0 || header.height > 1080
		|| header.format != B_YCbCr422) {
		_MakeEmpty();
		return B_ERROR;
	}

	fFormat.u.raw_video.display.line_width = header.width;
	fFormat.u.raw_video.display.line_count = header.height;
	fFormat.u.raw_video.display.format = header.format;
	fFormat.u.raw_video.display.bytes_per_row = 2 * header.width;
	fFormat.u.raw_video.display.bytes_per_row += (2 * header.width) % 4;

	// init frame->offset map
	fFrameCount = header.frameCount;
	fFrameOffsetMap = new (nothrow) off_t[fFrameCount];
	if (!fFrameOffsetMap) {
		_MakeEmpty();
		return B_NO_MEMORY;
	}

	// read frame->offset map from file
	ssize_t mapSize = sizeof(off_t) * fFrameCount;
	if (fStream->Read(fFrameOffsetMap, mapSize) != mapSize) {
		_MakeEmpty();
		return B_IO_ERROR;
	}

	return ret;
}

// GetFormat
status_t
VideoPreviewStream::GetFormat(media_format* format)
{
	if (!fStream || !fFrameOffsetMap)
		return B_NO_INIT;
	if (!format)
		return B_BAD_VALUE;

	memcpy(format, &fFormat, sizeof(media_format));
	return B_OK;
}

// SeekToFrame
status_t
VideoPreviewStream::SeekToFrame(int64 frame)
{
	if (!fStream || !fFrameOffsetMap)
		return B_NO_INIT;
	if (frame < 0 || frame >= fFrameCount)
		return B_BAD_VALUE;

 	off_t offset = fFrameOffsetMap[(int32)frame];
	if (fStream->Seek(offset, SEEK_SET) != offset)
		return B_IO_ERROR;

	fCurrentFrame = frame;

	return B_OK;
}

// #pragma mark -

// ReadFrame
status_t
VideoPreviewStream::ReadFrame(uint8* buffer)
{
	if (!buffer)
		return B_BAD_VALUE;
	if (!fStream || !fFrameOffsetMap)
		return B_NO_INIT;
	if (fCurrentFrame < 0 || fCurrentFrame >= fFrameCount)
		return B_ERROR;

	_CheckStreamPosition();

	// decode into provided frameBuffer
	_DecodeBuffer(buffer, fFormat.u.raw_video.display.line_width,
		fFormat.u.raw_video.display.line_count,
		fFormat.u.raw_video.display.bytes_per_row, *fStream);

	// ready to read next frame
	fCurrentFrame++;

	return B_OK;
}

// ReadFrame
const BBitmap*
VideoPreviewStream::ReadFrame()
{
	if (!fStream || !fFrameOffsetMap) {
		printf("VideoPreviewStream::ReadFrame() - no init\n");
		return NULL;
	}

	if (fCurrentFrame < 0 || fCurrentFrame >= fFrameCount)
		fCurrentFrame = 0;

	_CheckStreamPosition();

	// create bitmap if necessary
	if (!fDecodedBitmap) {
		fDecodedBitmap = new BBitmap(BRect(0, 0,
			fFormat.u.raw_video.display.line_width - 1,
			fFormat.u.raw_video.display.line_count - 1), 0, B_YCbCr422);
		if (fDecodedBitmap->InitCheck() < B_OK) {
			printf("VideoPreviewStream::ReadFrame() - creating bitmap failed!\n");
			delete fDecodedBitmap;
			fDecodedBitmap = NULL;
			return NULL;
		}
	}

	status_t ret = _DecodeBitmap(fDecodedBitmap, *fStream);
	if (ret < B_OK) {
		printf("VideoPreviewStream::ReadFrame() - decoding: %s\n", strerror(ret));
	}

	// ready to read next frame
	fCurrentFrame++;

	return fDecodedBitmap;
}

// #pragma mark -

#define TIMING 1

// CreateStream
/*static*/ status_t
VideoPreviewStream::CreateStream(const BString& serverID,
//	VideoRenderer* renderer)
	VideoClip* renderer)
{
	#if TIMING
		bigtime_t startTime = system_time();
	#endif

	// open/create output file
	entry_ref ref;
	status_t ret = _GetRefForID(serverID, &ref);
	if (ret < B_OK)
		return ret;

	BFile file(&ref, B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	ret = file.InitCheck();
	if (ret < B_OK)
		return ret;

	// seek to clip start
	int64 frame = 0;
	ret = renderer->SeekToFrame(&frame);
	if (ret < B_OK)
		return ret;

	// write stream header
	media_format format;
	ret = renderer->GetFormat(&format);
	if (ret < B_OK)
		return ret;

	stream_header header;
	header.width = format.u.raw_video.display.line_width;
	header.height = format.u.raw_video.display.line_count;
	header.format = format.u.raw_video.display.format;
	header.frameCount = renderer->CountFrames();

	ssize_t written = file.Write(&header, sizeof(header));
	if (written != (ssize_t)sizeof(header)) {
		if (written < 0)
			return (status_t)written;
		return B_IO_ERROR;
	}

	// zero out frame->offset map area
	off_t frameOffsetMap[header.frameCount];
	memset(&frameOffsetMap, 0, sizeof(frameOffsetMap));
	written = file.Write(frameOffsetMap, sizeof(frameOffsetMap));
	if (written != (ssize_t)sizeof(frameOffsetMap)) {
		if (written < 0)
			return (status_t)written;
		return B_IO_ERROR;
	}

	#if TIMING
		bigtime_t decodingTime = 0;
		bigtime_t encodingTime = 0;
		bigtime_t decodingStart = system_time();
	#endif

	// write stream data
	for (frame = 0; frame < header.frameCount; frame++) {
		const BBitmap* bitmap = renderer->ReadFrame();
		if (!bitmap)
			return B_ERROR;

		#if TIMING
			bigtime_t decodingEnd = system_time();
		#endif

		frameOffsetMap[(int32)frame] = file.Position();
//		if (frame == 0) {
//			BFile test("/boot/home/Desktop/test.jpg",
//				B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
//			ret = _EncodeBitmap(bitmap, test);
//		}
		ret = _EncodeBitmap(bitmap, file);
		if (ret < B_OK)
			return ret;

		#if TIMING
			decodingTime += decodingEnd - decodingStart;
			decodingStart = system_time();
			encodingTime += decodingStart - decodingEnd;
		#endif
	}

	#if TIMING
		printf("decoding time: %lld µs/frame\n", decodingTime / header.frameCount);
		printf("encoding time: %lld µs/frame\n", encodingTime / header.frameCount);
	#endif

	// seek to position after header, write frame->offset map
	if (file.Seek(sizeof(stream_header), SEEK_SET) != sizeof(stream_header))
		return B_IO_ERROR;
	written = file.Write(frameOffsetMap, sizeof(frameOffsetMap));
	if (written != (ssize_t)sizeof(frameOffsetMap)) {
		if (written < 0)
			return (status_t)written;
		return B_IO_ERROR;
	}

	#if TIMING
		printf("total time: %lld µs for %lld frames\n",
			system_time() - startTime, header.frameCount);
	#endif

	return B_OK;
}

// #pragma mark -

void
VideoPreviewStream::_MakeEmpty()
{
	fCurrentFrame = -1;
	fFrameCount = 0;

	delete fStream;
	fStream = NULL;

	delete[] fFrameOffsetMap;
	fFrameOffsetMap = NULL;

	delete fDecodedBitmap;
	fDecodedBitmap = NULL;
}

// _GetRefForID
/*static*/ status_t
VideoPreviewStream::_GetRefForID(const BString& serverID, entry_ref* ref)
{
	BString filename(serverID);
	filename << "_preview_stream";

//	return = get_ref_for_id(filename, ref);
	return get_ref_for_path(filename.String(), ref);
}

// create_planar_image_buffer
template<typename jpeg_info>
static JSAMPIMAGE
create_planar_image_buffer(jpeg_info& cinfo)
{
	int32 ySamplesPerRow = cinfo.comp_info[0].width_in_data_units * DCTSIZE;
	int32 cSamplesPerRow = cinfo.comp_info[1].width_in_data_units * DCTSIZE;
	int32 rowsPerWriteCall = cinfo.max_v_samp_factor * DCTSIZE;

	// use jpeglib terminology for sample, row and image
	int32 yBufferSize = rowsPerWriteCall * ySamplesPerRow;
	int32 cBufferSize = rowsPerWriteCall * cSamplesPerRow;
	JSAMPROW yBuffer = new JSAMPLE[yBufferSize];
	JSAMPROW cbBuffer = new JSAMPLE[cBufferSize];
	JSAMPROW crBuffer = new JSAMPLE[cBufferSize];

	JSAMPARRAY yPlane = new JSAMPROW[cinfo.comp_info[0].v_samp_factor * DCTSIZE];
	JSAMPARRAY cbPlane = new JSAMPROW[cinfo.comp_info[1].v_samp_factor * DCTSIZE];
	JSAMPARRAY crPlane = new JSAMPROW[cinfo.comp_info[2].v_samp_factor * DCTSIZE];

	// assign pointers
	for (int32 row = 0; row < rowsPerWriteCall; row++) {
		yPlane[row] = yBuffer + row * ySamplesPerRow;
		cbPlane[row] = cbBuffer + row * cSamplesPerRow;
		crPlane[row] = crBuffer + row * cSamplesPerRow;
	}

	JSAMPIMAGE imageData = new JSAMPARRAY[3];
	imageData[0] = yPlane;
	imageData[1] = cbPlane;
	imageData[2] = crPlane;

	return imageData;
}

// _EncodeBitmap
/*static*/ status_t
VideoPreviewStream::_EncodeBitmap(const BBitmap* bitmap, BFile& file)
{
#if 1
	uint8* bits = (uint8*)bitmap->Bits();
	int32 width = bitmap->Bounds().IntegerWidth() + 1;
	int32 height = bitmap->Bounds().IntegerHeight() + 1;
	int32 srcBPR = bitmap->BytesPerRow();

	// init jpeg writing
	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = be_jpeg_std_error(&jerr);
	jpeg_create_compress(&cinfo);
	be_jpeg_stdio_dest(&cinfo, &file);

	// basic setup
	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_YCbCr;
	jpeg_set_defaults(&cinfo);

	// additional settings
	cinfo.dct_method = JDCT_FASTEST;
	jpeg_set_quality(&cinfo, 50, true);

	// caused additional pass over image data to compute optimal Huffman tables
	// for smaller file sizes, cost is in speed
//	cinfo.optimize_coding = true;
//	cinfo.smoothing_factor = false;
//	cinfo.write_JFIF_header = false; // ?!?

	// configure "raw" input
	cinfo.raw_data_in = true;
	// Y sampling factor
	cinfo.comp_info[0].h_samp_factor = 2;
	cinfo.comp_info[0].v_samp_factor = 2;
	// Cb sampling factor
	cinfo.comp_info[1].h_samp_factor = 1;
	cinfo.comp_info[1].v_samp_factor = 2;
	// Cr sampling factor
	cinfo.comp_info[2].h_samp_factor = 1;
	cinfo.comp_info[2].v_samp_factor = 2;

	// initialize compression cycle
	// this will also compute the image dimensions, number of blocks etc
	// "true" means "complete interchange datestream"
	// "false" means "abbreviated datastream"
	jpeg_start_compress(&cinfo, true);

	int32 rowsPerWriteCall = cinfo.max_v_samp_factor * DCTSIZE;

/*	int32 ySamplesPerRow = cinfo.comp_info[0].width_in_data_units * DCTSIZE;
	int32 cSamplesPerRow = cinfo.comp_info[1].width_in_data_units * DCTSIZE;
	int32 rowsPerWriteCall = cinfo.max_v_samp_factor * DCTSIZE;

	// use jpeglib terminology for sample, row and image
	int32 yBufferSize = rowsPerWriteCall * ySamplesPerRow;
	int32 cBufferSize = rowsPerWriteCall * cSamplesPerRow;
	JSAMPROW yBuffer = new JSAMPLE[yBufferSize];
	JSAMPROW cbBuffer = new JSAMPLE[cBufferSize];
	JSAMPROW crBuffer = new JSAMPLE[cBufferSize];

	JSAMPARRAY yPlane = new JSAMPROW[cinfo.comp_info[0].v_samp_factor * DCTSIZE];
	JSAMPARRAY cbPlane = new JSAMPROW[cinfo.comp_info[1].v_samp_factor * DCTSIZE];
	JSAMPARRAY crPlane = new JSAMPROW[cinfo.comp_info[2].v_samp_factor * DCTSIZE];

	// assign pointers
	for (int32 row = 0; row < rowsPerWriteCall; row++) {
		yPlane[row] = yBuffer + row * ySamplesPerRow;
		cbPlane[row] = cbBuffer + row * cSamplesPerRow;
		crPlane[row] = crBuffer + row * cSamplesPerRow;
	}

	JSAMPIMAGE imageData = new JSAMPARRAY[3];
	imageData[0] = yPlane;
	imageData[1] = cbPlane;
	imageData[2] = crPlane;*/
	JSAMPIMAGE imageData = create_planar_image_buffer(cinfo);

	// iterate over scanlines and compress
	while (cinfo.next_scanline < cinfo.image_height) {
		// convert chunky YCbCr into planar YCbCr
		for (int32 row = 0; row < rowsPerWriteCall; row++) {
			// don't read more data than available
			if ((int32)cinfo.next_scanline + row >= height)
				break;
			// pointers to source and dest buffers
			uint8* src = bits + row * srcBPR;
			uint8* yDest = imageData[0][row];
			uint8* cbDest = imageData[1][row];
			uint8* crDest = imageData[2][row];
			// handle 2 pixels at a time
			for (int32 x = 0; x < width / 2; x++) {
				yDest[0] = src[0];
				cbDest[0] = src[1];
				yDest[1] = src[2];
				crDest[0] = src[3];
				yDest += 2;
				cbDest += 1;
				crDest += 1;
				src += 4;
			}
			// handle last pixel if width is odd
			if (width & 1) {
				yDest[0] = src[0];
				cbDest[0] = src[1];
			}
		}
		// processes one "MCU row" per call
		// (v_samp_factor*DCTSIZE sample rows of each component)
		jpeg_write_raw_data(&cinfo, imageData, rowsPerWriteCall);

		// point to next scanline of input data
		bits += srcBPR * rowsPerWriteCall;
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	// delete buffers
	// TODO: keep these around
	delete[] imageData[0][0];
	delete[] imageData[1][0];
	delete[] imageData[2][0];

	delete[] imageData[0];
	delete[] imageData[1];
	delete[] imageData[2];

	delete[] imageData;

#else
	ssize_t written = file.Write(bitmap->Bits(), bitmap->BitsLength());
	if (written != bitmap->BitsLength()) {
		if (written < 0)
			return (status_t)written;
		return B_IO_ERROR;
	}
#endif

	return B_OK;
}

// _DecodeBitmap
status_t
VideoPreviewStream::_DecodeBitmap(const BBitmap* bitmap, BFile& file)
{
	uint8* bits = (uint8*)bitmap->Bits();
	uint32 width = bitmap->Bounds().IntegerWidth() + 1;
	uint32 height = bitmap->Bounds().IntegerHeight() + 1;
	int32 dstBPR = bitmap->BytesPerRow();
	return _DecodeBuffer(bits, width, height, dstBPR, file);
}

// _DecodeBuffer
status_t
VideoPreviewStream::_DecodeBuffer(uint8* bits,
	uint32 width, uint32 height, uint32 dstBPR, BFile& file)
{
	// init jpeg reading
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;
	cinfo.err = be_jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);
	be_jpeg_stdio_src(&cinfo, &file);

	// read info about image
	jpeg_read_header(&cinfo, true);

	// configure output colorspace
	cinfo.out_color_space = JCS_YCbCr;
	cinfo.output_components = 3;

	// configure "raw" output
	cinfo.raw_data_out = true;

	// initialize decompression
	jpeg_start_decompress(&cinfo);

	// check if this frame has correct settings
	if (cinfo.output_height != height || cinfo.output_width != width) {
		printf("JPEG frame and stream header incompatible (image size)\n");
		printf("  width: %ld/%d\n", width, cinfo.output_width);
		printf("  height: %ld/%d\n", height, cinfo.output_height);
		jpeg_destroy_decompress(&cinfo);
		return B_MISMATCHED_VALUES;
	}

	if (cinfo.comp_info[0].h_samp_factor != 2
		|| cinfo.comp_info[0].v_samp_factor != 2
		|| cinfo.comp_info[1].h_samp_factor != 1
		|| cinfo.comp_info[1].v_samp_factor != 2
		|| cinfo.comp_info[2].h_samp_factor != 1
		|| cinfo.comp_info[2].v_samp_factor != 2) {
		printf("JPEG color space incompatible\n");
		jpeg_destroy_decompress(&cinfo);
		return B_MISMATCHED_VALUES;
	}

	JSAMPIMAGE imageData = create_planar_image_buffer(cinfo);
	int32 rowsPerReadCall = cinfo.max_v_samp_factor * DCTSIZE;
	uint32 rowsRead = 0;

	while (cinfo.output_scanline < cinfo.output_height) {
		// decompress rowsPerBlock number of scanlines
		// processes one "MCU row" per call
		jpeg_read_raw_data(&cinfo, imageData, rowsPerReadCall);

		// convert planaer YCbCr into chunky YCbCr
		for (int32 row = 0; row < rowsPerReadCall; row++, rowsRead++) {
			// don't write more data than available
			if (rowsRead >= height)
				break;
			// pointers to source and dest buffers
			uint8* dst = bits + row * dstBPR;
			uint8* ySrc = imageData[0][row];
			uint8* cbSrc = imageData[1][row];
			uint8* crSrc = imageData[2][row];
			// handle 2 pixels at a time
			for (uint32 x = 0; x < width / 2; x++) {
				dst[0] = ySrc[0];
				dst[1] = cbSrc[0];
				dst[2] = ySrc[1];
				dst[3] = crSrc[0];
				ySrc += 2;
				cbSrc += 1;
				crSrc += 1;
				dst += 4;
			}
			// handle last pixel if width is odd
			if (width & 1) {
				dst[0] = ySrc[0];
				dst[1] = cbSrc[0];
			}
		}

		// point to next scanline of output data
		bits += dstBPR * rowsPerReadCall;
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	// delete buffers
	// TODO: keep these around
	delete[] imageData[0][0];
	delete[] imageData[1][0];
	delete[] imageData[2][0];

	delete[] imageData[0];
	delete[] imageData[1];
	delete[] imageData[2];

	delete[] imageData;

	return B_OK;
}

// _CheckStreamPosition
void
VideoPreviewStream::_CheckStreamPosition()
{
	off_t position = fFrameOffsetMap[(int32)fCurrentFrame];
	if (fStream->Position() != position) {
		SeekToFrame(fCurrentFrame);
	}
}
