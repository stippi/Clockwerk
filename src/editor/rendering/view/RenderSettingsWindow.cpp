/*
 * Copyright 2000-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "RenderSettingsWindow.h"

#include <stdio.h>
#include <stdlib.h>

#include <new>

#include <Application.h>
#include <Autolock.h>
#include <Box.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <File.h>
#include <Path.h>
#include <FindDirectory.h>
#include <Screen.h>
#include <SeparatorView.h>
#include <Slider.h>
#include <TextControl.h>

#include "common_constants.h"

#if BUBBLE_HELPER
#  include "BubbleHelper.h"
#endif
#include "DimensionsControl.h"
#include "NamePanel.h"
#include "LabelCheckBox.h"
#include "LabelPopup.h"
#if LANGUAGE_MANAGER
#  include "LanguageManager.h"
#endif
#include "RenderPreset.h"

// debugging
#include "Debug.h"
#define ldebug	debug
//#define ldebug	nodebug

static const char* kMsgFamilyField = "BW:family";
static const char* kMsgCodecField = "BW:codec";

enum {
	MSG_FILE_FORMAT						= 'Ffmt',
	MSG_INVOKE_ITEM						= 'invi',
	MSG_VIDEO_CODEC						= 'vidc',
	MSG_AUDIO_CODEC						= 'audc',
	MSG_COLOR_SPACE_CHANGED				= 'csch',
	MSG_RENDER_QUALITY_CHANGED			= 'rqch',
	MSG_FRAME_RATE_CHANGED				= 'afrc',
	MSG_CHANNEL_COUNT_CHANGED			= 'ccch',
	MSG_VIDEO_ACTIVE_CHANGED			= 'vach',
	MSG_AUDIO_ACTIVE_CHANGED			= 'aach',
	MSG_QUALITY_CHANGED					= 'qlch',
	MSG_COPYRIGHT_CHANGED				= 'crsh',
	MSG_TIMECODE_ACTIVE_CHANGED			= 'tcac',
	MSG_TIMECODE_SCALE_CHANGED			= 'tcsc',
	MSG_TIMECODE_TRANSPARENCY_CHANGED	= 'tctc',
	// preset management
	MSG_NEW_PRESET						= 'nwrp',
	MSG_RESTORE_PRESET					= 'rspt',
	MSG_STORE_IN_PRESET					= 'sipt',
	MSG_DELETE_PRESET					= 'dlpt',
	// dimensions watching
	MSG_DIMENSIONS_CHANGED				= 'dmch',
	// general
	MSG_OK								= 'okok',
	MSG_CANCEL							= 'cncl',
	MSG_REVERT							= 'rvrt',
	MSG_NAME_PANEL						= 'nmpn',
	MSG_ENABLE_BUBBLE_HELP				= 'enbh',
// TODO: put somewhere else, be_app is supposed to get it
	MSG_TOGGLE_BUBBLE_HELP				= 'tgbh',
};

#if !defined(LANGUAGE_MANAGER) || !LANGUAGE_MANAGER
enum {
	NEW,
	DELETE,
	LOAD,
	SAVE_AS,
	OK,
	REVERT,
	CANCEL,

	WIDTH,
	HEIGHT,

	SETTINGS,
	HELP,
	BUBBLE_HELP,

	RENDER,
	RENDER_SETTINGS,
	NAME_OF_RENDER_PRESET,
	OUTPUT_AS,
	CODEC,
	COLOR_SPACE,
	RGB_32K_COLORS,
	RGB_64K_COLORS,
	RGB24_16M_COLORS,
	RGB32_16M_COLORS,
	YCBCR_16M_COLORS,
	BW_PREVIEW,
	FULL_COLOR,
	FULL_COLOR_ALPHA,
	SAMPLE_RATE,
	CHANNELS,
	MONO,
	STEREO,
	SIZE,
	SMALL,
	MEDIUM,
	LARGE,
	TRANSPARENCY,
	TRANSPARENT,
	OPAQUE,
	RENDER_SETTINGS_WINDOW,
	VIDEO,
	QUALITY,
	AUDIO,
	RENDER_TIME_CODE,
	COPYRIGHT_INFO,
};
#endif

// constructor
RenderSettingsWindow::RenderSettingsWindow(BMessage* message, BHandler* target)
	:
	BWindow(BRect(50.0f, 50.0f, 100.0f, 100.0f), "Render Settings",
		B_FLOATING_WINDOW_LOOK, B_FLOATING_APP_WINDOW_FEEL,
		B_ASYNCHRONOUS_CONTROLS | B_NOT_ZOOMABLE
			| B_NOT_ANCHORED_ON_ACTIVATE | B_AUTO_UPDATE_SIZE_LIMITS),

	fMessage(message),
	fTarget(target),

	fCurrentSettings(new RenderPreset("Default", 384, 288, B_RGB32, 1.0f,
		true, true, false, false, "© 2009")),
	fBackupSettings(NULL)
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	fOkB = new BButton("Ok", new BMessage(MSG_OK));

	fPresetsM = new BMenu("Settings");

	BMenuItem* item;		// used to setup popup menus
	BMessage* msg;

	fFormatM = new BPopUpMenu("File Format");
	_BuildFormatMenu();
	fFormatPU = new LabelPopup("Output As", fFormatM, true, true);

	fVidCodecPU = new LabelPopup("Codec");
	fAudCodecPU = new LabelPopup("Codec");

	fColorSpacePU = new LabelPopup("Color Space");
	msg = new BMessage(MSG_COLOR_SPACE_CHANGED);
	msg->AddInt32("format", B_RGB15);
	fRGB15MI = new BMenuItem("RGB (16 bpp, 32K colors)", msg);
fRGB15MI->SetEnabled(false);
	fColorSpacePU->Menu()->AddItem(fRGB15MI);
	msg = new BMessage(MSG_COLOR_SPACE_CHANGED);
	msg->AddInt32("format", B_RGB16);
	fRGB16MI = new BMenuItem("RGB (16 bpp, 64K colors)", msg);
fRGB16MI->SetEnabled(false);
	fColorSpacePU->Menu()->AddItem(fRGB16MI);
	fColorSpacePU->Menu()->AddSeparatorItem();
	msg = new BMessage(MSG_COLOR_SPACE_CHANGED);
	msg->AddInt32("format", B_RGB24);
	fRGB24MI = new BMenuItem("RGB (24 bpp, 16M colors)", msg);
	fColorSpacePU->Menu()->AddItem(fRGB24MI);
	msg = new BMessage(MSG_COLOR_SPACE_CHANGED);
	msg->AddInt32("format", B_RGB32);
	fRGB32MI = new BMenuItem("RGB (32 bpp, 16M colors)", msg);
	fRGB32MI->SetMarked(true);
	fColorSpacePU->Menu()->AddItem(fRGB32MI);
	fColorSpacePU->Menu()->AddSeparatorItem();
	msg = new BMessage(MSG_COLOR_SPACE_CHANGED);
	msg->AddInt32("format", B_YCbCr422);
	fYCbCr422MI = new BMenuItem("YCbCr 4:2:2 (16 bpp, 16M colors)", msg);
	fColorSpacePU->Menu()->AddItem(fYCbCr422MI);

	fRenderQualityPU = new LabelPopup("Render");

	fBWPreviewMI = new BMenuItem("Black & White (Animation preview)",
		new BMessage(MSG_RENDER_QUALITY_CHANGED));
	fRenderQualityPU->Menu()->AddItem(fBWPreviewMI);
	fFullColorMI = new BMenuItem("Full Color (Load frames from disk)",
		new BMessage(MSG_RENDER_QUALITY_CHANGED));
	fFullColorMI->SetMarked(true);
	fRenderQualityPU->Menu()->AddItem(fFullColorMI);
	fFullColorAlphaMI = new BMenuItem("Full Color (Alphachannel)",
		new BMessage(MSG_RENDER_QUALITY_CHANGED));
	fRenderQualityPU->Menu()->AddSeparatorItem();
	fRenderQualityPU->Menu()->AddItem(fFullColorAlphaMI);
	fRenderQualityPU->Menu()->SetTargetForItems(this);

	fAudFrameRatePU = new LabelPopup("Sample Rate");
	msg = new BMessage(MSG_FRAME_RATE_CHANGED);
	msg->AddFloat("frame rate", 11025.0);
	item = new BMenuItem("11025 Hz", msg);
	fAudFrameRatePU->Menu()->AddItem(item);
	msg = new BMessage(MSG_FRAME_RATE_CHANGED);
	msg->AddFloat("frame rate", 22050.0);
	item = new BMenuItem("22050 Hz", msg);
	fAudFrameRatePU->Menu()->AddItem(item);
	msg = new BMessage(MSG_FRAME_RATE_CHANGED);
	msg->AddFloat("frame rate", 44100.0);
	item = new BMenuItem("44100 Hz", msg);
	fAudFrameRatePU->Menu()->AddItem(item);
	msg = new BMessage(MSG_FRAME_RATE_CHANGED);
	msg->AddFloat("frame rate", 48000.0);
	item = new BMenuItem("48000 Hz", msg);
	fAudFrameRatePU->Menu()->AddItem(item);
	fAudFrameRatePU->Menu()->SetTargetForItems(this);


	fAudChannelCountPU = new LabelPopup("Channels");
	msg = new BMessage(MSG_CHANNEL_COUNT_CHANGED);
	msg->AddInt32("channel count", 1);
	fMonoMI = new BMenuItem("Mono", msg);
	fAudChannelCountPU->Menu()->AddItem(fMonoMI);
	msg = new BMessage(MSG_CHANNEL_COUNT_CHANGED);
	msg->AddInt32("channel count", 2);
	fStereoMI = new BMenuItem("Stereo", msg);
	fAudChannelCountPU->Menu()->AddItem(fStereoMI);
	fAudChannelCountPU->Menu()->SetTargetForItems(this);

	fTimeCodeSizePU = new LabelPopup("Size");
	msg = new BMessage(MSG_TIMECODE_SCALE_CHANGED);
	msg->AddFloat("scale", 1.0);
	fSmallMI = new BMenuItem("Small", msg);
	fTimeCodeSizePU->Menu()->AddItem(fSmallMI);
	msg = new BMessage(MSG_TIMECODE_SCALE_CHANGED);
	msg->AddFloat("scale", 2.0);
	fMediumMI = new BMenuItem("Medium", msg);
	fTimeCodeSizePU->Menu()->AddItem(fMediumMI);
	msg = new BMessage(MSG_TIMECODE_SCALE_CHANGED);
	msg->AddFloat("scale", 3.0);
	fLargeMI = new BMenuItem("Large", msg);
	fTimeCodeSizePU->Menu()->AddItem(fLargeMI);
	fTimeCodeSizePU->Menu()->SetTargetForItems(this);

	fTimeCodeTransparencyPU = new LabelPopup("Transparency");
	msg = new BMessage(MSG_TIMECODE_TRANSPARENCY_CHANGED);
	msg->AddFloat("transparency", 0.4);
	fTransparentMI = new BMenuItem("Semi Transparent", msg);
	fTimeCodeTransparencyPU->Menu()->AddItem(fTransparentMI);
	msg = new BMessage(MSG_TIMECODE_TRANSPARENCY_CHANGED);
	msg->AddFloat("transparency", 0.7);
	fMediumTransMI = new BMenuItem("Medium", msg);
	fTimeCodeTransparencyPU->Menu()->AddItem(fMediumTransMI);
	msg = new BMessage(MSG_TIMECODE_TRANSPARENCY_CHANGED);
	msg->AddFloat("transparency", 1.0);
	fOpaqueMI = new BMenuItem("Opaque", msg);
	fTimeCodeTransparencyPU->Menu()->AddItem(fOpaqueMI);
	fTimeCodeTransparencyPU->Menu()->SetTargetForItems(this);

	BMenuBar* menuBar = new BMenuBar("menu bar");
	menuBar->AddItem(fPresetsM);
	fHelpM = new BMenu("Help");
	fHelpMI = new BMenuItem("Render Settings Window"B_UTF8_ELLIPSIS,
		new BMessage(MSG_HELP_RENDERSETTINGS), 'H');
	fHelpMI->SetTarget(be_app);
	fHelpM->AddItem(fHelpMI);
	fBubbleHelpMI = new BMenuItem("Bubble Help",
		new BMessage(MSG_TOGGLE_BUBBLE_HELP), 0);
	fBubbleHelpMI->SetTarget(be_app);
	fHelpM->AddItem(fBubbleHelpMI);
	menuBar->AddItem(fHelpM);

	fVideoCB = new LabelCheckBox("Video",
		new BMessage(MSG_VIDEO_ACTIVE_CHANGED), this, true),
	fAudioCB = new LabelCheckBox("Audio",
		new BMessage(MSG_AUDIO_ACTIVE_CHANGED), this, true),
	fVidQualityS = new BSlider(NULL, "Quality",
		new BMessage(MSG_QUALITY_CHANGED), 1, 100, B_HORIZONTAL),
	fDimensionsC = new DimensionsControl(new BMessage(MSG_DIMENSIONS_CHANGED),
		this),

	fTimeCodeCB = new LabelCheckBox("Render Time Code",
		new BMessage(MSG_TIMECODE_ACTIVE_CHANGED), this, true),
	fCopyrightTC = new BTextControl("Copyright Info", "",
		new BMessage(MSG_COPYRIGHT_CHANGED)),
	fRevertB = new BButton("Revert", new BMessage(MSG_REVERT));
	fCancelB = new BButton("Cancel", new BMessage(MSG_CANCEL));

	// now build the UI
	const float spacing = 8.0f;

	BGroupView* groupView = new BGroupView(B_VERTICAL, spacing);
	BGroupLayoutBuilder(groupView)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(fVidCodecPU)
		.Add(fVidQualityS)
		.Add(fDimensionsC)
		.Add(fColorSpacePU)
	;
	BBox* videoGroup = new BBox(B_FANCY_BORDER, groupView);
	videoGroup->SetLabel(fVideoCB);

	groupView = new BGroupView(B_VERTICAL, spacing);
	BGroupLayoutBuilder(groupView)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(fAudCodecPU)
		.Add(fAudChannelCountPU)
		.Add(fAudFrameRatePU)
	;
	BBox* audioGroup = new BBox(B_FANCY_BORDER, groupView);
	audioGroup->SetLabel(fAudioCB);

	groupView = new BGroupView(B_VERTICAL, spacing);
	BGroupLayoutBuilder(groupView)
		.SetInsets(spacing, spacing, spacing, spacing)
		.Add(fTimeCodeSizePU)
		.Add(fTimeCodeTransparencyPU)
	;
	BBox* timeCodeGroup = new BBox(B_FANCY_BORDER, groupView);
	timeCodeGroup->SetLabel(fTimeCodeCB);

	groupView = new BGroupView(B_VERTICAL, spacing);
	BGroupLayoutBuilder(groupView)
		.Add(menuBar)
		.AddGroup(B_VERTICAL, spacing)
			.SetInsets(spacing, spacing, spacing, 0.0f)
			.Add(fFormatPU)
			.AddGroup(B_HORIZONTAL, spacing)
				.Add(videoGroup)
				.AddGroup(B_VERTICAL, spacing)
					.Add(audioGroup)
					.Add(timeCodeGroup)
				.End()
			.End()
			.Add(fCopyrightTC)
		.End()
		.Add(new BSeparatorView("", B_HORIZONTAL, B_PLAIN_BORDER))
		.AddGroup(B_HORIZONTAL, spacing)
			.SetInsets(spacing, 0.0f, spacing, spacing)
			.AddGlue(3.0f)
			.Add(fRevertB)
			.Add(fCancelB)
			.AddStrut(spacing)
			.Add(fOkB)
		.End()
	;
	AddChild(groupView);

	SetDefaultButton(fOkB);

	fDimensionsC->SetWidthLimits(20, 4096);
	fDimensionsC->SetHeightLimits(15, 4096);

	fCopyrightTC->SetTarget(this);

	// put current data into the text views
	fDimensionsC->SetDimensions(fCurrentSettings->LineWidth(),
		fCurrentSettings->LineCount());

	// adjust some settings for the video quality slider
	fVidQualityS->SetLimitLabels("1%", "100%");
	fVidQualityS->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fVidQualityS->SetHashMarkCount(10);
	fVidQualityS->SetValue(fCurrentSettings->VideoQuality() * 100.0f);

	UpdateStrings();

	Hide();
	Show();

	// launch init method in different thread
	thread_id initThread = spawn_thread(_InitEntry, "render settings init",
										B_LOW_PRIORITY, this);
	if (initThread >= B_OK)
		resume_thread(initThread);
	else
		_Init();
}

// destructor
RenderSettingsWindow::~RenderSettingsWindow()
{
#if BUBBLE_HELPER
	BubbleHelper* helper = BubbleHelper::GetDefault();
	helper->UnsetHelp(fFormatPU);
	helper->UnsetHelp(fVidCodecPU);
	helper->UnsetHelp(fAudCodecPU);
	helper->UnsetHelp(fAudFrameRatePU);
	helper->UnsetHelp(fAudChannelCountPU);
	helper->UnsetHelp(fVidQualityS);
	helper->UnsetHelp(fDimensionsC);
	helper->UnsetHelp(fCopyrightTC);
	helper->UnsetHelp(fVideoCB);
	helper->UnsetHelp(fAudioCB);
	helper->UnsetHelp(fColorSpacePU);
	helper->UnsetHelp(fRenderQualityPU);
	helper->UnsetHelp(fTimeCodeCB);
	helper->UnsetHelp(fTimeCodeSizePU);
	helper->UnsetHelp(fTimeCodeTransparencyPU);
#endif // BUBBLE_HELPER
	_CleanupPresets();
	delete fCurrentSettings;
	for (int32 i = 0; media_file_format* format
			= (media_file_format*)fFormats.ItemAt(i); i++) {
		delete format;
	}
	delete fMessage;
}

// #pragma mark -

// QuitRequested
bool
RenderSettingsWindow::QuitRequested()
{
	be_app->PostMessage(MSG_RENDER_SETTINGS_HIDDEN);
	if (!IsHidden())
		Hide();
	return false;
}

// MessageReceived
void
RenderSettingsWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
		// general
		case MSG_OK:
			_Invoke();
			PostMessage(B_QUIT_REQUESTED, this);
			break;
		case MSG_CANCEL:
			Hide();
			if (fBackupSettings)
				_SetToRenderPreset(fBackupSettings);
			PostMessage(B_QUIT_REQUESTED, this);
			break;
		case MSG_REVERT:
			if (fBackupSettings)
				_SetToRenderPreset(fBackupSettings);
			break;
#if LANGUAGE_MANAGER
		case MSG_SET_LANGUAGE:
			UpdateStrings();
			break;
#endif // LANGUAGE_MANAGER
		// preset management messages
		case MSG_RESTORE_PRESET: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK)
				_RestorePreset(index);
				_CheckCurrentPresetName();
			break;
		}
		case MSG_STORE_IN_PRESET: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK) {
				// first commit any changes in the text controls
				_CommitTextControlChanges();
				_StoreInPreset(index);
			}
			break;
		}
		case MSG_DELETE_PRESET: {
			int32 index;
			if (msg->FindInt32("index", &index) == B_OK)
				_DeletePreset(index);
				_CheckCurrentPresetName();
			break;
		}
		// preset config messages
		case MSG_VIDEO_ACTIVE_CHANGED: {
			bool video = fVideoCB->Value();
			fCurrentSettings->SetVideo(video);
			// dis/enable all controls for video
			_SetVideoEnabled(video);
			bool audio = fAudioCB->Value();
			if (audio == false && video == false) {
				fAudioCB->SetValue(true);
				PostMessage(new BMessage(MSG_AUDIO_ACTIVE_CHANGED), this);
			}
			_CheckCurrentPresetName();
			break;
		}
		case MSG_AUDIO_ACTIVE_CHANGED: {
			bool audio = fAudioCB->Value();
			fCurrentSettings->SetAudio(audio);
			// dis/enable all controls for audio
			_SetAudioEnabled(audio);
			bool video = fVideoCB->Value();
			if (video == false && audio == false) {
				fVideoCB->SetValue(true);
				PostMessage(new BMessage(MSG_VIDEO_ACTIVE_CHANGED), this);
			}
			_CheckCurrentPresetName();
			break;
		}
		case MSG_QUALITY_CHANGED:
			fCurrentSettings->SetVideoQuality(fVidQualityS->Value() / 100.0f);
			_CheckCurrentPresetName();
			break;
		case MSG_NEW_PRESET: {
			// hack to make it possible to have a requester in front of a floating window
			// which doesn't hide and is the only window blocked by this requester.
			// as soon as the message returns from the requester, the feel is set back to B_FLOATING_SUBSET...
			SetFeel(B_NORMAL_WINDOW_FEEL);
			_CommitTextControlChanges();
			new NamePanel(_GetString(NAME_OF_RENDER_PRESET,
									 "Render Preset Name"),
						  "Render Preset", this, this,
						  new BMessage(MSG_NAME_PANEL));
			break;
		}
		case MSG_NAME_PANEL:
			// set feel back to what it should be
			SetFeel(B_FLOATING_APP_WINDOW_FEEL);
			const char* name;
			if (msg->FindString("name", &name) >= B_OK) {
				fPresets.AddItem((void *)_NewRenderPreset(name), 0);
				_BuildPresetsMenu();
				_CheckCurrentPresetName();
			}
			break;
		case MSG_DIMENSIONS_CHANGED: {
			int32 width;
			int32 height;
			if (msg->FindInt32("width", &width) == B_OK
				&& msg->FindInt32("height", &height) == B_OK) {
				fCurrentSettings->SetLineWidth(width);
				fCurrentSettings->SetLineCount(height);
				_CheckCodecs();
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_COLOR_SPACE_CHANGED: {
			color_space format;
			if (msg->FindInt32("format", (int32*)&format) == B_OK) {
				fCurrentSettings->SetColorSpace(format);
				_CheckCodecs();
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_COPYRIGHT_CHANGED:
			fCurrentSettings->SetCopyright(fCopyrightTC->Text());
			_CheckCurrentPresetName();
			break;
		case MSG_FILE_FORMAT: {
			int32 index;
			media_file_format* format = NULL;
			if (msg->FindInt32("index", &index) == B_OK
				&& (format = (media_file_format*)fFormats.ItemAt(index))) {
				_CheckTracks(format);
				fCurrentSettings->SetFormatFamily(format->pretty_name);
				fCurrentSettings->SetVideo(fVideoCB->Value() == B_CONTROL_ON);
				fCurrentSettings->SetAudio(fAudioCB->Value() == B_CONTROL_ON);
				_CheckCodecs();
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_VIDEO_CODEC: {
			media_codec_info* p;
			ssize_t size;
			if (msg->FindData(kMsgCodecField, B_SIMPLE_DATA, (const void**) &p, &size) == B_OK) {
				fCurrentSettings->SetVideoCodec(p->pretty_name);
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_AUDIO_CODEC: {
			media_codec_info* p;
			ssize_t size;
			if (msg->FindData(kMsgCodecField, B_SIMPLE_DATA, (const void**) &p, &size) == B_OK) {
				fCurrentSettings->SetAudioCodec(p->pretty_name);
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_FRAME_RATE_CHANGED: {
			float frameRate;
			if (msg->FindFloat("frame rate", &frameRate) == B_OK) {
				fCurrentSettings->SetAudioFrameRate(frameRate);
				_CheckCodecs();
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_CHANNEL_COUNT_CHANGED: {
			uint32 channels;
			if (msg->FindInt32("channel count", (int32*)&channels) == B_OK) {
				fCurrentSettings->SetAudioChannelCount(channels);
				_CheckCodecs();
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_RENDER_QUALITY_CHANGED: {
			int32 index = fRenderQualityPU->Menu()->IndexOf(fRenderQualityPU->Menu()->FindMarked());
			if (index == 0) {
				fCurrentSettings->SetRenderPreview(true);
				fCurrentSettings->SetUseAlpha(false);
			} else if (index == 1) {
				fCurrentSettings->SetRenderPreview(false);
				fCurrentSettings->SetUseAlpha(false);
			} else if (index == 3) {
				fCurrentSettings->SetRenderPreview(false);
				fCurrentSettings->SetUseAlpha(true);
			}
			_CheckCurrentPresetName();
			break;
		}
		case MSG_ENABLE_BUBBLE_HELP: {
			bool enable;
			if (msg->FindBool("enable", &enable) == B_OK)
				fBubbleHelpMI->SetMarked(enable);
			break;
		}
		case MSG_TIMECODE_ACTIVE_CHANGED: {
			bool enabled = fTimeCodeCB->Value() == B_CONTROL_ON;
			fCurrentSettings->SetTimeCodeOverlay(enabled);
			_EnableTimeCodeControls(enabled);
			_CheckCurrentPresetName();
			break;
		}
		case MSG_TIMECODE_SCALE_CHANGED: {
			float scale;
			if (msg->FindFloat("scale", &scale) == B_OK) {
				fCurrentSettings->SetTimeCodeScale(scale);
				_CheckCurrentPresetName();
			}
			break;
		}
		case MSG_TIMECODE_TRANSPARENCY_CHANGED: {
			float transparency;
			if (msg->FindFloat("transparency", &transparency) == B_OK) {
				fCurrentSettings->SetTimeCodeTransparency(transparency);
				_CheckCurrentPresetName();
			}
			break;
		}
		default:
			BWindow::MessageReceived(msg);
	}
}

// Show
void
RenderSettingsWindow::Show()
{
	delete fBackupSettings;
	fBackupSettings = new RenderPreset("backup", fCurrentSettings);
	BWindow::Show();
}

// Hide
void
RenderSettingsWindow::Hide()
{
	delete fBackupSettings;
	fBackupSettings = NULL;
	_CommitTextControlChanges();
	BWindow::Hide();
}

// #pragma mark -

#if 0
// XMLStore
status_t
RenderSettingsWindow::XMLStore(XMLHelper& xmlHelper) const
{
	status_t error = B_OK;
	if ((error = xmlHelper.CreateTag("WINDOW_FRAME")) == B_OK) {
		xmlHelper.SetAttribute("left", Frame().left);
		xmlHelper.SetAttribute("top", Frame().top);
		xmlHelper.SetAttribute("right", Frame().right);
		xmlHelper.SetAttribute("bottom", Frame().bottom);
		xmlHelper.CloseTag();	// WINDOW_FRAME
	}
	if ((error = xmlHelper.CreateTag("RENDER_PRESETS")) == B_OK) {
		for (int32 i = 0;
			 RenderPreset* preset = (RenderPreset*)fPresets.ItemAt(i);
			 i++) {
			if ((error = xmlHelper.CreateTag("PRESET")) == B_OK) {
				xmlHelper.StoreObject(preset);
				xmlHelper.CloseTag();	// PRESET
			} else
				printf("ERROR: creating PRESETS tag failed!\n");
		}
		// add current settings as another preset
		if ((error = xmlHelper.CreateTag("CURRENT_SETTINGS")) == B_OK) {
			xmlHelper.StoreObject(fCurrentSettings);
			xmlHelper.CloseTag();	// CURRENT_SETTINGS
		}
		xmlHelper.CloseTag();	// RENDER_PRESETS
	} else
		printf("ERROR: creating RENDER_PRESETS tag failed!\n");
	return B_OK;
}

// XMLRestore
status_t
RenderSettingsWindow::XMLRestore(XMLHelper& xmlHelper)
{
	// clean up the existing fPresets
	_CleanupPresets();
	// adjust window frame to the settings inside that file
	float left = 50.0f, top = 50.0f, right = 100.0f, bottom = 100.0f;
	if (xmlHelper.OpenTag("WINDOW_FRAME") == B_OK) {
		left = xmlHelper.GetAttribute("left", left);
		top = xmlHelper.GetAttribute("top", top);
		right = xmlHelper.GetAttribute("right", right);
		bottom = xmlHelper.GetAttribute("bottom", bottom);
		xmlHelper.CloseTag();	// WINDOW_FRAME
	}
	if (left >= 0.0f && right >= left + 50.0f
		&& top >= 0.0f && bottom >= top + 50.0f) {
		BRect frame(left, top, right, bottom);
		BScreen screen;
		if (screen.Frame().Contains(frame)) {
			ResizeTo(frame.Width(), frame.Height());
			MoveTo(frame.LeftTop());
		}
	}
	if (xmlHelper.OpenTag("RENDER_PRESETS") == B_OK) {
		while (xmlHelper.OpenTag("PRESET") == B_OK) {
			RenderPreset* preset = new RenderPreset();
			if (xmlHelper.RestoreObject(preset) == B_OK) {
				fPresets.AddItem(preset);
			} else {
				delete preset;
				printf("ERROR: restoring PRESET failed!\n");
			}
			xmlHelper.CloseTag();	// PRESET
		}
		if (xmlHelper.OpenTag("CURRENT_SETTINGS") == B_OK) {
			if (xmlHelper.RestoreObject(fCurrentSettings) != B_OK)
				printf("ERROR: failed to restore current render settings!\n");
			xmlHelper.CloseTag();	// CURRENT_SETTINGS
		}
		xmlHelper.CloseTag();	// RENDER_PRESETS
	} else
		printf("ERROR: opening RENDER_PRESETS tag failed!\n");

	return B_OK;
}
#endif


// StoreSettings
status_t
RenderSettingsWindow::StoreSettings(BMessage* archive) const
{
	status_t ret = archive->AddRect("window frame", Frame());
	BMessage renderPresets;
	if (ret == B_OK) {
		for (int32 i = 0;
				RenderPreset* preset = (RenderPreset*)fPresets.ItemAt(i);
			 	i++) {
			BMessage presetArchive;
			ret = preset->Archive(&presetArchive);
			if (ret != B_OK)
				break;
			ret = renderPresets.AddMessage("render preset", &presetArchive);
			if (ret != B_OK)
				break;
		}
		if (ret == B_OK) {
			BMessage presetArchive;
			ret = fCurrentSettings->Archive(&presetArchive);
			if (ret == B_OK) {
				ret = renderPresets.AddMessage("current preset",
					&presetArchive);
			}
		}
	}
	if (ret == B_OK)
		ret = archive->AddMessage("render presets", &renderPresets);

	return ret;
}

// RestoreSettings
status_t
RenderSettingsWindow::RestoreSettings(const BMessage* archive)
{
	// clean up the existing fPresets
	_CleanupPresets();
	// adjust window frame to the settings inside that file
	BRect frame(50.0f, 50.0f, 100.0f, 100.0f);
	if (archive->FindRect("window frame", &frame) == B_OK) {
		BScreen screen(this);
		if (screen.Frame().Contains(frame)) {
			ResizeTo(frame.Width(), frame.Height());
			MoveTo(frame.LeftTop());
		}
	}
	BMessage presets;
	if (archive->FindMessage("render presets", &presets) == B_OK) {
		BMessage presetArchive;
		for (int32 i = 0; presets.FindMessage("render preset", i,
			&presetArchive) == B_OK; i++) {
			RenderPreset* preset = new(std::nothrow) RenderPreset();
			if (preset == NULL
				|| preset->Unarchive(&presetArchive) != B_OK
				|| !fPresets.AddItem(preset)) {
				delete preset;
				printf("ERROR: restoring RenderPreset failed!\n");
			}
		}
		if (presets.FindMessage("current preset", &presetArchive) == B_OK) {
			if (fCurrentSettings->Unarchive(&presetArchive) != B_OK)
				printf("ERROR: failed to restore current render settings!\n");
		} else
			printf("ERROR: failed to find current render settings!\n");
	} else
		printf("ERROR: restoring any RenderPreset failed!\n");

	return B_OK;
}

// #pragma mark -

// RenderPresetInfo
const RenderPreset*
RenderSettingsWindow::RenderPresetInfo()
{
	return fCurrentSettings;
}

// SaveSettings
void
RenderSettingsWindow::SaveSettings()
{
	BAutolock _(this);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kM3BaseSettingsName) != B_OK
		|| path.Append("clockwerk_render_settings") != B_OK) {
		printf("ERROR: user settings folder not found!\n");
		return;
	}
	BFile settingsFile(path.Path(), B_CREATE_FILE | B_ERASE_FILE
		| B_WRITE_ONLY);
	status_t ret = settingsFile.InitCheck();
	if (ret != B_OK) {
		printf("ERROR: Unable to create settings file: %s\n",
			strerror(ret));
		return;
	}

	BMessage archive;
	ret = StoreSettings(&archive);
	if (ret != B_OK) {
		fprintf(stderr, "ERROR: Error storing render settings: %s\n",
			strerror(ret));
		return;
	}

	ret = archive.Flatten(&settingsFile);
	if (ret < B_OK) {
		fprintf(stderr, "ERROR: Error flatting render settings: %s\n",
			strerror(ret));
	}
}

// LoadSettings
void
RenderSettingsWindow::LoadSettings()
{
	BAutolock _(this);

	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path) != B_OK
		|| path.Append(kM3BaseSettingsName) != B_OK
		|| path.Append("clockwerk_render_settings") != B_OK) {
		printf("ERROR: user settings folder not found!\n");
		return;
	}
	BFile settingsFile(path.Path(), B_READ_ONLY);
	status_t ret = settingsFile.InitCheck();
	if (ret != B_OK) {
		printf("ERROR: Unable to open settings file: %s\n",
			strerror(ret));
		return;
	}

	BMessage archive;
	ret = archive.Unflatten(&settingsFile);
	if (ret < B_OK) {
		fprintf(stderr, "ERROR: Error unflatting render settings: %s\n",
			strerror(ret));
	}

	ret = RestoreSettings(&archive);
	if (ret != B_OK) {
		fprintf(stderr, "ERROR: Error restoring render settings: %s\n",
			strerror(ret));
		return;
	}
}

// UpdateStrings
void
RenderSettingsWindow::UpdateStrings()
{
	SetTitle(_GetString(RENDER_SETTINGS, "Render Settings"));
	// menu bar labels
	fPresetsM->Superitem()->SetLabel(_GetString(SETTINGS, "Settings"));
	fHelpM->Superitem()->SetLabel(_GetString(HELP, "Help"));
	fBubbleHelpMI->SetLabel(_GetString(BUBBLE_HELP, "Bubble Help"));
	BString string(_GetString(RENDER_SETTINGS_WINDOW, "Render Settings Window"));
	string << B_UTF8_ELLIPSIS;
	fHelpMI->SetLabel(string.String());
	// popup menu items
	fSmallMI->SetLabel(_GetString(SMALL, "Small"));
	fMediumMI->SetLabel(_GetString(MEDIUM, "Medium"));
	fLargeMI->SetLabel(_GetString(LARGE, "Large"));
	fTransparentMI->SetLabel(_GetString(TRANSPARENT, "Semi Transparent"));
	fMediumTransMI->SetLabel(_GetString(MEDIUM, "Medium"));
	fOpaqueMI->SetLabel(_GetString(OPAQUE, "Opaque"));
	fRGB15MI->SetLabel(_GetString(RGB_32K_COLORS, "RGB (16 bpp, 32K colors)"));
	fRGB16MI->SetLabel(_GetString(RGB_64K_COLORS, "RGB (16 bpp, 64K colors)"));
	fRGB24MI->SetLabel(_GetString(RGB24_16M_COLORS, "RGB (24 bpp, 16M colors)"));
	fRGB32MI->SetLabel(_GetString(RGB32_16M_COLORS, "RGB (32 bpp, 16M colors)"));
	fYCbCr422MI->SetLabel(_GetString(YCBCR_16M_COLORS, "YCbCr 4:2:2 (16 bpp, 16M colors)"));
	fMonoMI->SetLabel(_GetString(MONO, "Mono"));
	fStereoMI->SetLabel(_GetString(STEREO, "Stereo"));
	fBWPreviewMI->SetLabel(_GetString(BW_PREVIEW, "Black & White (Animation preview)"));
	fFullColorMI->SetLabel(_GetString(FULL_COLOR, "Full Color (Load frames from disk)"));
	fFullColorAlphaMI->SetLabel(_GetString(FULL_COLOR_ALPHA, "Full Color (Alphachannel)"));
	// update control labels
	fDimensionsC->SetLabels(_GetString(WIDTH, "Width"),
							_GetString(HEIGHT, "Height"));
	fFormatPU->SetLabel(_GetString(OUTPUT_AS, "Output As"));
	fVidCodecPU->SetLabel(_GetString(CODEC, "Codec"));
	fColorSpacePU->SetLabel(_GetString(COLOR_SPACE, "Color Space"));
	fAudCodecPU->SetLabel(_GetString(CODEC, "Codec"));
	fAudFrameRatePU->SetLabel(_GetString(SAMPLE_RATE, "Sample Rate"));
	fRenderQualityPU->SetLabel(_GetString(RENDER, "Render"));
	fAudChannelCountPU->SetLabel(_GetString(CHANNELS, "Channels"));
	fVidQualityS->SetLabel(_GetString(QUALITY, "Quality"));
	fCopyrightTC->SetLabel(_GetString(COPYRIGHT_INFO, "Copyright Info"));
	fVideoCB->SetLabel(_GetString(VIDEO, "Video"));
	fAudioCB->SetLabel(_GetString(AUDIO, "Audio"));
	fTimeCodeCB->SetLabel(_GetString(RENDER_TIME_CODE, "Render Time Code"));
	fTimeCodeSizePU->SetLabel(_GetString(SIZE, "Size"));
	fTimeCodeTransparencyPU->SetLabel(_GetString(TRANSPARENCY, "Transparency"));
	// button labels
	fOkB->SetLabel(_GetString(OK, "Ok"));
	fCancelB->SetLabel(_GetString(CANCEL, "Cancel"));
	fRevertB->SetLabel(_GetString(REVERT, "Revert"));
	// set help texts
#if BUBBLE_HELPER
	BubbleHelper* helper = BubbleHelper::GetDefault();
	helper->SetHelp(fFormatPU,
					_GetString(FILE_FORMAT_TIP,
							   "This will be the file format used for rendering.\n"
							   "The available file formats are split into\n"
							   "video/audio and audio only formats."));
	helper->SetHelp(fVidCodecPU,
					_GetString(VIDEO_CODEC_TIP,
							   "Encoder used for the video track.\n"
							   "Available codecs depend on the\n"
							   "file format, resolution and color\n"
							   "space."));
	helper->SetHelp(fAudCodecPU,
					_GetString(AUDIO_CODEC_TIP,
							   "Encoder used for the audio track.\n"
							   "Available codecs depend on the\n"
							   "file format."));
	helper->SetHelp(fAudFrameRatePU,
					_GetString(AUDIO_FRAME_RATE_TIP,
							   "Sample rate of the audio track."));
	helper->SetHelp(fAudChannelCountPU,
					_GetString(AUDIO_CHANNEL_COUNT_TIP,
							   "Number of channels in the audio track."));
	helper->SetHelp(fVidQualityS,
					_GetString(VIDEP_QUALITY_TIP,
							   "Quality parameter for the encoder."));
	helper->SetHelp(fDimensionsC,
					_GetString(VIDEO_DIMENSIONS_TIP,
							   "Size of the encoded video frames."));
	helper->SetHelp(fCopyrightTC,
					_GetString(COPYRIGHT_TIP,
							   "Copyright string that will be\n"
							   "put into the output file."));
	helper->SetHelp(fVideoCB,
					_GetString(VIDEO_CHECK_TIP,
							   "Specifies, if the output file should contain video."));
	helper->SetHelp(fAudioCB,
					_GetString(AUDIO_CHECK_TIP,
							   "Specifies, if the output file should contain audio."));
	helper->SetHelp(fColorSpacePU,
					_GetString(RENDER_COLOR_SPACE_TIP,
							   "Sets the color space of the video data.\n"
							   "15 or 16 bits per pixel RGB data mean\n"
							   "32000 or 64000 colors.\n"
							   "24 or 32 bits per pixel RGB data mean\n"
							   "16.7 million colors.\n"
							   "16 bits per pixel YCrCb data also means\n"
							   "16.7 million colors, but stored in 4:2:2\n"
							   "format (less bandwidth).\n\n"
							   "Most codecs, except the RAW codec will only\n"
							   "accept 32 bits RGB data!"));
	helper->SetHelp(fRenderQualityPU,
					_GetString(RENDER_QUALITY_TIP,
							   "If set to \"Black & White\",\n"
							   "it will render the animation\n"
							   "exactly as seen in the preview.\n"
							   "Otherwise it will render the\n"
							   "frames stored on disk."));
	helper->SetHelp(fTimeCodeCB,
					_GetString(TIMECODE_TIP,
							   "If checked a time code string will be\n"
							   "rendered at the bottom of the video."));
	helper->SetHelp(fTimeCodeSizePU,
					_GetString(TIMECODE_SIZE_TIP,
							   "Size of the time code string."));
	helper->SetHelp(fTimeCodeTransparencyPU,
					_GetString(TIMECODE_TRANSPARENCY_TIP,
							   "Visibility level of the time code string"
							   "(will be blended with the video)."));
#endif // BUBBLE_HELPER

	_BuildPresetsMenu();

	fVidCodecPU->RefreshItemLabel();
	fColorSpacePU->RefreshItemLabel();
	fAudCodecPU->RefreshItemLabel();
	fAudFrameRatePU->RefreshItemLabel();
	fRenderQualityPU->RefreshItemLabel();
	fAudChannelCountPU->RefreshItemLabel();
	fTimeCodeSizePU->RefreshItemLabel();
	fTimeCodeTransparencyPU->RefreshItemLabel();

//	DivideSame(fVidCodecPU,
//			   fColorSpacePU,
//			   fDimensionsC->WidthControl(),
//			   fDimensionsC->HeightControl(),
//			   fRenderQualityPU,
//			   NULL);
//
//	DivideSame(fAudChannelCountPU, fAudFrameRatePU,
//			   fAudCodecPU,
//			   fTimeCodeSizePU,
//			   fTimeCodeTransparencyPU,
//			   NULL);
//
//	RecalcSize();
}

// SetMessage
void
RenderSettingsWindow::SetMessage(BMessage* message)
{
	delete fMessage;
	fMessage = message;
}

// #pragma mark -

// _Init
void
RenderSettingsWindow::_Init()
{
	if (!Lock())
		return;

//	_BuildFormatMenu();
	_BuildVideoCodecMenu();
	_BuildAudioCodecMenu();
	LoadSettings();

	// rebuild the presets menu
	_BuildPresetsMenu();
	_SetToRenderPreset(fCurrentSettings);
	_CheckCurrentPresetName();

	Unlock();
}

// _InitEntry
int32
RenderSettingsWindow::_InitEntry(void* cookie)
{
	if (RenderSettingsWindow* window = (RenderSettingsWindow*)cookie) {
		window->_Init();
	}
	return B_OK;
}

// _Invoke
void
RenderSettingsWindow::_Invoke()
{
	if (!fMessage)
		return;

	BHandler* target = fTarget ? fTarget : be_app;

	BLooper* looper = target->Looper();
	if (!looper)
		return;

	looper->PostMessage(fMessage, target);
}

// _NewRenderPreset
RenderPreset*
RenderSettingsWindow::_NewRenderPreset(const char *name)
{
	// clone current settings in new preset
	RenderPreset *preset = new RenderPreset(name, fCurrentSettings);
	return preset;
}


// _SetToRenderPreset
void
RenderSettingsWindow::_SetToRenderPreset(RenderPreset *preset)
{
	if (preset && preset != fCurrentSettings) {
		// put settings of preset into current settings
/*		fCurrentSettings->SetLineWidth(preset->LineWidth());
		fCurrentSettings->SetLineCount(preset->LineCount());
		fCurrentSettings->SetColorSpace(preset->ColorSpace());
		fCurrentSettings->SetFormatFamily(preset->FormatFamily().String());
		fCurrentSettings->SetCopyright(preset->Copyright().String());
		fCurrentSettings->SetQuality(preset->Quality());
		fCurrentSettings->SetVideoCodec(preset->VideoCodec().String());
		fCurrentSettings->SetAudioCodec(preset->AudioCodec().String());
		fCurrentSettings->SetVideo(preset->IsVideo());
		fCurrentSettings->SetAudio(preset->IsAudio());
		fCurrentSettings->SetRenderPreview(preset->RenderPreview());
		fCurrentSettings->SetUseAlpha(preset->UseAlpha());
		fCurrentSettings->SetAudioChannelCount(preset->AudioChannelCount());
		fCurrentSettings->SetAudioFrameRate(preset->AudioFrameRate());
		fCurrentSettings->SetTimeCodeOverlay(preset->TimeCodeOverlay());
		fCurrentSettings->SetTimeCodeTransparency(preset->TimeCodeTransparency());
		fCurrentSettings->SetTimeCodeScale(preset->TimeCodeScale());
		*/
		*fCurrentSettings = *preset;
	}

	// set the gadgets to their new values
	fDimensionsC->SetDimensions(fCurrentSettings->LineWidth(), fCurrentSettings->LineCount());
	// TODO: mark correct menu item in color space popup!
	fVidQualityS->SetValue(fCurrentSettings->VideoQuality() * 100.0f);
	fCopyrightTC->SetText(fCurrentSettings->Copyright().String());

	_SetVideoEnabled(fCurrentSettings->IsVideo());
	_SetAudioEnabled(fCurrentSettings->IsAudio());
	fVideoCB->SetValue(fCurrentSettings->IsVideo());
	fAudioCB->SetValue(fCurrentSettings->IsAudio());

	fTimeCodeCB->SetValue(fCurrentSettings->TimeCodeOverlay());

	if (BMenuItem *markedRenderItem = fRenderQualityPU->Menu()->FindMarked())
		markedRenderItem->SetMarked(false);

	// mark the correct render quality item
	BMenuItem *renderQualityItem;
	if (fCurrentSettings->RenderPreview()) {
		renderQualityItem = fRenderQualityPU->Menu()->ItemAt(0L);
	} else {
		if (fCurrentSettings->UseAlpha()) {
			renderQualityItem = fRenderQualityPU->Menu()->ItemAt(3L);
		} else {
			renderQualityItem = fRenderQualityPU->Menu()->ItemAt(1L);
		}
	}
	if (renderQualityItem)
		renderQualityItem->SetMarked(true);

	// mark the correct color space item
	for (int32 i = 0; BMenuItem* item = fColorSpacePU->Menu()->ItemAt(i); i++) {
		if (BMessage* msg = item->Message()) {
			color_space format;
			if (msg->FindInt32("format", (int32*)&format) == B_OK
				&& format == fCurrentSettings->ColorSpace()) {
				item->SetMarked(true);
				break;
			}
		}
	}

	// mark the correct frame rate item
	for (int32 i = 0; BMenuItem* item = fAudFrameRatePU->Menu()->ItemAt(i); i++) {
		if (BMessage* msg = item->Message()) {
			float frameRate;
			if (msg->FindFloat("frame rate", &frameRate) == B_OK
				&& frameRate == fCurrentSettings->AudioFrameRate()) {
				item->SetMarked(true);
				break;
			}
		}
	}

	// mark the correct channel count item
	for (int32 i = 0; BMenuItem* item = fAudChannelCountPU->Menu()->ItemAt(i); i++) {
		if (BMessage* msg = item->Message()) {
			uint32 channels;
			if (msg->FindInt32("channel count", (int32*)&channels) == B_OK
				&& channels == fCurrentSettings->AudioChannelCount()) {
				item->SetMarked(true);
				break;
			}
		}
	}

	// mark the correct time code scale item
	for (int32 i = 0; BMenuItem* item = fTimeCodeSizePU->Menu()->ItemAt(i); i++) {
		if (BMessage* msg = item->Message()) {
			float scale;
			if (msg->FindFloat("scale", &scale) == B_OK
				&& scale == fCurrentSettings->TimeCodeScale()) {
				item->SetMarked(true);
				break;
			}
		}
	}

	// mark the correct time code transparency item
	for (int32 i = 0; BMenuItem* item = fTimeCodeTransparencyPU->Menu()->ItemAt(i); i++) {
		if (BMessage* msg = item->Message()) {
			float transparency;
			if (msg->FindFloat("transparency", &transparency) == B_OK
				&& transparency == fCurrentSettings->TimeCodeTransparency()) {
				item->SetMarked(true);
				break;
			}
		}
	}

	// find the correct format menu item and select it
	bool found = false;
	for (int32 i = 0; BMenuItem *item = fFormatM->ItemAt(i); i++) {
		if (strcmp(item->Label(), fCurrentSettings->FormatFamily().String()) == 0) {
			item->SetMarked(true);
			found = true;
		}
	}
	if (!found) {
		// select the first file format we find
		if (BMenuItem* item = fFormatM->ItemAt(0)) {
			item->SetMarked(true);
			fCurrentSettings->SetFormatFamily(item->Label());
		}
	}
	for (int32 i = 0; BMenuItem *item = fVidCodecPU->Menu()->ItemAt(i); i++) {
		item->SetEnabled(true);
		if (strcmp(item->Label(), fCurrentSettings->VideoCodec().String()) == 0)
			item->SetMarked(true);
		else
			item->SetMarked(false);
	}
	for (int32 i = 0; BMenuItem *item = fAudCodecPU->Menu()->ItemAt(i); i++) {
		item->SetEnabled(true);
		if (strcmp(item->Label(), fCurrentSettings->AudioCodec().String()) == 0)
			item->SetMarked(true);
		else
			item->SetMarked(false);
	}
	// this will check the validity of the selected codecs
	// it will also select new codecs if we did not select one
	// further, as we enabled all items above, it will disable
	// invalid codecs again
	_CheckCodecs();
	// this will set the availablility of the check marks for the tracks correctly
	for (int32 i = 0; media_file_format* mff = (media_file_format*)fFormats.ItemAt(i); i++) {
		if (strcmp(mff->pretty_name, fCurrentSettings->FormatFamily().String()) == 0) {
			_CheckTracks(mff);
			break;
		}

	}
}

