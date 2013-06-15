/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PropertyObjectFactory.h"

#include <new>
#include <string.h>

#include "common_constants.h"
#include "ui_defines.h"

#include "ColorProperty.h"
#include "CommonPropertyIDs.h"
#include "DurationProperty.h"
#include "FontProperty.h"
#include "Int64Property.h"
#include "OptionProperty.h"
#include "Property.h"
#include "PropertyObject.h"
#include "ServerObjectManager.h"
#include "support.h"
#include "WeekDaysProperty.h"

using std::nothrow;

status_t
PropertyObjectFactory::CreatePropertyObject(const char* type,
	PropertyObject** _object)
{
	PropertyObject* object = new (nothrow) PropertyObject();
	if (!object)
		return B_NO_MEMORY;

	status_t ret = InitPropertyObject(type, object);
	if (ret < B_OK) {
		delete object;
	} else {
		*_object = object;
	}
	return ret;
}

status_t
PropertyObjectFactory::InitPropertyObject(const char* type,
	PropertyObject* object)
{
	if (!type)
		return B_BAD_VALUE;

	BString typeString(type);

	const char* mimeTypeString = NULL;

	if (string_ends_with(typeString, "Clip")) {

		// TODO: maybe there should be a global list of known clients,
		// but this will do for the time being
		object->AddProperty(new StringProperty(PROPERTY_CLIENT, "<unset>"));
		// TODO: maybe there should be a global list of known scopes
		// (locations), but this will do for the time being
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
		// could be used later for queries
		object->AddProperty(new StringProperty(PROPERTY_KEYWORDS, ""));

		// template name, if given, this clip is marked as a template
		object->AddProperty(new StringProperty(PROPERTY_TEMPLATE_NAME, ""));

		// option if logging of playback is required
		object->AddProperty(new BoolProperty(PROPERTY_LOG_PLAYBACK, false));

		if (typeString == "ColorClip") {
			ColorProperty* color = new ColorProperty(PROPERTY_COLOR);
			object->AddProperty(color);
		} else if (typeString == "ClockClip") {
			BoolProperty* analog = new BoolProperty(PROPERTY_CLOCK_ANALOG_FACE,
													false);
			OptionProperty* timeFormat
				= new OptionProperty(PROPERTY_TIME_FORMAT);

			timeFormat->AddOption(TIME_NONE, "No Time");
			timeFormat->AddOption(TIME_HH_MM_SS, "HH:MM:SS");
			timeFormat->AddOption(TIME_HH_MM, "HH:MM");
			timeFormat->SetCurrentOptionID(TIME_HH_MM_SS);
			object->AddProperty(timeFormat);

			OptionProperty* dateFormat
				= new OptionProperty(PROPERTY_DATE_FORMAT);

			dateFormat->AddOption(DATE_NONE, "No Date");
			dateFormat->AddOption(DATE_DD_MM_YYYY, "DD.MM.YYYY");
			dateFormat->AddOption(DATE_DD_MM_YY, "DD.MM.YY");
			dateFormat->AddOption(DATE_DD_MM, "DD.MM.");
			dateFormat->AddOption(DATE_WEEKDAY_DD_MM_YYYY, "Weekday, DD.MM.YYYY");
			dateFormat->AddOption(DATE_WEEKDAY_DD_MONTH_YYYY, "Weekday, DD. Month YYYY");
			dateFormat->AddOption(DATE_DD, "DD");
			dateFormat->AddOption(DATE_WEEKDAY, "Weekday");
			dateFormat->AddOption(DATE_MONTH, "Month");
			dateFormat->AddOption(DATE_MONTH_ALL_UPPERS, "MONTH");

			dateFormat->SetCurrentOptionID(DATE_DD_MM_YYYY);
			object->AddProperty(dateFormat);

			object->AddProperty(new IntProperty(PROPERTY_DATE_OFFSET, 0));

			object->AddProperty(analog);
			ColorProperty* color = new ColorProperty(PROPERTY_COLOR, kWhite);
			object->AddProperty(color);
			FontProperty* font = new FontProperty(PROPERTY_FONT, *be_bold_font);
			object->AddProperty(font);
			object->AddProperty(new FloatProperty(PROPERTY_FONT_SIZE,
				kDefaultFontSize, 1.0, 500.0));
		} else if (typeString == "ExecuteClip") {
			object->AddProperty(new StringProperty(
				PROPERTY_COMMAND_PATH, "<path to command/script>"));
			
		} else if (typeString == "TimerClip") {
			OptionProperty* timeFormat
				= new OptionProperty(PROPERTY_TIME_FORMAT);

			timeFormat->AddOption(TIME_HH_MM_SS, "HH:MM:SS");
			timeFormat->AddOption(TIME_MM_SS, "MM:SS");
			timeFormat->AddOption(TIME_SS, "SS");
			timeFormat->AddOption(TIME_HH_MM_SS_MMM, "HH:MM:SS.MMM");
			timeFormat->AddOption(TIME_HH_MM_SS_FF, "HH:MM:SS.FF");
			timeFormat->AddOption(TIME_MM_SS_MMM, "MM:SS.MMM");
			timeFormat->AddOption(TIME_SS_MMM, "SS.MMM");

			timeFormat->SetCurrentOptionID(TIME_SS);
			object->AddProperty(timeFormat);

			OptionProperty* timerDirection
				= new OptionProperty(PROPERTY_TIMER_DIRECTION);

			timerDirection->AddOption(TIMER_DIRECTION_FORWARD, "Forward");
			timerDirection->AddOption(TIMER_DIRECTION_BACKWARD, "Backward");

			timerDirection->SetCurrentOptionID(TIMER_DIRECTION_BACKWARD);
			object->AddProperty(timerDirection);

			ColorProperty* color = new ColorProperty(PROPERTY_COLOR, kWhite);
			object->AddProperty(color);
			FontProperty* font = new FontProperty(PROPERTY_FONT, *be_bold_font);
			object->AddProperty(font);
			object->AddProperty(new FloatProperty(PROPERTY_FONT_SIZE,
				kDefaultFontSize, 1.0, 500.0));
		} else if (typeString == "WeatherClip") {
			OptionProperty* location
				= new OptionProperty(PROPERTY_METAR_LOCATION);


			location->AddOption(METAR_LOCATION_AACHEN, "Aachen");
			location->AddOption(METAR_LOCATION_ALTENSTADT, "Altenstadt");
			location->AddOption(METAR_LOCATION_AUGSBURG, "Augsburg");
			location->AddOption(METAR_LOCATION_BADEN_WUERTTEMBERG, "Baden Württemberg");
			location->AddOption(METAR_LOCATION_BAYREUTH, "Bayreuth");
			location->AddOption(METAR_LOCATION_BERLIN, "Berlin");
			location->AddOption(METAR_LOCATION_BRAUNSCHWEIG, "Braunschweig");
			location->AddOption(METAR_LOCATION_BREMEN, "Bremen");
			location->AddOption(METAR_LOCATION_CHEMNITZ, "Chemnitz");
			location->AddOption(METAR_LOCATION_COBURG, "Coburg");
			location->AddOption(METAR_LOCATION_COTTBUS, "Cottbus");
			location->AddOption(METAR_LOCATION_DORTMUND, "Dortmund");
			location->AddOption(METAR_LOCATION_DRESDEN, "Dresden");
			location->AddOption(METAR_LOCATION_DUESSELDORF, "Düsseldorf");
			location->AddOption(METAR_LOCATION_ERDING, "Erding");
			location->AddOption(METAR_LOCATION_ESSEN, "Essen");
			location->AddOption(METAR_LOCATION_FRANKFURT_MAIN, "Frankfurt/Main");
			location->AddOption(METAR_LOCATION_FRIEDRICHSHAFEN, "Friedrichshafen");
			location->AddOption(METAR_LOCATION_GIESSEN, "Gießen");
			location->AddOption(METAR_LOCATION_HAMBURG, "Hamburg");
			location->AddOption(METAR_LOCATION_HANAU, "Hanau");
			location->AddOption(METAR_LOCATION_HANNOVER, "Hannover");
			location->AddOption(METAR_LOCATION_HEIDELBERG, "Heidelberg");
			location->AddOption(METAR_LOCATION_INGOLSTADT, "Ingolstadt");
			location->AddOption(METAR_LOCATION_KARLSRUHE, "Karlsruhe");
			location->AddOption(METAR_LOCATION_KASSEL, "Kassel");
			location->AddOption(METAR_LOCATION_KIEL, "Kiel");
			location->AddOption(METAR_LOCATION_KOELN, "Koeln");
			location->AddOption(METAR_LOCATION_KONSTANZ, "Konstanz");
			location->AddOption(METAR_LOCATION_LANDSBERG, "Landsberg");
			location->AddOption(METAR_LOCATION_LEIPZIG, "Leipzig");
			location->AddOption(METAR_LOCATION_MEMMINGEN, "Memmingen");
			location->AddOption(METAR_LOCATION_MUENCHEN, "München");
			location->AddOption(METAR_LOCATION_MUENSTER, "Münster");
			location->AddOption(METAR_LOCATION_NUERNBERG, "Nürnberg");
			location->AddOption(METAR_LOCATION_PADERBORN, "Paderborn");
			location->AddOption(METAR_LOCATION_REGENSBURG, "Regensburg");
			location->AddOption(METAR_LOCATION_ROSTOCK, "Rostock");
			location->AddOption(METAR_LOCATION_SAARBRUECKEN, "Saarbrücken");
			location->AddOption(METAR_LOCATION_STUTTGART, "Stuttgart");
			location->AddOption(METAR_LOCATION_WEIMAR, "Weimar");
			location->AddOption(METAR_LOCATION_WIESBADEN, "Wiesbaden");
			location->AddOption(METAR_LOCATION_WUERZBURG, "Würzburg");

			location->SetCurrentOptionID(METAR_LOCATION_ROSTOCK);
			object->AddProperty(location);

			OptionProperty* displayMode
				= new OptionProperty(PROPERTY_WEATHER_DISPLAY_LAYOUT);
			displayMode->AddOption(WEATHER_DISPLAY_LAYOUT_ICON_LEFT,
				"Icon Left");
			displayMode->AddOption(WEATHER_DISPLAY_LAYOUT_ICON_RIGHT,
				"Icon Right");
			displayMode->AddOption(WEATHER_DISPLAY_LAYOUT_NO_ICON,
				"No Icon");
			displayMode->SetCurrentOptionID(WEATHER_DISPLAY_LAYOUT_ICON_LEFT);
			object->AddProperty(displayMode);

			OptionProperty* weatherTime
				= new OptionProperty(PROPERTY_WEATHER_TIME);

			weatherTime->AddOption(WEATHER_FORMAT_NOW, "Now");
			weatherTime->AddOption(WEATHER_FORMAT_TODAY_MORNING, "Today Morning");
			weatherTime->AddOption(WEATHER_FORMAT_TODAY_DAY, "Today Day");
			weatherTime->AddOption(WEATHER_FORMAT_TODAY_EVENING, "Today Evening");
			weatherTime->AddOption(WEATHER_FORMAT_TODAY_NIGHT, "Today Night");
			weatherTime->AddOption(WEATHER_FORMAT_NEXT_DAY, "Next Day");
			weatherTime->AddOption(WEATHER_FORMAT_NEXT_NIGHT, "Next Night");
			weatherTime->AddOption(WEATHER_FORMAT_NEXT_NEXT_DAY, "Next Next Day");
			weatherTime->AddOption(WEATHER_FORMAT_NEXT_NEXT_NIGHT, "Next Next Night");

			weatherTime->SetCurrentOptionID(WEATHER_FORMAT_NOW);
			object->AddProperty(weatherTime);

			ColorProperty* color = new ColorProperty(PROPERTY_COLOR, kWhite);
			object->AddProperty(color);
			FontProperty* font = new FontProperty(PROPERTY_FONT, *be_bold_font);
			object->AddProperty(font);
			object->AddProperty(new FloatProperty(PROPERTY_FONT_SIZE,
				kDefaultFontSize, 1.0, 500.0));
// TODO: at the moment, BitmapClip and MediaClip have a type of "FileBasedClip"
// this should be changed, no?
//		} else if (typeString == "BitmapClip" || typeString == "MediaClip") {
		} else if (typeString == "FileBasedClip") {
			// "native" width and height, just so that the clip can
			// display it in it's properties
			IntProperty* width = new IntProperty(PROPERTY_WIDTH, 0, 0, 8192);
			width->SetEditable(false);
			object->AddProperty(width);
			IntProperty* height = new IntProperty(PROPERTY_HEIGHT, 0, 0, 8192);
			height->SetEditable(false);
			object->AddProperty(height);

			// fade mode
			OptionProperty* fadeMode
				= new OptionProperty(PROPERTY_FADE_MODE);
			fadeMode->AddOption(FADE_MODE_ALPHA, "Alpha");
			fadeMode->AddOption(FADE_MODE_WIPE_LEFT_RIGHT, "Wipe left-right");
			fadeMode->AddOption(FADE_MODE_WIPE_RIGHT_LEFT, "Wipe right-left");
			fadeMode->AddOption(FADE_MODE_WIPE_TOP_BOTTOM, "Wipe top-bottom");
			fadeMode->AddOption(FADE_MODE_WIPE_BOTTOM_TOP, "Wipe bottom-top");
			fadeMode->SetCurrentOptionID(FADE_MODE_ALPHA);
			object->AddProperty(fadeMode);

			// these have a mime type, but it isn't known yet
			mimeTypeString = "";

		} else if (typeString == "ScrollingTextClip") {
			StringProperty* text = new StringProperty(
				PROPERTY_TEXT, "Enter a text in the Property list view!");
			object->AddProperty(text);
			ColorProperty* color = new ColorProperty(PROPERTY_COLOR, kWhite);
			object->AddProperty(color);

			object->AddProperty(new BoolProperty(PROPERTY_USE_OUTLINE, false));

			color = new ColorProperty(PROPERTY_OUTLINE_COLOR, kBlack);
			object->AddProperty(color);

			FontProperty* font = new FontProperty(PROPERTY_FONT, *be_bold_font);
			object->AddProperty(font);
			object->AddProperty(new FloatProperty(PROPERTY_FONT_SIZE,
				kDefaultFontSize, 1.0, 500.0));

			object->AddProperty(new FloatProperty(PROPERTY_SCROLLING_SPEED,
				kDefaultScrollingSpeed, -500.0, 500.0));
			object->AddProperty(
				new IntProperty(PROPERTY_SCROLLING_RESET_TIMEOUT, 25, 0, 3600));
			object->AddProperty(new FloatProperty(PROPERTY_BLOCK_WIDTH,
				684.0, 0.0, 10000.0));
		} else if (typeString == "TextClip") {
			StringProperty* text = new StringProperty(
				PROPERTY_TEXT, "Enter a text in the Property list view!");
			object->AddProperty(text);
			ColorProperty* color = new ColorProperty(PROPERTY_COLOR, kWhite);
			object->AddProperty(color);
			FontProperty* font = new FontProperty(PROPERTY_FONT, *be_bold_font);
			object->AddProperty(font);
			object->AddProperty(new FloatProperty(PROPERTY_FONT_SIZE,
				kDefaultFontSize, 1.0, 500.0));
			object->AddProperty(new FloatProperty(PROPERTY_PARAGRAPH_INSET,
				0.0, -10000.0, 10000.0));
			object->AddProperty(new FloatProperty(PROPERTY_PARAGRAPH_SPACING,
				 1.0, 0.0, 4.0));
			object->AddProperty(new FloatProperty(PROPERTY_LINE_SPACING,
				1.0, 0.0, 4.0));
			object->AddProperty(new FloatProperty(PROPERTY_GLYPH_SPACING,
				1.0, 0.0, 2.0));
			object->AddProperty(new BoolProperty(PROPERTY_FONT_HINTING,
				true));
			object->AddProperty(new FloatProperty(PROPERTY_BLOCK_WIDTH,
				300.0, 0.0, 10000.0));
			OptionProperty* horizontalAlignment
				= new OptionProperty(PROPERTY_HORIZONTAL_ALIGNMENT);

			horizontalAlignment->AddOption(ALIGN_BEGIN, "Left");
			horizontalAlignment->AddOption(ALIGN_CENTER, "Center");
			horizontalAlignment->AddOption(ALIGN_END, "Right");
			horizontalAlignment->AddOption(ALIGN_STRETCH, "Justify");
			horizontalAlignment->SetCurrentOptionID(ALIGN_BEGIN);
			object->AddProperty(horizontalAlignment);

		} else if (typeString == "TableClip") {
			// column count
			object->AddProperty(
				new IntProperty(PROPERTY_TABLE_COLUMN_COUNT,
					3, 1, 10000));
			// default column width
			object->AddProperty(
				new FloatProperty(PROPERTY_TABLE_COLUMN_WIDTH,
					75.0, 1.0, 10000.0));
			// column spacing
			object->AddProperty(
				new FloatProperty(PROPERTY_TABLE_COLUMN_SPACING,
					4.0, 0.0, 100.0));

			// row count
			object->AddProperty(
				new IntProperty(PROPERTY_TABLE_ROW_COUNT,
					5, 1, 10000));
			// default row height
			object->AddProperty(
				new FloatProperty(PROPERTY_TABLE_ROW_HEIGHT,
					25.0, 1.0, 10000.0));
			// row spacing
			object->AddProperty(
				new FloatProperty(PROPERTY_TABLE_ROW_SPACING,
					4.0, 0.0, 100.0));

			// round corner radius
			object->AddProperty(
				new FloatProperty(PROPERTY_TABLE_ROUND_CORNER_RADIUS,
					4.0, 0.0, 100.0));

			// default horizontal and vertical alignment
			OptionProperty* horizontalAlignment
				= new OptionProperty(PROPERTY_HORIZONTAL_ALIGNMENT);
			OptionProperty* verticalAlignment
				= new OptionProperty(PROPERTY_VERTICAL_ALIGNMENT);

			horizontalAlignment->AddOption(ALIGN_BEGIN, "Left");
			horizontalAlignment->AddOption(ALIGN_CENTER, "Center");
			horizontalAlignment->AddOption(ALIGN_END, "Right");
			horizontalAlignment->AddOption(ALIGN_STRETCH, "Justify");
			horizontalAlignment->SetCurrentOptionID(ALIGN_BEGIN);
			object->AddProperty(horizontalAlignment);
		
			verticalAlignment->AddOption(ALIGN_BEGIN, "Top");
			verticalAlignment->AddOption(ALIGN_CENTER, "Center");
			verticalAlignment->AddOption(ALIGN_END, "Bottom");
			verticalAlignment->AddOption(ALIGN_STRETCH, "Stretch");
			verticalAlignment->SetCurrentOptionID(ALIGN_CENTER);
			object->AddProperty(verticalAlignment);

			// default cell font
			FontProperty* font = new FontProperty(PROPERTY_FONT, *be_bold_font);
			object->AddProperty(font);
			object->AddProperty(new FloatProperty(PROPERTY_FONT_SIZE,
				kDefaultFontSize, 1.0, 500.0));

			// fade in mode
			OptionProperty* fadeInMode
				= new OptionProperty(PROPERTY_TABLE_FADE_IN_MODE);
			fadeInMode->AddOption(TABLE_FADE_IN_EXPAND, "Diagonal Fade");
			fadeInMode->AddOption(TABLE_FADE_IN_SLIDE, "Slide in");
			fadeInMode->AddOption(TABLE_FADE_IN_EXPAND_COLUMNS, "Expand Columns");
			fadeInMode->AddOption(TABLE_FADE_IN_EXPAND_ROWS, "Expand Rows");
			fadeInMode->SetCurrentOptionID(TABLE_FADE_IN_EXPAND);
			object->AddProperty(fadeInMode);
			// fade in frames
			object->AddProperty(
				new IntProperty(PROPERTY_TABLE_FADE_IN_FRAMES,
					10, 0, 10000));

			// fade out mode
			OptionProperty* fadeOutMode
				= new OptionProperty(PROPERTY_TABLE_FADE_OUT_MODE);
			fadeOutMode->AddOption(TABLE_FADE_OUT_COLLAPSE, "Diagonal Fade");
			fadeOutMode->AddOption(TABLE_FADE_OUT_SLIDE, "Slide out");
			fadeOutMode->AddOption(TABLE_FADE_COLLAPSE_COLUMNS, "Collapse Columns");
			fadeOutMode->AddOption(TABLE_FADE_COLLAPSE_ROWS, "Collapse Rows");
			fadeOutMode->SetCurrentOptionID(TABLE_FADE_OUT_COLLAPSE);
			object->AddProperty(fadeOutMode);
			// fade out frames
			object->AddProperty(
				new IntProperty(PROPERTY_TABLE_FADE_OUT_FRAMES,
					10, 0, 10000));
			// mime type
			mimeTypeString = "text/xml";
		}

	} else if (string_ends_with(typeString, "Playlist")) {
		// auto assign the "all" scope
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
		
		// TODO: maybe there should be a global list of known authors,
		// but this will do for the time being
		object->AddProperty(new StringProperty(PROPERTY_AUTHOR, "<unset>"));
		// TODO: maybe there should be a global list of known companies,
		// but this will do for the time being
		object->AddProperty(new StringProperty(PROPERTY_COMPANY, "<unset>"));
	
		object->AddProperty(new StringProperty(PROPERTY_DESCRIPTION, ""));
	
		// could be used later for queries
		object->AddProperty(new StringProperty(PROPERTY_KEYWORDS, ""));

		// option if logging of playback is required
		object->AddProperty(new BoolProperty(PROPERTY_LOG_PLAYBACK, false));

		// playlist status
		OptionProperty* status = new OptionProperty(PROPERTY_STATUS);
		status->AddOption(PLAYLIST_STATUS_DRAFT, "Draft");
		status->AddOption(PLAYLIST_STATUS_TESTING, "Testing");
		status->AddOption(PLAYLIST_STATUS_READY, "Ready");
		status->AddOption(PLAYLIST_STATUS_LIVE, "Live");
	
		status->SetCurrentOptionID(PLAYLIST_STATUS_DRAFT);
		object->AddProperty(status);

		object->AddProperty(new BoolProperty(PROPERTY_PLAYLIST_SCHEDULEABLE,
			true));

		// "native" width and height
		object->AddProperty(new IntProperty(PROPERTY_WIDTH, 684, 320, 2048));
		object->AddProperty(new IntProperty(PROPERTY_HEIGHT, 384, 240, 2048));

		if (typeString == "SlideShowPlaylist") {
			// the user specified total duration
			Int64Property* duration = new Int64Property(PROPERTY_DURATION,
														400);
			object->AddProperty(duration);

			// the user specified transition duration
			duration = new Int64Property(PROPERTY_TRANSITION_DURATION, 10);
			object->AddProperty(duration);
			// TODO: transition mode

		} else if (typeString == "StretchingPlaylist") {
			// the user specified total duration
			object->AddProperty(new Int64Property(PROPERTY_DURATION, 400));

		} else if (typeString == "CollectablePlaylist") {
			// the user specified total duration
			object->AddProperty(new Int64Property(PROPERTY_DURATION, 9 * 25));

			// type marker, to indicate special playlists
			// ("news", "sport", ...)
			object->AddProperty(new StringProperty(PROPERTY_TYPE_MARKER, ""));
			object->AddProperty(new StringProperty(PROPERTY_TEMPLATE_NAME, ""));

			// sequence index in multi part template playlists
			object->AddProperty(new IntProperty(PROPERTY_SEQUENCE_INDEX, 0, 0));

			// at which day is this playlist becoming valid
			object->AddProperty(new StringProperty(PROPERTY_START_DATE, ""));
			// for how many days is this playlist remaining valid
			object->AddProperty(new IntProperty(PROPERTY_VALID_DAYS, 1, 1));

		} else if (typeString == "CollectingPlaylist") {
			// the collector mode
			OptionProperty* mode = new OptionProperty(PROPERTY_COLLECTOR_MODE);
			mode->AddOption(COLLECTOR_MODE_SEQUENCE, "All in Sequence");
			mode->AddOption(COLLECTOR_MODE_RANDOM, "Select one Random");
		
			mode->SetCurrentOptionID(COLLECTOR_MODE_SEQUENCE);
			object->AddProperty(mode);

			// type marker, to indicate which special playlists should
			// be collected (see CollectablePlaylist)
			object->AddProperty(new StringProperty(PROPERTY_TYPE_MARKER, ""));

			// the id of the clip used as transitions between
			// collectable playlists
			object->AddProperty(new StringProperty(PROPERTY_CLIP_ID, ""));
			// the id of the clip used as background sound
			object->AddProperty(new StringProperty(
				PROPERTY_BACKGROUND_SOUND_ID, ""));
			// the volume of the clip used as background sound
			object->AddProperty(new FloatProperty(
				PROPERTY_BACKGROUND_SOUND_VOLUME, 1.0, 0.0, 2.0));
			
			object->AddProperty(new Int64Property(PROPERTY_ITEM_DURATION,
				9 * 25));

			// the user specified maximum duration
			object->AddProperty(new Int64Property(PROPERTY_DURATION,
				60 * 60 * 25));
		}

		// add non-editable duration property
		DurationProperty* duration = new DurationProperty(
			PROPERTY_DURATION_INFO);
		duration->SetEditable(false);
		object->AddProperty(duration);

		// mime type
		mimeTypeString = "text/xml";

	} else if (typeString == "Schedule") {
		// auto assign the "all" scope
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
		
		object->AddProperty(new StringProperty(PROPERTY_DESCRIPTION, ""));

		// could be used later for queries
		object->AddProperty(new StringProperty(PROPERTY_KEYWORDS, ""));
	
		// schedule status
		OptionProperty* status = new OptionProperty(PROPERTY_STATUS);
		status->AddOption(PLAYLIST_STATUS_DRAFT, "Draft");
		status->AddOption(PLAYLIST_STATUS_TESTING, "Testing");
		status->AddOption(PLAYLIST_STATUS_READY, "Ready");
		status->AddOption(PLAYLIST_STATUS_LIVE, "Live");
	
		status->SetCurrentOptionID(PLAYLIST_STATUS_DRAFT);
		object->AddProperty(status);

		// schedule type
		OptionProperty* type = new OptionProperty(PROPERTY_SCHEDULE_TYPE);
		type->AddOption(SCHEDULE_TYPE_WEEKLY, "Weekly");
		type->AddOption(SCHEDULE_TYPE_DATE, "Specific Date");
		type->SetCurrentOptionID(SCHEDULE_TYPE_WEEKLY);

		object->AddProperty(type);
		object->AddProperty(new WeekDaysProperty(PROPERTY_WEEK_DAYS));
		object->AddProperty(new StringProperty(PROPERTY_DATE, "Today"));

		// mime type
		mimeTypeString = "text/xml";
	} else if (typeString == "ClockwerkComponent") {
		// mime type
		mimeTypeString = "application/octet-stream";
		// no additional properties
	} else if (typeString == "ClockwerkRevision") {
		Property* p = new BoolProperty(PROPERTY_REVISION_INSTALLED, false);
		p->SetEditable(false);
		object->AddProperty(p);

		object->AddProperty(new StringProperty(PROPERTY_DESCRIPTION, ""));

		// mime type
		mimeTypeString = "text/xml";
	} else if (typeString == "ClientSettings") {
		// TODO: maybe there should be a global list of known scopes
		// (locations), but this will do for the time being
		// NOTE: the global list is on the server (network of scopes)
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
		
		// TODO: remove, once all servers are updated to new metadata-defs.xml!
		// (29.08.2007)
		// referenced playlist id
		object->AddProperty(new StringProperty(
			PROPERTY_REFERENCED_PLAYLIST, ""));

		// referenced display settings id
		object->AddProperty(new StringProperty(
			PROPERTY_REFERENCED_DISPLAY_SETTINGS, ""));
		// primary server IP
		object->AddProperty(new StringProperty(
			PROPERTY_SERVER_IP, "192.168.0.100"));
		// secondary server IP
		object->AddProperty(new StringProperty(
			PROPERTY_SECONDARY_SERVER_IP, "81.169.187.105"));
		// erase data folder
		object->AddProperty(new BoolProperty(
			PROPERTY_ERASE_DATA_FOLDER, false));
		// erase old revisions
		object->AddProperty(new BoolProperty(
			PROPERTY_ERASE_OLD_REVISIONS, false));
		// daily log upload time
		object->AddProperty(new StringProperty(
			PROPERTY_LOG_UPLOAD_TIME, "23:00:00"));

		// a flag for the client to track wether the settings have
		// already been applied
		Property* p = new BoolProperty(PROPERTY_CLIENT_SETTINGS_APPLIED,
			false);
		p->SetEditable(false);
		object->AddProperty(p);

	} else if (typeString == "PlaybackReport") {
		object->AddProperty(new StringProperty(PROPERTY_UNIT_ID, ""));
		// mime type
		mimeTypeString = "text/xml";
	} else if (typeString == "User") {
		// password
		object->AddProperty(new StringProperty(PROPERTY_PASSWORD, ""));
	} else if (typeString == "DisplaySettings") {
		// scope
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
		// width
		object->AddProperty(new IntProperty(PROPERTY_WIDTH, 640, 512, 1920));
		// height
		object->AddProperty(new IntProperty(PROPERTY_HEIGHT, 480, 320, 1200));

		// input source		
		OptionProperty* source = new OptionProperty(PROPERTY_INPUT_SOURCE);
		source->AddOption(DISPLAY_INPUT_SOURCE_VGA, "VGA");
		source->AddOption(DISPLAY_INPUT_SOURCE_DVI, "DVI");
		source->SetCurrentOptionID(DISPLAY_INPUT_SOURCE_VGA);
		object->AddProperty(source);

		// display width, height, and frequency
		object->AddProperty(new IntProperty(PROPERTY_DISPLAY_WIDTH, 1368, 512, 1920));
		object->AddProperty(new IntProperty(PROPERTY_DISPLAY_HEIGHT, 1368, 512, 1200));
		object->AddProperty(new FloatProperty(PROPERTY_DISPLAY_FREQUENCY,
			60.0f, 50.0f, 120.0f));
	} else if (typeString == "FileTransferTestDescription") {
		// scope
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
	} else if (typeString == "FileTransferTestFile") {
		// scope
		object->AddProperty(new StringProperty(PROPERTY_SCOPE, "all"));
	} else {
		// unknown object
	}

	// Add common properties

	// name
	object->AddProperty(new StringProperty(PROPERTY_NAME, ""));

	// server id
	StringProperty* id = new StringProperty(PROPERTY_ID,
		ServerObjectManager::NextID().String());
		// NOTE: this generates an id that might very well
		// be unused, because it gets overwritten with an
		// id restored from disk or whatever

	id->SetEditable(false);
	object->AddProperty(id);

	// version
	IntProperty* version = new IntProperty(PROPERTY_VERSION, 0L);
	version->SetEditable(false);
	object->AddProperty(version);

	// sync status
	OptionProperty* status = new OptionProperty(PROPERTY_SYNC_STATUS);
	status->AddOption(SYNC_STATUS_LOCAL, "Local");
	status->AddOption(SYNC_STATUS_PUBLISHED, "Published");
	status->AddOption(SYNC_STATUS_MODIFIED, "Modified");
	status->AddOption(SYNC_STATUS_LOCAL_REMOVED, "Locally Removed");
	status->AddOption(SYNC_STATUS_SERVER_REMOVED, "Removed");
	status->SetCurrentOptionID(SYNC_STATUS_LOCAL);
	status->SetEditable(false);
	object->AddProperty(status);

	// type
	StringProperty* t = new StringProperty(PROPERTY_TYPE, type);
	t->SetEditable(false);
	object->AddProperty(t);

	if (mimeTypeString != NULL) {
		// mime type
		object->AddProperty(new StringProperty(PROPERTY_MIME_TYPE,
			mimeTypeString));
	}

	return B_OK;
}



