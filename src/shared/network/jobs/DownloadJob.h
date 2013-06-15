/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DOWNLOAD_JOB_H
#define DOWNLOAD_JOB_H

#include <Path.h>

#include "InformingJob.h"


class BFile;
class BDataIO;


class DownloadJob : public InformingJob {
public:
								DownloadJob(const char* directory,
									const BString& serverID,
									int32 version,
									BHandler* handler,
									BMessage* message,
									bool getData = true);
								DownloadJob(const BString& serverID,
									int32 version,
									BDataIO* data,
									BHandler* handler,
									BMessage* message);
	virtual						~DownloadJob();

	virtual	status_t			Execute(JobConnection* jobConnection);

			const BString&		ServerID() const
									{ return fServerID; }

private:
			status_t			_WriteChunk(RequestConnection* connection,
									StatusOutput* statusOutput, BDataIO& file,
									int32 chunkSize, void* buffer,
									int32 bufferSize);
			void				_CleanUpFailedDownloads(StatusOutput* output);


			BPath				fPath;
			BString				fServerID;
			int32				fVersion;
			BDataIO*			fData;
			bool				fGetData;
};

#endif	// DOWNLOAD_JOB_H