// _BuildPresetsMenu
void
RenderSettingsWindow::_BuildPresetsMenu()
{
	// remove old contents from settings menu
	while (BMenuItem* item = fPresetsM->RemoveItem(fPresetsM->CountItems() - 1))
		delete item;
	// add sub menu structure to settings menu
	BMenu* restoreMenu = new BMenu(_GetString(LOAD, "Load"));
	fPresetsM->AddItem(restoreMenu);
	BMenu* storeMenu = new BMenu(_GetString(SAVE_AS, "Save As"));
	BMenuItem* item = new BMenuItem(_GetString(NEW, "New"),
									new BMessage(MSG_NEW_PRESET));
	storeMenu->AddItem(item);
	storeMenu->AddSeparatorItem();
	fPresetsM->AddItem(storeMenu);
	fPresetsM->AddSeparatorItem();
	BMenu* deleteMenu = new BMenu(_GetString(DELETE, "Delete"));
	fPresetsM->AddItem(deleteMenu);
	// add fPresets into sub menus
	for (int32 i = 0; RenderPreset* preset = (RenderPreset*)fPresets.ItemAt(i); i++) {
		// add preset to restore sub menu
		BMessage *message = new BMessage(MSG_RESTORE_PRESET);
		message->AddInt32("index", i);
		item = new BMenuItem(preset->Name().String(), message);
		restoreMenu->AddItem(item);
		// add preset to store sub menu
		message = new BMessage(MSG_STORE_IN_PRESET);
		message->AddInt32("index", i);
		item = new BMenuItem(preset->Name().String(), message);
		storeMenu->AddItem(item);
		// add preset to delete sub menu
		message = new BMessage(MSG_DELETE_PRESET);
		message->AddInt32("index", i);
		item = new BMenuItem(preset->Name().String(), message);
		deleteMenu->AddItem(item);
	}
}

