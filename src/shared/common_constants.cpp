/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "common_constants.h"

const char* kClockwerkMimeSig	= "application/x-vnd.MindWork-Clockwerk";
const char* kPlayerMimeSig		= "application/x-vnd.MindWork-Clockwerk.Player";
const char* kConfiguratorMimeSig = "application/x-vnd.MindWork-Clockwerk.Configurator";
const char* kControllerMimeSig	= "application/x-vnd.MindWork-Clockwerk.Controller";
const char* kNewsFlowMimeSig	= "application/x-vnd.MindWork-Clockwerk.NewsFlow";
const char* kWatchDogMimeSig	= "application/x-vnd.MindWork-Clockwerk.WatchDog";

const char* kPlayerName			= "Clockwerk-Player";
const char* kControllerName		= "Clockwerk-Controller";

const char* kClockwerkPath		= "/boot/Clockwerk";
const char* kClockwerkInstallationsPath = "/boot";
const char* kObjectLibraryPath	= "/boot/Clockwerk/object_library";
const char* kObjectFilePath		= "/boot/home/clockwerk";

const char* kAttrPrefix			= "CLKW:";
const char* kTypeAttr			= "CLKW:type";
const char* kStatusAttr			= "CLKW:syst";
const char* kSHA256Attr			= "CLKW:s256";

const char* kNativeDocumentMimeType = "application/x-vnd.MindWork-Clockwerk.Object";

const int64 kWholeDayDuration	= (24 * 60 * 60) * 25;

const char* kUnsetClientID		= "unset_client_id";

const char* kM3BaseSettingsPath
	= "/boot/home/config/settings/mindwork-m3";
const char* kM3BaseSettingsName = "mindwork-m3";

const char* kClientIDSettingsPath
	= "/boot/home/config/settings/mindwork-m3/controller_client_id";
const char* kPlayerLogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/player-log.properties";
const char* kPlayerLoggingSettingsPath
	= "/boot/home/config/settings/mindwork-m3/player_log_path";
const char* kServerListSettingsPath
	= "/boot/home/config/settings/mindwork-m3/controller_known_server_list";
const char* kClockwerkConfiguredIndicatorPath
	= "/boot/home/config/settings/mindwork-m3/clockwerk_configured";

const char* kControllerSettingsPath
	= "/boot/home/config/settings/mindwork-m3/controller_settings";
const char* kControllerLogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/controller-log.properties";
const char* kClockwerkSettingsPath
	= "/boot/home/config/settings/mindwork-m3/clockwerk_settings";
const char* kClockwerkLogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/clockwerk-log.properties";
const char* kNewsFlowSettingsPath
	= "/boot/home/config/settings/mindwork-m3/newsflow_settings";
const char* kNewsFlowLogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/newsflow-log.properties";
const char* kUpdaterLogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/updater-log.properties";
const char* kWatchdogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/watchdog.properties";
const char* kWatchdogLogSettingsPath
	= "/boot/home/config/settings/mindwork-m3/watchdog-log.properties";


// backwards compatibility
const char* kOldClockwerkConfiguredIndicatorPath
	= "/boot/home/config/settings/clockwerk_configured";
const char* kOldClientIDSettingsPath
	= "/boot/home/config/settings/Clockwerk/controller-client-id";
const char* kOldDefaultServerSettingsPath
	= "/boot/home/config/settings/clockwerk_default_server";
