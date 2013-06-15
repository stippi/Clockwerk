/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ComponentQualityInfo.h"

#include <stdio.h>

#include <File.h>

struct ComponentQualityInfo::Data {
	int32	permanent_problem_count;
	int32	temporary_problem_count;
};

// constructor
ComponentQualityInfo::ComponentQualityInfo()
	: permanent_problem_count(0),
	  temporary_problem_count(0),
	  fRef()
{
}

// destructor
ComponentQualityInfo::~ComponentQualityInfo()
{
}

// Init
status_t
ComponentQualityInfo::Init(const entry_ref& appRef)
{
	// construct report file name from app name
	// and copy other entry params
	char name[B_FILE_NAME_LENGTH];
	sprintf(name, "report.%s", appRef.name);

	fRef.device = appRef.device;
	fRef.directory = appRef.directory;

	status_t ret = fRef.set_name(name);
	if (ret < B_OK)
		return ret;

	Data d;
	d.permanent_problem_count = 0;
	d.temporary_problem_count = 0;

	// load the report if it exists
	BFile file(&fRef, B_READ_ONLY);
	if (file.InitCheck() >= B_OK) {
		file.Read(&d, sizeof(Data));
	}

	permanent_problem_count = d.permanent_problem_count;
	temporary_problem_count = d.temporary_problem_count;

	return B_OK;
}

// Save
status_t
ComponentQualityInfo::Save() const
{
	if (!fRef.name)
		return B_NO_INIT;

	// try to save the report
	BFile file(&fRef, B_CREATE_FILE | B_ERASE_FILE | B_WRITE_ONLY);
	status_t ret = file.InitCheck();
	if (ret >= B_OK) {
		Data d;
		d.permanent_problem_count = permanent_problem_count;
		d.temporary_problem_count = temporary_problem_count;
	
		ssize_t written = file.Write(&d, sizeof(Data));
		if (written < (ssize_t)sizeof(Data))
			ret = written < B_OK ? written : B_ERROR;
	}
	return ret;
}

