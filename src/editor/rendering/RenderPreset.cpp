/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RenderPreset.h"

#include <stdio.h>

#include <Message.h>

//#include "XMLHelper.h"

// debugging
#include "Debug.h"
#define ldebug	debug
//#define ldebug	nodebug

// default values
static const uint32 		kDefaultLineWidth			= 320;
static const uint32 		kDefaultLineCount			= 240;
static const color_space	kDefaultColorSpace			= B_RGB32;
static const char*			kDefaultName				= "Render Preset";
static const char*			kDefaultCopyright			= "© 2009";
static const char*			kDefaultFormatFamily		= "AVI (Audio Video Interleaved)";
static const char*			kDefaultVideoCodec			= "MPEG4";
static const char*			kDefaultAudioCodec			= "Raw Audio";
static const float			kDefaultQuality				= 1.00;
static const bool			kDefaultVideoOn				= true;
static const bool			kDefaultAudioOn				= true;
static const bool			kDefaultRenderPreview		= false;
static const bool			kDefaultUseAlpha			= false;
static const float			kDefaultVideoFrameRate		= 25.0;
static const float			kDefaultAudioFrameRate		= 44100.0;
static const uint32			kDefaultAudioChannelCount	= 2;
static const bool			kDefaultRenderTimeCode		= false;
static const float			kDefaultTCTransparency		= 0.7;
static const float			kDefaultTCScale				= 1.0;

// TODO:
// there are two flags, "render preview" and "use alpha"
// this sucks, there should only be one enumerator,
// but I hacked this in, and it is backwards compatible this way
// maybe there is a smarter idea


// constructor
RenderPreset::RenderPreset()
	: fLineWidth(kDefaultLineWidth),
	  fLineCount(kDefaultLineCount),
	  fColorSpace(kDefaultColorSpace),
	  fName(kDefaultName),
	  fCopyright(kDefaultCopyright),
	  fFamilyName(kDefaultFormatFamily),
	  fVideoCodecName(kDefaultVideoCodec),
	  fAudioCodecName(kDefaultAudioCodec),
	  fVideoQuality(kDefaultQuality),
	  fAudioQuality(kDefaultQuality),
	  fHasVideoTrack(kDefaultVideoOn),
	  fHasAudioTrack(kDefaultAudioOn),
	  fRenderPreview(kDefaultRenderPreview),
	  fUseAlpha(kDefaultUseAlpha),
	  fTimeCodeOverlay(kDefaultRenderTimeCode),
	  fTimeCodeTransparency(kDefaultTCTransparency),
	  fTimeCodeScale(kDefaultTCScale),
	  fVideoFrameRate(kDefaultVideoFrameRate),
	  fAudioFrameRate(kDefaultAudioFrameRate),
	  fAudioChannelCount(kDefaultAudioChannelCount)
{
}

// constructor
RenderPreset::RenderPreset(const char* name, uint32 lineWidth,
		uint32 lineCount, color_space format, float quality, bool video,
		bool audio, bool renderPreview, bool useAlpha, const char* copyright)
	:
	fLineWidth(lineWidth),
	fLineCount(lineCount),
	fColorSpace(format),
	fName(name),
	fCopyright(copyright),
	fFamilyName(kDefaultFormatFamily),
	fVideoCodecName(kDefaultVideoCodec),
	fAudioCodecName(kDefaultAudioCodec),
	fVideoQuality(quality),
	fHasVideoTrack(video),
	fHasAudioTrack(audio),
	fRenderPreview(renderPreview),
	fUseAlpha(useAlpha),
	fTimeCodeOverlay(kDefaultRenderTimeCode),
	fTimeCodeTransparency(kDefaultTCTransparency),
	fTimeCodeScale(kDefaultTCScale),
	fAudioFrameRate(kDefaultAudioFrameRate),
	fAudioChannelCount(kDefaultAudioChannelCount)
{
}

// copy constructor
RenderPreset::RenderPreset(const char* name, RenderPreset* preset)
	:
	fName(name)
{
	AdoptValues(*preset);
}

// destructor
RenderPreset::~RenderPreset()
{
}

