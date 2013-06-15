/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef RENDER_SETTINGS_WINDOW_H
#define RENDER_SETTINGS_WINDOW_H

#include <MediaFormats.h>
#include <String.h>
#include <Window.h>

//#include "XMLStorable.h"

class BButton;
class BCheckBox;
class BMenu;
class BMenuItem;
class BSlider;
class BTextControl;
class DimensionsControl;
class LabelCheckBox;
class LabelPopup;
class RenderPreset;
class XMLHelper;

enum {
	MSG_RENDER_SETTINGS_HIDDEN		= 'rshd',
	MSG_HELP_RENDERSETTINGS			= 'hprs',
};

class RenderSettingsWindow : public BWindow/*, public XMLStorable*/ {
public:
								RenderSettingsWindow(BMessage* message = NULL,
									BHandler* target = NULL);
	virtual						~RenderSettingsWindow();

	// BWindow interface
	virtual bool				QuitRequested();
	virtual void				MessageReceived(BMessage* message);

	virtual void				Show();
	virtual void				Hide();

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

	virtual	status_t			StoreSettings(BMessage* archive) const;
	virtual	status_t			RestoreSettings(const BMessage* archive);

	// RenderSettingsWindow
			const RenderPreset*	RenderPresetInfo();

			void				SaveSettings();
			void				LoadSettings();

			void				UpdateStrings();

			void				SetMessage(BMessage* message);

private:
			void				_Init();
	static	int32				_InitEntry(void* cookie);

			void				_Invoke();
			RenderPreset*		_NewRenderPreset(const char* name);
			void				_SetToRenderPreset(RenderPreset* preset);
			void				_BuildPresetsMenu();
			void				_BuildFormatMenu();
			void				_BuildVideoCodecMenu();
			void				_BuildAudioCodecMenu();
			void				_CleanupPresets();
			void				_RestorePreset(int32 index);
			void				_StoreInPreset(int32 index);
			void				_DeletePreset(int32 index);
			void				_CheckCodecs();
			void				_SetVideoEnabled(bool enabled);
			void				_SetAudioEnabled(bool enabled);
			void				_CheckTracks(media_file_format* format);
			void				_CommitTextControlChanges();
			void				_CheckCurrentPresetName();
			void				_EnableTimeCodeControls(bool enable);
			const char*			_GetString(uint32 index,
									const char* defaultString) const;

			BMessage*			fMessage;
			BHandler*			fTarget;

			BList				fPresets;
			BList				fFormats;
			RenderPreset*		fCurrentSettings;
			RenderPreset*		fBackupSettings;

			BMenu*				fPresetsM;
			BMenu*				fHelpM;
			DimensionsControl*	fDimensionsC;
			LabelPopup*			fFormatPU;
			BMenu*				fFormatM;
			LabelPopup*			fVidCodecPU;
			LabelPopup*			fColorSpacePU;
			LabelPopup*			fAudCodecPU;
			LabelPopup*			fAudFrameRatePU;
			LabelPopup*			fRenderQualityPU;
			LabelPopup*			fAudChannelCountPU;
			LabelPopup*			fTimeCodeSizePU;
			LabelPopup*			fTimeCodeTransparencyPU;
			BSlider*			fVidQualityS;
			BTextControl*		fCopyrightTC;
			BMenuItem*			fBubbleHelpMI;
			BMenuItem*			fHelpMI;
			LabelCheckBox*		fVideoCB;
			LabelCheckBox*		fAudioCB;
			LabelCheckBox*		fTimeCodeCB;
			BMenuItem*			fSmallMI;
			BMenuItem*			fMediumMI;
			BMenuItem*			fLargeMI;
			BMenuItem*			fTransparentMI;
			BMenuItem*			fMediumTransMI;
			BMenuItem*			fOpaqueMI;
			BMenuItem*			fRGB15MI;
			BMenuItem*			fRGB16MI;
			BMenuItem*			fRGB24MI;
			BMenuItem*			fRGB32MI;
			BMenuItem*			fYCbCr422MI;
			BMenuItem*			fMonoMI;
			BMenuItem*			fStereoMI;
			BMenuItem*			fBWPreviewMI;
			BMenuItem*			fFullColorMI;
			BMenuItem*			fFullColorAlphaMI;
			BButton*			fOkB;
			BButton*			fCancelB;
			BButton*			fRevertB;
};

#endif // RENDER_SETTINGS_WINDOW_H
