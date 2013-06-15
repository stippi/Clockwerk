/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "TimeSource.h"

#include <stdio.h>
#include <string.h>

#include <MediaRoster.h>
#include <Screen.h>

#include "common.h"

#include "AutoLocker.h"


// constructor
TimeSource::TimeSource()
	: BMediaNode("Clockwerk Simple Playback"),

	  fTimerThread(-1),
	  fQuitting(false),
	  fPaused(false),
	  fInitSem(-1)
{
	// default estimated retrace duration for a 60 Hz display
	estimatedRetraceDuration = 16667;

	// in case we have a valid screen, calculate the estimated
	// retrace duration based on the actual screen refresh rate
	BScreen screen(B_MAIN_SCREEN_ID);
	display_mode mode;
	if (!screen.IsValid() || screen.GetMode(&mode) != B_OK)
		return;

	double refreshRate = (10 * double(mode.timing.pixel_clock * 1000)
		/ double(mode.timing.h_total * mode.timing.v_total)) / 10.0;

	estimatedRetraceDuration = (bigtime_t)(1000000.0 / refreshRate);
	print_info("screen refresh rate: %.2f Hz, estimated retrace "
		"duration: %lld µs\n", refreshRate, estimatedRetraceDuration);
}

// destructor
TimeSource::~TimeSource()
{
	fQuitting = true;
	if (fTimerThread >= 0) {
		status_t ret;
		wait_for_thread(fTimerThread, &ret);
	}
}

// TimeSourceOp
status_t
TimeSource::TimeSourceOp(const time_source_op_info& op,
									void* _reserved)
{
	switch (op.op) {
		case B_TIMESOURCE_START:
			printf("TimeSource::TimeSourceOp(B_TIMESOURCE_START)\n");
			fPaused = false;
			return B_OK;

		case B_TIMESOURCE_STOP:
			printf("TimeSource::TimeSourceOp(B_TIMESOURCE_STOP)\n");
			fPaused = true;
			PublishTime(0, 0, 0.0);
			return B_OK;

		case B_TIMESOURCE_STOP_IMMEDIATELY:
			printf("TimeSource::TimeSourceOp(B_TIMESOURCE_STOP_IMMEDIATELY)\n");
			fPaused = true;
			PublishTime(0, 0, 0.0);
			return B_OK;

		case B_TIMESOURCE_SEEK:
			printf("TimeSource::TimeSourceOp(B_TIMESOURCE_SEEK)\n");
//			BroadcastTimeWarp(op.real_time, op.performance_time);
			return B_ERROR;
	}
	return B_ERROR;
}

// AddOn
BMediaAddOn*
TimeSource::AddOn(int32* internalID) const
{
	// not supposed to live in an add-on
	return NULL;
}

// #pragma mark -

// Init
status_t
TimeSource::Init()
{
	if (fTimerThread >= 0)
		return B_OK;

	fInitSem = create_sem(0, "timesource init sem");
	if (fInitSem < 0)
		return (status_t)fInitSem;

	// spawn time source thread
	fTimerThread = spawn_thread(_TimerEntry, "timer",
								B_REAL_TIME_PRIORITY, this);
	status_t ret;
	if (fTimerThread >= B_OK)
		ret = resume_thread(fTimerThread);
	else
		ret = fTimerThread;

	if (ret < B_OK)
		return ret;

	// register with the media_server
	ret = B_OK;
	BMediaRoster* mediaRoster = BMediaRoster::Roster(&ret);
	if (ret != B_OK) {
		printf("Can't find the media roster: %s\n", strerror(ret));
		mediaRoster = NULL;
		return ret;
	}
	
	AutoLocker<BMediaRoster> rosterLocker(mediaRoster);
	if (!rosterLocker.IsLocked())
		return B_ERROR;

	ret = mediaRoster->RegisterNode(this);
	if (ret != B_OK) {
		printf("Can't register with the media roster: %s\n", strerror(ret));
		return ret;
	}

	ret = mediaRoster->StartTimeSource(Node(), RealTime());
	if (ret != B_OK) {
		printf("Can't start time source: %s\n", strerror(ret));
		return ret;
	}

	// wait for the timer thread to init the first members
	while (acquire_sem(fInitSem) == B_INTERRUPTED)
		;

	return B_OK;
}

// TimePerVideoFrame
double
TimeSource::TimePerVideoFrame(int32 frameRateScale)
{
	AutoLocker<TimeSource> _(this);
	return double(lastRetraceRealTime - firstRetraceRealTime)
		* frameRateScale / retraceCount;
}

// #pragma mark -

// _TimerEntry
int32
TimeSource::_TimerEntry(void* cookie)
{
	TimeSource* pm = (TimeSource*)cookie;
	pm->_Timer();
	return 0;
}

// _Timer
void
TimeSource::_Timer()
{
	BScreen screen(B_MAIN_SCREEN_ID);
	bool waitForRetraceSupported = screen.WaitForRetrace() >= B_OK;
	if (!waitForRetraceSupported) {
		print_warning("graphics driver does not support v-sync, playback "
			"may not be as fluent\n");
	}

	// init time source info
	bigtime_t realTime = RealTime();

	Lock();

	firstRetraceRealTime = realTime;

	lastRetraceRealTime = realTime;
	lastRetracePerformanceTime = 0;

	lastSeekRealTime = realTime;
	lastSeekPerformanceTime = 0;

	retraceCount = 0;
	timeDrift = 1.0;

	Unlock();


	while (!fQuitting) {

		if (waitForRetraceSupported) {
			screen.WaitForRetrace();
			realTime = RealTime();
		} else {
			realTime += estimatedRetraceDuration;
			snooze_until(realTime, B_SYSTEM_TIMEBASE);
		}
		// update time source info
		Lock();

		lastRetraceRealTime = realTime;
		retraceCount++;

		double retraceDuration
			= (lastRetraceRealTime - firstRetraceRealTime)
				/ (double)retraceCount;

		lastRetracePerformanceTime = (bigtime_t)(
			lastSeekPerformanceTime
			+ (lastRetraceRealTime - lastSeekRealTime)
				* estimatedRetraceDuration / retraceDuration);

		timeDrift
			= estimatedRetraceDuration / retraceDuration;

		Unlock();

		if (fInitSem >= 0) {
			delete_sem(fInitSem);
			fInitSem = -1;
		}

//PrintToStream();

		if (!fPaused) {
			PublishTime(lastRetracePerformanceTime,
						lastRetraceRealTime,
						timeDrift);
		}
	}
}

// PrintToStream
void
TimeSource::PrintToStream()
{
	printf("PublishTime(): (paused: %d)\n"
		"	lastRetracePerformanceTime %lld,\n"
		"	lastRetraceRealTime: %lld,\n"
		"	timeDrift: %.4f\n",
		fPaused, lastRetracePerformanceTime, lastRetraceRealTime, timeDrift);
}