#if 0
// XMLStore
status_t
RenderPreset::XMLStore(XMLHelper& xmlHelper) const
{
	status_t error = B_OK;
	xmlHelper.SetAttribute("name", Name());
	xmlHelper.SetAttribute("preview", fRenderPreview);
	xmlHelper.SetAttribute("use_alpha", fUseAlpha);
	if ((error = xmlHelper.CreateTag("FILE_FORMAT")) == B_OK) {
		xmlHelper.SetAttribute("name", FormatFamily());
		xmlHelper.CloseTag();	// FILE_FORMAT
	}
	if ((error = xmlHelper.CreateTag("VIDEO")) == B_OK) {
		xmlHelper.SetAttribute("on", fHasVideoTrack);
		if ((error = xmlHelper.CreateTag("VIDEO_FORMAT")) == B_OK) {
			xmlHelper.SetAttribute("line_width", fLineWidth);
			xmlHelper.SetAttribute("line_count", fLineCount);
			xmlHelper.SetAttribute("color_space", (int32)fColorSpace);
			xmlHelper.CloseTag();	// VIDEO_FORMAT
		}
		if ((error = xmlHelper.CreateTag("VIDEO_CODEC")) == B_OK) {
			xmlHelper.SetAttribute("name", fVideoCodecName);
			xmlHelper.CloseTag();	// VIDEO_CODEC
		}
		if ((error = xmlHelper.CreateTag("QUALITY")) == B_OK) {
			xmlHelper.SetAttribute("value", fVideoQuality);
			xmlHelper.CloseTag();	// QUALITY
		}
		xmlHelper.CloseTag();	// VIDEO
	}
	if ((error = xmlHelper.CreateTag("AUDIO")) == B_OK) {
		xmlHelper.SetAttribute("on", fHasAudioTrack);
		if ((error = xmlHelper.CreateTag("AUDIO_CODEC")) == B_OK) {
			xmlHelper.SetAttribute("name", fAudioCodecName);
			xmlHelper.SetAttribute("frame_rate", fAudioFrameRate);
			xmlHelper.SetAttribute("channels", fAudioChannelCount);
			xmlHelper.CloseTag();	// AUDIO_CODEC
		}
		xmlHelper.CloseTag();	// AUDIO
	}
	if ((error = xmlHelper.CreateTag("COPYRIGHT")) == B_OK) {
		xmlHelper.SetAttribute("string", fCopyright);
		xmlHelper.CloseTag();	// COPYRIGHT
	}
	if ((error = xmlHelper.CreateTag("TIMECODE")) == B_OK) {
		xmlHelper.SetAttribute("visible", fTimeCodeOverlay);
		xmlHelper.SetAttribute("transparency", fTimeCodeTransparency);
		xmlHelper.SetAttribute("scale", fTimeCodeScale);
		xmlHelper.CloseTag();	// TIMECODE
	}
	// Who cares about errors?
	return B_OK;
}

