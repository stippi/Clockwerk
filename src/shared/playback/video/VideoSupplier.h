/*
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef VIDEO_SUPPLIER_H
#define VIDEO_SUPPLIER_H

#include <SupportDefs.h>
#include <List.h>

struct media_format;

class VideoSupplier {
 public:
								VideoSupplier();
	virtual						~VideoSupplier();

	virtual	status_t			FillBuffer(int64 startFrame,
										   void* buffer,
										   const media_format* format,
										   bool& wasCached) = 0;

	virtual	void				DeleteCaches();

 	inline	bigtime_t			ProcessingLatency() const
 									{ return fProcessingLatency; }

 protected:
		 	bigtime_t			fProcessingLatency;
};

#endif	// VIDEO_SUPPLIER_H
