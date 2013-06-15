/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COMMON_H
#define COMMON_H

#include <stdarg.h>

#include <SupportDefs.h>

#include "common_constants.h"

#include "StatusOutput.h"

class BDataIO;
class BMessage;
class BPath;
class BString;
class ServerObject;

struct entry_ref;

void fatal(status_t error);

off_t copy_data(BDataIO& source, BDataIO& destination, off_t size = -1);

// output
void set_global_output(StatusOutput* statusOutput);

void print_info(const char* format, ...);
void print_warning(const char* format, ...);
void print_error(const char* format, ...);

#endif // COMMON_H