// XMLRestore
status_t
RenderPreset::XMLRestore(XMLHelper& xmlHelper)
{
	xmlHelper.GetAttribute("name", fName);
	fRenderPreview = xmlHelper.GetAttribute("preview", fRenderPreview);

	// "use alpha" depends on "preview" setting
	if (!fRenderPreview)
		fUseAlpha = xmlHelper.GetAttribute("use_alpha", fUseAlpha);
	else
		fUseAlpha = false;

	if (xmlHelper.OpenTag("FILE_FORMAT") == B_OK) {
		xmlHelper.GetAttribute("name", fFamilyName);
		xmlHelper.CloseTag();	// FILE_FORMAT
	}
	if (xmlHelper.OpenTag("VIDEO") == B_OK) {
		fHasVideoTrack = xmlHelper.GetAttribute("on", fHasVideoTrack);
		if (xmlHelper.OpenTag("VIDEO_FORMAT") == B_OK) {
			fLineWidth = xmlHelper.GetAttribute("line_width", fLineWidth);
			fLineCount = xmlHelper.GetAttribute("line_count", fLineCount);
			fColorSpace = (color_space)xmlHelper.GetAttribute("color_space",
														 (int32)fColorSpace);
			xmlHelper.CloseTag();	// VIDEO_FORMAT
		}
		if (xmlHelper.OpenTag("VIDEO_CODEC") == B_OK) {
			xmlHelper.GetAttribute("name", fVideoCodecName);
			xmlHelper.CloseTag();	// VIDEO_CODEC
		}
		if (xmlHelper.OpenTag("QUALITY") == B_OK) {
			fVideoQuality = xmlHelper.GetAttribute("value", fVideoQuality);
			xmlHelper.CloseTag();	// QUALITY;
		}
		xmlHelper.CloseTag();	// VIDEO;
	}
	if (xmlHelper.OpenTag("AUDIO") == B_OK) {
		fHasAudioTrack = xmlHelper.GetAttribute("on", fHasAudioTrack);
		if (xmlHelper.OpenTag("AUDIO_CODEC") == B_OK) {
			xmlHelper.GetAttribute("name", fAudioCodecName);
			fAudioFrameRate = xmlHelper.GetAttribute("frame_rate", fAudioFrameRate);
			fAudioChannelCount = xmlHelper.GetAttribute("channels", fAudioChannelCount);
			xmlHelper.CloseTag();	// AUDIO_CODEC
		}
		xmlHelper.CloseTag();	// AUDIO
	}
	if (xmlHelper.OpenTag("COPYRIGHT") == B_OK) {
		xmlHelper.GetAttribute("string", fCopyright);
		xmlHelper.CloseTag();	// COPYRIGHT
	}
	if (xmlHelper.OpenTag("TIMECODE") == B_OK) {
		fTimeCodeOverlay = xmlHelper.GetAttribute("visible", fTimeCodeOverlay);
		fTimeCodeTransparency = xmlHelper.GetAttribute("transparency", fTimeCodeTransparency);
		fTimeCodeScale = xmlHelper.GetAttribute("scale", fTimeCodeScale);
		xmlHelper.CloseTag();	// TIMECODE
	}
	return B_OK;
}
#endif

// Archive
status_t
RenderPreset::Archive(BMessage* into, bool deep) const
{
	if (into == NULL)
		return B_BAD_VALUE;

	status_t ret = B_OK;
	if (ret == B_OK)
		ret = into->AddUInt32("line width", fLineWidth);
	if (ret == B_OK)
		ret = into->AddUInt32("line count", fLineCount);
	if (ret == B_OK)
		ret = into->AddUInt32("color space", (uint32)fColorSpace);
	if (ret == B_OK)
		ret = into->AddString("name", fName.String());
	if (ret == B_OK)
		ret = into->AddString("copyright", fCopyright.String());
	if (ret == B_OK)
		ret = into->AddString("family name", fFamilyName.String());
	if (ret == B_OK)
		ret = into->AddString("video codec name", fVideoCodecName.String());
	if (ret == B_OK)
		ret = into->AddString("audio codec name", fAudioCodecName.String());
	if (ret == B_OK)
		ret = into->AddFloat("video quality", fVideoQuality);
	if (ret == B_OK)
		ret = into->AddFloat("audio quality", fAudioQuality);
	if (ret == B_OK)
		ret = into->AddBool("has video track", fHasVideoTrack);
	if (ret == B_OK)
		ret = into->AddBool("has audio track", fHasAudioTrack);
	if (ret == B_OK)
		ret = into->AddBool("render preview", fRenderPreview);
	if (ret == B_OK)
		ret = into->AddBool("use alpha", fUseAlpha);
	if (ret == B_OK)
		ret = into->AddBool("time code overlay", fTimeCodeOverlay);
	if (ret == B_OK)
		ret = into->AddFloat("time code transparency", fTimeCodeTransparency);
	if (ret == B_OK)
		ret = into->AddFloat("time code scale", fTimeCodeScale);
	if (ret == B_OK)
		ret = into->AddFloat("video frame rate", fVideoFrameRate);
	if (ret == B_OK)
		ret = into->AddFloat("audio frame rate", fAudioFrameRate);
	if (ret == B_OK)
		ret = into->AddUInt32("audio channel count", fAudioChannelCount);

	return ret;
}