// _BuildFormatMenu
void
RenderSettingsWindow::_BuildFormatMenu()
{
	// remove the current contents of the menu
	BMenuItem* item;
	while ((item = fFormatM->RemoveItem(int32(0))) != NULL) {
		delete item;
	}
	// fill in the list of available file formats
	media_file_format mff;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		if (mff.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO
			|| mff.capabilities & media_file_format::B_KNOWS_ENCODED_AUDIO) {
			// add format to our list for caching
			media_file_format* format = new media_file_format;
			memcpy(format, &mff, sizeof(media_file_format));
			fFormats.AddItem((void*)format);
		}
	}
	// put video formats first into menu
	fFormatM->AddSeparatorItem();
	for (int32 i = 0; media_file_format* mff = (media_file_format*)fFormats.ItemAt(i); i++) {
		BMessage* msg = new BMessage(MSG_FILE_FORMAT);
		msg->AddInt32(kMsgFamilyField, mff->family);
		msg->AddInt32("index", i);
		item = new BMenuItem(mff->pretty_name, msg);
		if (mff->capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO)
			fFormatM->AddItem(item, 0);
		else
			fFormatM->AddItem(item, fFormatM->CountItems());
	}
	fFormatM->SetTargetForItems(this);
}

// _BuildVideoCodecMenu
void
RenderSettingsWindow::_BuildVideoCodecMenu()
{
	// clear previous contents of menu
	while (BMenuItem* item = fVidCodecPU->Menu()->RemoveItem(int32(0)))
		delete item;
	// put all found video encoders into the menu
	media_format inputFormat;
	media_format outputFormat;
	media_codec_info codecInfo;
	int32 cookie = 0;
	// clear out format descriptions
	memset(&inputFormat, 0, sizeof(media_format));
	memset(&outputFormat, 0, sizeof(media_format));
	// specialize input and output format to video
	inputFormat.type = B_MEDIA_RAW_VIDEO;
	outputFormat.type = B_MEDIA_ENCODED_VIDEO;
	// iterate through installed video codecs
	while (get_next_encoder(&cookie, (media_file_format*)NULL,
							&inputFormat, &outputFormat, &codecInfo) == B_OK) {
		BMessage* msg = new BMessage(MSG_VIDEO_CODEC);
		msg->AddData(kMsgCodecField, B_SIMPLE_DATA, &codecInfo, sizeof(media_codec_info));
		BMenuItem* item = new BMenuItem(codecInfo.pretty_name, msg);
		fVidCodecPU->Menu()->AddItem(item);
	}
}


