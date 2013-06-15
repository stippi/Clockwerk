/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COMMON_CONSTANTS_H
#define COMMON_CONSTANTS_H

#include <SupportDefs.h>

extern const char* kClockwerkMimeSig;
extern const char* kPlayerMimeSig;
extern const char* kConfiguratorMimeSig;
extern const char* kControllerMimeSig;
extern const char* kNewsFlowMimeSig;
extern const char* kWatchDogMimeSig;

extern const char* kPlayerName;
extern const char* kControllerName;

extern const char* kClockwerkPath;
extern const char* kClockwerkInstallationsPath;
extern const char* kObjectLibraryPath;
extern const char* kObjectFilePath;

extern const char* kAttrPrefix;
extern const char* kTypeAttr;
extern const char* kStatusAttr;
extern const char* kSHA256Attr;

extern const char* kNativeDocumentMimeType;

extern const int64 kWholeDayDuration;

extern const char* kUnsetClientID;

// settings paths
extern const char* kM3BaseSettingsPath;
extern const char* kM3BaseSettingsName;

extern const char* kClientIDSettingsPath;
extern const char* kPlayerLogSettingsPath;		// new-style logging
extern const char* kPlayerLoggingSettingsPath;
extern const char* kServerListSettingsPath;
extern const char* kClockwerkConfiguredIndicatorPath;

extern const char* kControllerSettingsPath;
extern const char* kControllerLogSettingsPath;
extern const char* kClockwerkSettingsPath;
extern const char* kClockwerkLogSettingsPath;
extern const char* kNewsFlowSettingsPath;
extern const char* kNewsFlowLogSettingsPath;
extern const char* kUpdaterLogSettingsPath;
extern const char* kWatchdogSettingsPath;
extern const char* kWatchdogLogSettingsPath;

extern const char* kOldClockwerkConfiguredIndicatorPath;
extern const char* kOldClientIDSettingsPath;
extern const char* kOldDefaultServerSettingsPath;


#define kDefaultFontSize 20.0
	// for TextClips and such
#define kDefaultScrollingSpeed 42.0
	// for ScrollingTextClips

#define CLOCKWERK_STAND_ALONE 1

#endif // COMMON_CONSTANTS_H