// Unarchive
status_t
RenderPreset::Unarchive(const BMessage* archive)
{
	if (archive == NULL)
		return B_BAD_VALUE;

	uint32 uIntValue;
	float floatValue;
	const char* stringValue;
	bool boolValue;

	if (archive->FindUInt32("line width", &uIntValue) == B_OK)
		fLineWidth = uIntValue;
	if (archive->FindUInt32("line count", &uIntValue) == B_OK)
		fLineCount = uIntValue;
	if (archive->FindUInt32("color space", &uIntValue) == B_OK)
		fColorSpace = (color_space)uIntValue;
	if (archive->FindString("name", &stringValue) == B_OK)
		fName = stringValue;
	if (archive->FindString("copyright", &stringValue) == B_OK)
		fCopyright = stringValue;
	if (archive->FindString("family name", &stringValue) == B_OK)
		fFamilyName = stringValue;
	if (archive->FindString("video codec name", &stringValue) == B_OK)
		fVideoCodecName = stringValue;
	if (archive->FindString("audio codec name", &stringValue) == B_OK)
		fAudioCodecName = stringValue;
	if (archive->FindFloat("video quality", &floatValue) == B_OK)
		fVideoQuality = floatValue;
	if (archive->FindFloat("audio quality", &floatValue) == B_OK)
		fAudioQuality = floatValue;
	if (archive->FindBool("has video track", &boolValue) == B_OK)
		fHasVideoTrack = boolValue;
	if (archive->FindBool("has audio track", &boolValue) == B_OK)
		fHasAudioTrack = boolValue;
	if (archive->FindBool("render preview", &boolValue) == B_OK)
		fRenderPreview = boolValue;
	if (archive->FindBool("use alpha", &boolValue) == B_OK)
		fUseAlpha = boolValue;
	if (archive->FindBool("time code overlay", &boolValue) == B_OK)
		fTimeCodeOverlay = boolValue;
	if (archive->FindFloat("time code transparency", &floatValue) == B_OK)
		fTimeCodeTransparency = floatValue;
	if (archive->FindFloat("time code scale", &floatValue) == B_OK)
		fTimeCodeScale = floatValue;
	if (archive->FindFloat("video frame rate", &floatValue) == B_OK)
		fVideoFrameRate = floatValue;
	if (archive->FindFloat("audio frame rate", &floatValue) == B_OK)
		fAudioFrameRate = floatValue;
	if (archive->FindUInt32("audio channel count", &uIntValue) == B_OK)
		fAudioChannelCount = uIntValue;

	return B_OK;
}

// operator=
RenderPreset&
RenderPreset::operator=(const RenderPreset& other)
{
	fLineWidth = other.fLineWidth;
	fLineCount = other.fLineCount;
	fColorSpace = other.fColorSpace;
	fName = other.fName;
	fCopyright = other.fCopyright;
	fFamilyName = other.fFamilyName;
	fVideoCodecName = other.fVideoCodecName;
	fAudioCodecName = other.fAudioCodecName;
	fVideoQuality = other.fVideoQuality;
	fAudioQuality = other.fAudioQuality;
	fHasVideoTrack = other.fHasVideoTrack;
	fHasAudioTrack = other.fHasAudioTrack;
	fRenderPreview = other.fRenderPreview;
	fUseAlpha = other.fUseAlpha;
	fTimeCodeOverlay = other.fTimeCodeOverlay;
	fTimeCodeTransparency = other.fTimeCodeTransparency;
	fTimeCodeScale = other.fTimeCodeScale;
	fVideoFrameRate = other.fVideoFrameRate;
	fAudioFrameRate = other.fAudioFrameRate;
	fAudioChannelCount = other.fAudioChannelCount;
	return *this;
}

// AdoptValues
void
RenderPreset::AdoptValues(const RenderPreset& preset)
{
	BString name = fName;
	*this = preset;
	fName = name;
}