// _BuildAudioCodecMenu
void
RenderSettingsWindow::_BuildAudioCodecMenu()
{
	// clear previous contents of menu
	while (BMenuItem* item = fAudCodecPU->Menu()->RemoveItem(int32(0)))
		delete item;
	// put all found video encoders into the menu
	media_format inputFormat;
	media_format outputFormat;
	media_codec_info codecInfo;
	int32 cookie = 0;
	// clear out format descriptions
	memset(&inputFormat, 0, sizeof(media_format));
	memset(&outputFormat, 0, sizeof(media_format));
	// specialize input and output format to video
	inputFormat.type = B_MEDIA_RAW_AUDIO;
	outputFormat.type = B_MEDIA_ENCODED_AUDIO;
	// iterate through installed video codecs
	while (get_next_encoder(&cookie, (media_file_format*)NULL,
							&inputFormat, &outputFormat, &codecInfo) == B_OK) {
		BMessage* msg = new BMessage(MSG_AUDIO_CODEC);
		msg->AddData(kMsgCodecField, B_SIMPLE_DATA, &codecInfo, sizeof(media_codec_info));
		BMenuItem* item = new BMenuItem(codecInfo.pretty_name, msg);
		fAudCodecPU->Menu()->AddItem(item);
	}
}

// _CleanupPresets
void
RenderSettingsWindow::_CleanupPresets()
{
	for (int32 i = 0;
		 RenderPreset *preset = (RenderPreset *)fPresets.ItemAt(i);
		 i++) {
		delete preset;
	}
	fPresets.MakeEmpty();
}

