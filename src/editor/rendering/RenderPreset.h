/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef RENDER_PRESET_H
#define RENDER_PRESET_H

#include <GraphicsDefs.h>
#include <String.h>

//#include "XMLStorable.h"

class BMessage;
class XMLHelper;

class RenderPreset /*: public XMLStorable*/ {
 public:
								RenderPreset();
								RenderPreset(const char* name,
									uint32 lineWidth, uint32 lineCount,
									color_space space, float videoQuality,
									bool hasVideoTrack, bool hasAudioTrack,
									bool renderPreview, bool useAlpha,
									const char* copyright);
								// "copy constructor"
								RenderPreset(const char* name,
									RenderPreset* preset);
	virtual						~RenderPreset();

								// the xml storable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	virtual	status_t			Unarchive(const BMessage* archive);

			RenderPreset&		operator=(const RenderPreset& other);

			void				AdoptValues(const RenderPreset& preset);
			bool				HasSameValues(
									const RenderPreset& preset) const;

			BString				Name() const
									{ return fName; }

			void				SetLineWidth(uint32 lineWidth);
			uint32				LineWidth() const
									{ return fLineWidth; }

			void				SetLineCount(uint32 lineCount);
			uint32				LineCount() const
									{ return fLineCount; }

			void				SetColorSpace(color_space colorSpace);
			color_space			ColorSpace() const
									{ return fColorSpace; }

			void				SetFormatFamily(const char* familyName);
			BString				FormatFamily() const
									{ return fFamilyName; }

			void				SetVideoCodec(const char* codecName);
			BString				VideoCodec() const
									{ return fVideoCodecName; }

			void				SetAudioCodec(const char* codecName);
			BString				AudioCodec() const
									{ return fAudioCodecName; }

			void				SetCopyright(const char* cpr);
			BString				Copyright() const
									{ return fCopyright; }

			void				SetVideoQuality(float quality);
			float				VideoQuality() const
									{ return fVideoQuality; }

			void				SetAudioQuality(float quality);
			float				AudioQuality() const
									{ return fAudioQuality; }

			void				SetVideo(bool active);
			bool				IsVideo() const
									{ return fHasVideoTrack; }

			void				SetAudio(bool active);
			bool				IsAudio() const
									{ return fHasAudioTrack; }

			void				SetRenderPreview(bool preview);
			bool				RenderPreview() const
									{ return fRenderPreview; }

			void				SetUseAlpha(bool useAlpha);
			bool				UseAlpha() const
									{ return fUseAlpha; }

			void				SetVideoFrameRate(float fps);
			float				VideoFrameRate() const;

			void				SetAudioFrameRate(float fps);
			float				AudioFrameRate() const;

			void				SetAudioChannelCount(uint32 channels);
			uint32				AudioChannelCount() const;

								// time code
			void				SetTimeCodeOverlay(bool use);
			bool				TimeCodeOverlay() const
									{ return fTimeCodeOverlay; }
			void				SetTimeCodeTransparency(float transparency);
			float				TimeCodeTransparency() const
									{ return fTimeCodeTransparency; }
			void				SetTimeCodeScale(float factor);
			float				TimeCodeScale() const
									{ return fTimeCodeScale; }

private:
			uint32				fLineWidth;
			uint32				fLineCount;
			color_space			fColorSpace;
			BString				fName;
			BString				fCopyright;
			BString				fFamilyName;
			BString				fVideoCodecName;
			BString				fAudioCodecName;
			float				fVideoQuality;
			float				fAudioQuality;
			bool				fHasVideoTrack;
			bool				fHasAudioTrack;
			bool				fRenderPreview;
			bool				fUseAlpha;
			bool				fTimeCodeOverlay;
			float				fTimeCodeTransparency;
			float				fTimeCodeScale;
			float				fVideoFrameRate;
			float				fAudioFrameRate;
			uint32				fAudioChannelCount;
};

#endif // RENDER_PRESET_H
