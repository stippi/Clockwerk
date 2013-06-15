/*
 * Copyright 2004-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

// CommonPropertyIDs.cpp

#include "CommonPropertyIDs.h"
//#include "LanguageManager.h"

#include "Property.h"

// name_for_id
const char*
name_for_id(int32 id)
{
//	LanguageManager* m = LanguageManager::Default();
	const char* name = NULL;
	switch (id) {
		case PROPERTY_NAME:
//			name = m->GetString(NAME, "Name");
			name = "Name";
			break;
		case PROPERTY_DESCRIPTION:
			name = "Description";
			break;
		case PROPERTY_KEYWORDS:
			name = "Keywords";
			break;
		case PROPERTY_TYPE:
			name = "Type";
			break;
		case PROPERTY_PATH:
			name = "Path";
			break;
		case PROPERTY_TEMPLATE_NAME:
			name = "Template Name";
			break;
		case PROPERTY_LOG_PLAYBACK:
			name = "Log Playback";
			break;

		case PROPERTY_TYPE_MARKER:
			name = "Special Type";
			break;
		case PROPERTY_SEQUENCE_INDEX:
			name = "Sequence #";
			break;
		case PROPERTY_ITEM_DURATION:
			name = "Item Duration";
			break;
		case PROPERTY_COLLECTOR_MODE:
			name = "Collecting Mode";
			break;
		case PROPERTY_BACKGROUND_SOUND_ID:
			name = "BG Sound ID";
			break;
		case PROPERTY_BACKGROUND_SOUND_VOLUME:
			name = "BG Sound Vol";
			break;

		case PROPERTY_AUTHOR:
			name = "Author";
			break;
		case PROPERTY_COMPANY:
			name = "Company";
			break;
		case PROPERTY_CLIENT:
			name = "Client";
			break;
		case PROPERTY_SCOPE:
			name = "Scope";
			break;

		case PROPERTY_UNIT_ID:
			name = "Unit ID";
			break;

		case PROPERTY_SCHEDULE_TYPE:
			name = "Schedule Type";
			break;
		case PROPERTY_WEEK_DAYS:
			name = "Week Days";
			break;
		case PROPERTY_DATE:
			name = "Specific Date";
			break;

		case PROPERTY_START_DATE:
			name = "Start Date";
			break;
		case PROPERTY_VALID_DAYS:
			name = "Valid Days";
			break;

		case PROPERTY_START_TIME:
//			name = m->GetString(START_TIME, "Starttime");
			name = "Starttime";
			break;
		case PROPERTY_REPEAT_COUNT:
			name = "Repeats";
			break;
		case PROPERTY_AUTO_START_TIME:
			name = "Snap to Previous";
			break;

		case PROPERTY_PLAYLIST_SCHEDULEABLE:
			name = "Allowed in Schedules";
			break;

		case PROPERTY_ID:
			name = "ID";
			break;
		case PROPERTY_STATUS:
			name = "Status";
			break;
		case PROPERTY_SYNC_STATUS:
			name = "Sync Status";
			break;
		case PROPERTY_VERSION:
			name = "Version";
			break;
		case PROPERTY_MIME_TYPE:
			name = "MIME Type";
			break;

		case PROPERTY_DURATION:
			name = "Duration";
			break;
		case PROPERTY_DURATION_INFO:
			name = "Duration Info";
			break;

		case PROPERTY_WIDTH:
			name = "Width";
			break;
		case PROPERTY_HEIGHT:
			name = "Height";
			break;

		case PROPERTY_TRANSITION_DURATION:
			name = "Trans. Duration";
			break;
		case PROPERTY_TRANSITION_MODE:
			name = "Trans. Mode";
			break;

		case PROPERTY_OPACITY:
//			name = m->GetString(ALPHA, "Opacity");
			name = "Opacity";
			break;

		case PROPERTY_MAX_AUTO_GAIN:
			name = "Max Auto Gain";
			break;

		case PROPERTY_PIVOT_X:
			name = "Pivot X";
			break;
		case PROPERTY_PIVOT_Y:
			name = "Pivot Y";
			break;
		case PROPERTY_TRANSLATION_X:
			name = "Translation X";
			break;
		case PROPERTY_TRANSLATION_Y:
			name = "Translation Y";
			break;
		case PROPERTY_ROTATION:
			name = "Rotation";
			break;
		case PROPERTY_SCALE_X:
			name = "Scale X";
			break;
		case PROPERTY_SCALE_Y:
			name = "Scale Y";
			break;

		case PROPERTY_TEXT:
			name = "Text";
			break;
		case PROPERTY_FONT:
			name = "Font";
			break;
		case PROPERTY_FONT_SIZE:
			name = "Font Size";
			break;

		case PROPERTY_COLOR:
			name = "Color";
			break;
		case PROPERTY_OUTLINE_COLOR:
			name = "Outline Color";
			break;
		case PROPERTY_USE_OUTLINE:
			name = "Use Outline";
			break;

		case PROPERTY_BACKGROUND_COLOR:
			name = "BG Color";
			break;

		case PROPERTY_HORIZONTAL_ALIGNMENT:
			name = "H Alignment";
			break;
		case PROPERTY_VERTICAL_ALIGNMENT:
			name = "V Alignment";
			break;

		case PROPERTY_SCROLLING_SPEED:
			name = "Scrolling Speed (px/s)";
			break;
		case PROPERTY_SCROLLING_RESET_TIMEOUT:
			name = "Pos. Reset Timeout (s)";
			break;

		case PROPERTY_PARAGRAPH_INSET:
			name = "P. Inset";
			break;
		case PROPERTY_PARAGRAPH_SPACING:
			name = "P. Spacing";
			break;
		case PROPERTY_LINE_SPACING:
			name = "Line Spacing";
			break;
		case PROPERTY_GLYPH_SPACING:
			name = "Glyph Spacing";
			break;
		case PROPERTY_FONT_HINTING:
			name = "Font Hinting";
			break;
		case PROPERTY_BLOCK_WIDTH:
			name = "Width";
			break;

		case PROPERTY_FADE_MODE:
			name = "Fade Mode";
			break;

		case PROPERTY_TABLE_COLUMN_COUNT:
			name = "Columns";
			break;
		case PROPERTY_TABLE_COLUMN_WIDTH:
			name = "Default Width";
			break;
		case PROPERTY_TABLE_COLUMN_SPACING:
			name = "Spacing";
			break;
		case PROPERTY_TABLE_ROW_COUNT:
			name = "Rows";
			break;
		case PROPERTY_TABLE_ROW_HEIGHT:
			name = "Default Height";
			break;
		case PROPERTY_TABLE_ROW_SPACING:
			name = "Spacing";
			break;
		case PROPERTY_TABLE_ROUND_CORNER_RADIUS:
			name = "Corner Radius";
			break;

		case PROPERTY_TABLE_FADE_IN_MODE:
			name = "Fade In Mode";
			break;
		case PROPERTY_TABLE_FADE_IN_FRAMES:
			name = "Fade In Frames";
			break;
		case PROPERTY_TABLE_FADE_OUT_MODE:
			name = "Fade Out Mode";
			break;
		case PROPERTY_TABLE_FADE_OUT_FRAMES:
			name = "Fade Out Frames";
			break;

		case PROPERTY_CELL_COLUMN:
			name = "Column";
			break;
		case PROPERTY_CELL_ROW:
			name = "Row";
			break;

		case PROPERTY_CLOCK_ANALOG_FACE:
			name = "Analog";
			break;

		case PROPERTY_TIME_FORMAT:
			name = "Time Format";
			break;
		case PROPERTY_DATE_FORMAT:
			name = "Date Format";
			break;
		case PROPERTY_DATE_OFFSET:
			name = "Date Offset";
			break;
		case PROPERTY_TIMER_DIRECTION:
			name = "Direction";
			break;

		case PROPERTY_WEATHER_TIME:
			name = "Time";
			break;
		case PROPERTY_WEATHER_DISPLAY_LAYOUT:
			name = "Icon Layout";
			break;
		case PROPERTY_METAR_LOCATION:
			name = "Location";
			break;

		case PROPERTY_REFERENCED_PLAYLIST:
			name = "Referenced PL";
			break;
		case PROPERTY_REFERENCED_DISPLAY_SETTINGS:
			name = "Referenced Display Settings";
				// B_NAME_TOO_LONG...
			break;

		case PROPERTY_SERVER_IP:
			name = "Server Address 1";
			break;
		case PROPERTY_SECONDARY_SERVER_IP:
			name = "Server Address 2";
			break;
		case PROPERTY_ERASE_DATA_FOLDER:
			name = "Erase Objects";
			break;
		case PROPERTY_ERASE_OLD_REVISIONS:
			name = "Erase Old Revisions";
			break;
		case PROPERTY_LOG_UPLOAD_TIME:
			name = "Log Upload Time";
			break;
		case PROPERTY_CLIENT_SETTINGS_APPLIED:
			name = "Settings Applied";
			break;

		case PROPERTY_CLIP_ID:
			name = "Referenced Clip";
			break;

		case PROPERTY_PASSWORD:
			name = "Password";
			break;

		case PROPERTY_REVISION_INSTALLED:
			name = "Installed";
			break;

		case PROPERTY_INPUT_SOURCE:
			name = "Input Source";
			break;
		case PROPERTY_DISPLAY_WIDTH:
			name = "Display Width";
			break;
		case PROPERTY_DISPLAY_HEIGHT:
			name = "Display Height";
			break;
		case PROPERTY_DISPLAY_FREQUENCY:
			name = "Display Frequency";
			break;

		case PROPERTY_COMMAND_PATH:
			name = "Command Path";
			break;

//		case PROPERTY_MIN_OPACITY:
//			name = m->GetString(MIN_OPACITY, "min Op.");
//			break;
//		case PROPERTY_BLENDING_MODE:
//			name = m->GetString(MODE, "Mode");
//			break;
//		case PROPERTY_GRADIENT:
//			name = m->GetString(GRADIENT, "Gradient");
//			break;
//		case PROPERTY_RADIUS:
//			name = m->GetString(RADIUS, "Radius");
//			break;
//		case PROPERTY_MIN_RADIUS:
//			name = m->GetString(MIN_RADIUS, "min Radius");
//			break;
//		case PROPERTY_HARDNESS:
//			name = m->GetString(HARDNESS, "Hardness");
//			break;
//		case PROPERTY_MIN_HARDNESS:
//			name = m->GetString(MIN_HARDNESS, "min Hardn.");
//			break;
//		case PROPERTY_NO_ANTIALIASING:
//			name = m->GetString(SOLID, "Solid");
//			break;
//		case PROPERTY_SPACING:
//			name = m->GetString(SPACING, "Spacing");
//			break;
//		case PROPERTY_PRESSURE_CONTROLS_OPACITY:
//			name = m->GetString(DYN_OPACITY, "dyn. Op.");
//			break;
//		case PROPERTY_PRESSURE_CONTROLS_RADIUS:
//			name = m->GetString(DYN_RADIUS, "dyn. Radius");
//			break;
//		case PROPERTY_PRESSURE_CONTROLS_HARDNESS:
//			name = m->GetString(DYN_HARDNESS, "dyn. Hardn.");
//			break;
//		case PROPERTY_PRESSURE_CONTROLS_SPACING:
//			name = m->GetString(DYN_SPACING, "dyn. Spac.");
//			break;
//		case PROPERTY_TILT:
//			name = m->GetString(TILT, "Tilt");
//			break;
//		case PROPERTY_BLUR_RADIUS:
//			name = m->GetString(BLUR_RADIUS, "Blur Radius");
//			break;
//		case PROPERTY_TEXT:
//			name = m->GetString(TEXT_INPUT, "Text");
//			break;
//		case PROPERTY_FONT:
//			name = m->GetString(FONT, "Font");
//			break;
//		case PROPERTY_FONT_SIZE:
//			name = m->GetString(SIZE, "Size");
//			break;
//		case PROPERTY_FONT_X_SCALE:
//			name = m->GetString(SPACING, "Spacing");
//			break;
//		case PROPERTY_FONT_LINE_SCALE:
//			name = m->GetString(TEXT_LINE_SPACING, "Line Spacing");
//			break;
//		case PROPERTY_FONT_HINTING:
//			name = m->GetString(HINTING, "Hinting");
//			break;
//		case PROPERTY_FONT_KERNING:
//			name = m->GetString(KERNING, "Kerning");
//			break;
//		case PROPERTY_ALIGNMENT:
//			name = m->GetString(ALIGNMENT, "Alignment");
//			break;
//		case PROPERTY_WIDTH:
//			name = m->GetString(WIDTH, "Width");
//			break;
//		case PROPERTY_PARAGRAPH_INSET:
//			name = m->GetString(PARAGRAPH_INSET, "Inset");
//			break;
//		case PROPERTY_PARAGRAPH_SPACING:
//			name = m->GetString(PARAGRAPH_SPACING, "P. Spacing");
//			break;
//		case PROPERTY_PATH:
//			name = m->GetString(PATH, "Path");
//			break;
//		case PROPERTY_OUTLINE:
//			name = m->GetString(OUTLINE, "Outline");
//			break;
//		case PROPERTY_OUTLINE_WIDTH:
//			name = m->GetString(WIDTH, "Width");
//			break;
//		case PROPERTY_CLOSED:
//			name = m->GetString(CLOSED, "Closed");
//			break;
//		case PROPERTY_CAP_MODE:
//			name = m->GetString(CAP_MODE, "Caps");
//			break;
//		case PROPERTY_JOIN_MODE:
//			name = m->GetString(JOIN_MODE, "Joints");
//			break;
//		case PROPERTY_FILLING_RULE:
//			name = m->GetString(FILLING_RULE, "Filling Rule");
//			break;
//		case PROPERTY_X_OFFSET:
//			name = m->GetString(X_OFFSET, "X Offset");
//			break;
//		case PROPERTY_Y_OFFSET:
//			name = m->GetString(Y_OFFSET, "Y Offset");
//			break;
//		case PROPERTY_BRIGHTNESS:
//			name = m->GetString(BRIGHTNESS, "Brightness");
//			break;
//		case PROPERTY_CONTRAST:
//			name = m->GetString(CONTRAST, "Contrast");
//			break;
//		case PROPERTY_SATURATION:
//			name = m->GetString(SATURATION, "Saturation");
//			break;
//		case PROPERTY_ANGLE:
//			name = m->GetString(ANGLE, "Angle");
//			break;
//		case PROPERTY_BLUR_ALPHA:
//			name = m->GetString(ALPHA_CHANNEL, "Alpha channel");
//			break;
//		case PROPERTY_HALFTONE_MODE:
//			name = m->GetString(MODE, "Modus");
//			break;
//		case PROPERTY_STRENGTH:
//			name = m->GetString(STRENGTH, "Strength");
//			break;
//		case PROPERTY_LUMINANCE_ONLY:
//			name = m->GetString(LUMINANCE_ONLY, "Luminance only");
//			break;
//
//		case PROPERTY_WARPSHARP_LAMBDA:
//			name = m->GetString(LAMBDA, "Lambda");
//			break;
//		case PROPERTY_WARPSHARP_MU:
//			name = m->GetString(MU, "Mu");
//			break;
//		case PROPERTY_WARPSHARP_NON_MAX_SUPR:
//			name = m->GetString(NON_MAXIMAL_SUPPRESSION, "NMS");
//			break;
//
//		case PROPERTY_BITMAP_INTERPOLATION:
//			name = m->GetString(INTERPOLATION, "Interpolation");
//			break;
//
		default:
//			name = m->GetString(UNKOWN_PROPERTY, "<unkown property>");
			name = "<unkown property>";
			break;
	}
	return name;
}

// is_default_value
bool
is_default_value(const Property* p)
{
	// only some property types/ids are known, for all
	// others, it is assumed the value is non-default
	const FloatProperty* fp = dynamic_cast<const FloatProperty*>(p);
	if (!fp)
		return false;

	switch (fp->Identifier()) {
		case PROPERTY_PIVOT_X:
		case PROPERTY_PIVOT_Y:
		case PROPERTY_TRANSLATION_X:
		case PROPERTY_TRANSLATION_Y:
		case PROPERTY_ROTATION:
			if (fp->Value() == 0.0)
				return true;
			break;
		case PROPERTY_OPACITY:
		case PROPERTY_SCALE_X:
		case PROPERTY_SCALE_Y:
		case PROPERTY_MAX_AUTO_GAIN:
			if (fp->Value() == 1.0)
				return true;
			break;
	}

	return false;
}