// _RestorePreset
void
RenderSettingsWindow::_RestorePreset(int32 index)
{
	if (RenderPreset* preset = (RenderPreset*)fPresets.ItemAt(index)) {
		_SetToRenderPreset(preset);
	}
}

// _StoreInPreset
void
RenderSettingsWindow::_StoreInPreset(int32 index)
{
	if (RenderPreset* preset = (RenderPreset*)fPresets.ItemAt(index))
		preset->AdoptValues(*fCurrentSettings);
}

// _DeletePreset
void
RenderSettingsWindow::_DeletePreset(int32 index)
{
	if (RenderPreset* preset = (RenderPreset*)fPresets.RemoveItem(index)) {
		delete preset;
		_BuildPresetsMenu();
		// TODO: ? That's all?
	}
}

// _CheckCodecs
void
RenderSettingsWindow::_CheckCodecs()
{
	// find the currently selected file format
	media_file_format *fileFormat = NULL;
	for (int32 i = 0; media_file_format* mff = (media_file_format*)fFormats.ItemAt(i); i++) {
		if (strcmp(mff->pretty_name, fCurrentSettings->FormatFamily().String()) == 0) {
			fileFormat = mff;
			break;
		}
	}
	if (fileFormat == NULL) {
//		printf("ERROR: current file format not among installed! (%ld)\n", fFormats.CountItems());
		return;
	}
	// disable all menu items in the encoder menu,
	// enable them again if codec supports current settings
	int32 markedVidItem = -1;
	int32 markedAudItem = -1;
	for (int32 i = 0; BMenuItem* item = fVidCodecPU->Menu()->ItemAt(i); i++) {
		if (item->IsMarked())
			markedVidItem = i;
		item->SetEnabled(false);
	}
	for (int32 i = 0; BMenuItem* item = fAudCodecPU->Menu()->ItemAt(i); i++) {
		if (item->IsMarked())
			markedAudItem = i;
		item->SetEnabled(false);
	}
	// search for all encoders that work with the current settings
	media_format inputFormat;
	media_format outputFormat;
	media_codec_info codecInfo;
	int32 cookie = 0;
	// clear out format descriptions
	memset(&inputFormat, 0, sizeof(media_format));
	memset(&outputFormat, 0, sizeof(media_format));
	// specialize input and output format to video
	inputFormat.type = B_MEDIA_RAW_VIDEO;
	inputFormat.u.raw_video.display.line_width = fCurrentSettings->LineWidth();
	inputFormat.u.raw_video.display.line_count = fCurrentSettings->LineCount();
	inputFormat.u.raw_video.last_active = inputFormat.u.raw_video.display.line_count - 1;
	inputFormat.u.raw_video.display.format = fCurrentSettings->ColorSpace();
	outputFormat.type = B_MEDIA_ENCODED_VIDEO;
	// iterate through installed video codecs
	bool selectFirst = true;
	while (get_next_encoder(&cookie, fileFormat,
							&inputFormat, &outputFormat, &codecInfo) == B_OK) {
		for (int32 i = 0; BMenuItem* item = fVidCodecPU->Menu()->ItemAt(i); i++) {
			if (strcmp(item->Label(), codecInfo.pretty_name) == 0) {
				item->SetEnabled(true);
				if (i == markedVidItem) {
					// found current codec
					item->SetMarked(true);
					selectFirst = false;
				}
			}
		}
	}
	// if the currently selected codec does not support the settings, select the first that can
	if (selectFirst) {
		for (int32 i = 0; BMenuItem* item = fVidCodecPU->Menu()->ItemAt(i); i++)
			if (item->IsEnabled()) {
				item->SetMarked(true);
				fCurrentSettings->SetVideoCodec(item->Label());
				break;
			}
	}
	// specialize format for audio
	memset(&inputFormat, 0, sizeof(media_format));
	memset(&outputFormat, 0, sizeof(media_format));
	inputFormat.type = B_MEDIA_RAW_AUDIO;
	inputFormat.u.raw_audio.channel_count = fCurrentSettings->AudioChannelCount();
	inputFormat.u.raw_audio.frame_rate = fCurrentSettings->AudioFrameRate();
	outputFormat.type = B_MEDIA_ENCODED_AUDIO;
	// iterate through installed audio codecs
	cookie = 0;
	selectFirst = true;
	while (get_next_encoder(&cookie, fileFormat,
							&inputFormat, &outputFormat, &codecInfo) == B_OK) {
		for (int32 i = 0; BMenuItem* item = fAudCodecPU->Menu()->ItemAt(i); i++)
			if (strcmp(item->Label(), codecInfo.pretty_name) == 0) {
				item->SetEnabled(true);
				if (i == markedAudItem) {
					// found current codec
					item->SetMarked(true);
					selectFirst = false;
				}
			}
	}
	// if the currently selected codec does not support the settings, select the first that can
	if (selectFirst) {
		for (int32 i = 0; BMenuItem* item = fAudCodecPU->Menu()->ItemAt(i); i++)
			if (item->IsEnabled()) {
				item->SetMarked(true);
				fCurrentSettings->SetAudioCodec(item->Label());
				break;
			}
	}
}

