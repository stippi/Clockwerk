/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIME_SOURCE_H
#define TIME_SOURCE_H

#include <Locker.h>
#include <TimeSource.h>

class TimeSource : public virtual BMediaNode,
				   public BTimeSource,
				   public BLocker {
 public:
								TimeSource();
	virtual						~TimeSource();

	// BTimeSource interface
	virtual	status_t			TimeSourceOp(
									const time_source_op_info& op,
									void* _reserved);

	virtual	BMediaAddOn*		AddOn(int32* internalID) const;


	// TimeSource
			status_t			Init();

			double				TimePerVideoFrame(int32 frameRateScale = 2);

			void				PrintToStream();

 private:
	static	int32				_TimerEntry(void* cookie);
			void				_Timer();

			thread_id			fTimerThread;
	volatile bool				fQuitting;
	volatile bool				fPaused;
	volatile sem_id				fInitSem;

 public:
	// you need to lock the object before accessing any
	// of these:
			bigtime_t			firstRetraceRealTime;
		
			bigtime_t			lastRetraceRealTime;
			bigtime_t			lastRetracePerformanceTime;

			bigtime_t			lastSeekRealTime;
			bigtime_t			lastSeekPerformanceTime;
		
			int64				retraceCount;
			bigtime_t			estimatedRetraceDuration;	

			float				timeDrift;
};

#endif // TIME_SOURCE_H

