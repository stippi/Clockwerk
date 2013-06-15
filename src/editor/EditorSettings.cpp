/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "EditorSettings.h"

#include <Application.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <Roster.h>

#include "common.h"

//#include "XMLHelper.h"
//#include "XMLSupport.h"

// constructor
EditorSettings::EditorSettings()
	: fServer("www.mindwork.de"),
	  fMediaFolder("/boot/home/clockwerk/"),
	  fClientID("client0001"),
	  fRevision(0)
{
}

// destructor
EditorSettings::~EditorSettings()
{
}

// Init
status_t
EditorSettings::Init()
{
#if 0
	BEntry entry;

	// try to find the settings file in the app folder
	app_info info;
	status_t ret = be_app->GetAppInfo(&info);
	if (ret >= B_OK)
		ret = entry.SetTo(&info.ref);
	if (ret >= B_OK)
		ret = entry.GetParent(&entry);
	if (ret >= B_OK) {
		BPath path(&entry);

		// figure out revision while we're at it
		if (sscanf(path.Leaf(), "Clockwerk-%ld", &fRevision) != 1) {
			printf("EditorSettings::EditorSettings() - "
				   "unable to obtain current revision from folder name, "
				   "defaulting to '0'\n");
			fRevision = 0;
		}

		ret = path.Append("clockwerk_settings");
		if (ret >= B_OK)
			ret = entry.SetTo(path.Path());
		if (!entry.Exists() || ret < B_OK)
			ret = entry.SetTo(kClockwerkSettingsPath);
	}
	if (ret < B_OK) {
		printf("EditorSettings::EditorSettings() - "
			   "failed to find app folder: %s\n", strerror(ret));
		return ret;
	}

	if (!entry.Exists()) {
		ret = B_ENTRY_NOT_FOUND;
		printf("EditorSettings::EditorSettings() - "
			   "failed to find settings file: %s\n", strerror(ret));
		return ret;
	}

	BFile file(&entry, B_READ_ONLY);

	XMLHelper* xml = create_xml_helper();
	if (!xml) {
		printf("EditorSettings::EditorSettings() - "
			   "failed to create XMLHelper\n");
		return B_NO_MEMORY;
	}

	ret = xml->Load(file);
	if (ret < B_OK) {
		printf("EditorSettings::EditorSettings() - "
			   "failed to load settings file: %s\n", strerror(ret));
		delete xml;
		return ret;
	}

	BString string;
	ret = xml->GetTagName(string);
	if (ret < B_OK || (string != "CLOCKWERK_SETTINGS"
			&& string != "CONTROLLER_SETTINGS")) {
				// "CONTROLLER_SETTINGS" for backwards compatibility
		printf("EditorSettings::EditorSettings() - "
			   "invalid settings file: %s\n", strerror(ret));
		delete xml;
		return ret;
	}

	// ok, everything is fine, read the parameters
	fServer = xml->GetAttribute("server", fServer.String());
	fMediaFolder = xml->GetAttribute("media_folder", fMediaFolder.String());
	fClientID = xml->GetAttribute("client_id", fClientID.String());

	delete xml;
#endif

	return B_OK;
}