// _SetVideoEnabled
void
RenderSettingsWindow::_SetVideoEnabled(bool enabled)
{
	fVidCodecPU->SetEnabled(enabled);
	fVidQualityS->SetEnabled(enabled);
	fDimensionsC->SetEnabled(enabled);
	fColorSpacePU->SetEnabled(enabled);
	fRenderQualityPU->SetEnabled(enabled);
	fTimeCodeCB->SetEnabled(enabled);
	if (enabled)
		_EnableTimeCodeControls(fCurrentSettings->TimeCodeOverlay());
	else
		_EnableTimeCodeControls(false);
}

// _SetAudioEnabled
void
RenderSettingsWindow::_SetAudioEnabled(bool enabled)
{
	fAudCodecPU->SetEnabled(enabled);
	fAudChannelCountPU->SetEnabled(enabled);
	fAudFrameRatePU->SetEnabled(enabled);
}

// _CheckTracks
void
RenderSettingsWindow::_CheckTracks(media_file_format* format)
{
	if (format) {
		if (format->capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO
			&& format->capabilities & media_file_format::B_KNOWS_ENCODED_AUDIO) {
			// format supports both video and audio
			// the user gets to pick wich tracks to include
			fVideoCB->SetEnabled(true);
			fAudioCB->SetEnabled(true);
			_SetVideoEnabled(fVideoCB->Value() == B_CONTROL_ON);
			_SetAudioEnabled(fAudioCB->Value() == B_CONTROL_ON);
		} else {
			// user does not get to pick wich tracks to include!
			fVideoCB->SetEnabled(false);
			fAudioCB->SetEnabled(false);
			if (format->capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO) {
				fVideoCB->SetValue(true);
				_SetVideoEnabled(true);
				fAudioCB->SetValue(false);
				_SetAudioEnabled(false);
			} else {
				fVideoCB->SetValue(false);
				_SetVideoEnabled(false);
				fAudioCB->SetValue(true);
				_SetAudioEnabled(true);
			}
		}
	}
}