// HasSameValues
bool
RenderPreset::HasSameValues(const RenderPreset& other) const
{
	return fLineWidth == other.fLineWidth &&
		fLineCount == other.fLineCount &&
		fColorSpace == other.fColorSpace &&
		fCopyright == other.fCopyright &&
		fFamilyName == other.fFamilyName &&
		fVideoCodecName == other.fVideoCodecName &&
		fAudioCodecName == other.fAudioCodecName &&
		fVideoQuality == other.fVideoQuality &&
		fAudioQuality == other.fAudioQuality &&
		fHasVideoTrack == other.fHasVideoTrack &&
		fHasAudioTrack == other.fHasAudioTrack &&
		fRenderPreview == other.fRenderPreview &&
		fUseAlpha == other.fUseAlpha &&
		fTimeCodeOverlay == other.fTimeCodeOverlay &&
		fTimeCodeTransparency == other.fTimeCodeTransparency &&
		fTimeCodeScale == other.fTimeCodeScale &&
		fVideoFrameRate == other.fVideoFrameRate &&
		fAudioFrameRate == other.fAudioFrameRate &&
		fAudioChannelCount == other.fAudioChannelCount;
}

// SetLineWidth
void
RenderPreset::SetLineWidth(uint32 lineWidth)
{
	fLineWidth = lineWidth;
}

// SetLineCount
void
RenderPreset::SetLineCount(uint32 lineCount)
{
	fLineCount = lineCount;
}

// SetColorSpace
void
RenderPreset::SetColorSpace(color_space colorSpace)
{
	fColorSpace = colorSpace;
}

// SetVideoQuality
void
RenderPreset::SetVideoQuality(float quality)
{
	fVideoQuality = quality;
}

// SetAudioQuality
void
RenderPreset::SetAudioQuality(float quality)
{
	fAudioQuality = quality;
}

// SetCopyright
void
RenderPreset::SetCopyright(const char *cpr)
{
	fCopyright.SetTo(cpr);
}

// SetFormatFamily
void
RenderPreset::SetFormatFamily(const char *family_name)
{
	fFamilyName.SetTo(family_name);
}

// SetVideoCodecInfo
void
RenderPreset::SetVideoCodec(const char *codec_name)
{
	fVideoCodecName.SetTo(codec_name);
}

// SetAudioCodecInfo
void
RenderPreset::SetAudioCodec(const char *codec_name)
{
	fAudioCodecName.SetTo(codec_name);
}

// SetVideo
void
RenderPreset::SetVideo(bool active)
{
	fHasVideoTrack = active;
}

// SetAudio
void
RenderPreset::SetAudio(bool active)
{
	fHasAudioTrack = active;
}

// SetRenderPreview
void
RenderPreset::SetRenderPreview(bool preview)
{
	fRenderPreview = preview;
}

// SetUseAlpha
void
RenderPreset::SetUseAlpha(bool use)
{
	fUseAlpha = use;
}

// SetVideoFrameRate
void
RenderPreset::SetVideoFrameRate(float fps)
{
	fVideoFrameRate = fps;
}

// VideoFrameRate
float
RenderPreset::VideoFrameRate() const
{
	return fVideoFrameRate;
}

// SetAudioFrameRate
void
RenderPreset::SetAudioFrameRate(float fps)
{
	fAudioFrameRate = fps;
}

// AudioFrameRate
float
RenderPreset::AudioFrameRate() const
{
	return fAudioFrameRate;
}

// SetAudioChannelCount
void
RenderPreset::SetAudioChannelCount(uint32 channels)
{
	fAudioChannelCount = channels;
}

// AudioChannelCount
uint32
RenderPreset::AudioChannelCount() const
{
	return fAudioChannelCount;
}

// SetTimeCodeOverlay
void
RenderPreset::SetTimeCodeOverlay(bool use)
{
	fTimeCodeOverlay = use;
}

// SetTimeCodeTransparency
void
RenderPreset::SetTimeCodeTransparency(float transparency)
{
	fTimeCodeTransparency = transparency;
}

// SetTimeCodeScale
void
RenderPreset::SetTimeCodeScale(float factor)
{
	if (factor < 1.0)
		factor = 1.0;
	if (factor > 5.0)
		factor = 5.0;
	fTimeCodeScale = factor;
}

