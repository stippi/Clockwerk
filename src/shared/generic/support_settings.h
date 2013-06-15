/*
 * Copyright 2001-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SUPPORT_SETTINGS_H
#define SUPPORT_SETTINGS_H

#include <GraphicsDefs.h>

class BMessage;

status_t load_settings(BMessage* message, const char* fileName,
					   const char* folder = NULL);

status_t save_settings(BMessage* message, const char* fileName,
					   const char* folder = NULL);


# endif // SUPPORT_SETTINGS_H