// _CommitTextControlChanges
void
RenderSettingsWindow::_CommitTextControlChanges()
{
	bool changed = false;
	if (fDimensionsC->Width() != fCurrentSettings->LineWidth()) {
		fCurrentSettings->SetLineWidth(fDimensionsC->Width());
		changed = true;
	}
	if (fDimensionsC->Height() != fCurrentSettings->LineCount()) {
		fCurrentSettings->SetLineCount(fDimensionsC->Height());
		changed = true;
	}
	fCurrentSettings->SetCopyright(fCopyrightTC->Text());
	if (changed)
		_CheckCodecs();
}

// _CheckCurrentPresetName
void
RenderSettingsWindow::_CheckCurrentPresetName()
{
	BString presetName("");
	for (int32 i = 0; RenderPreset* preset = (RenderPreset*)fPresets.ItemAt(i); i++) {
		if (fCurrentSettings->HasSameValues(*preset)) {
			if (presetName.CountChars() > 0)
				presetName << ", ";
			presetName << preset->Name();
		}
	}
	BString title("RenderSettings");
	if (presetName.CountChars())
		title << " (" << presetName << ")";
	SetTitle(title.String());
}

// _EnableTimeCodeControls
void
RenderSettingsWindow::_EnableTimeCodeControls(bool enable)
{
	fTimeCodeSizePU->SetEnabled(enable);
	fTimeCodeTransparencyPU->SetEnabled(enable);
}

// _GetString
const char*
RenderSettingsWindow::_GetString(uint32 index,
								 const char* defaultString) const
{
#if LANGUAGE_MANAGER
	return LanguageManager::CreateDefault()->GetString(index,
													   defaultString);
#else
	return defaultString;
#endif
}

