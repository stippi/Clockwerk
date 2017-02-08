/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

//	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
//	SMS

#ifndef VIDEO_CONSUMER_H
#define VIDEO_CONSUMER_H

#include <BufferConsumer.h>
#include <Locker.h>
#include <MediaEventLooper.h>

class BBitmap;
class NodeManager;
class VCTarget;

// TODO: The buffer count should depend on the latency!
static const unsigned int kBufferCount = 4;

class VideoConsumer : public BMediaEventLooper, public BBufferConsumer {
 public:
								VideoConsumer(
									const char* name,
									BMediaAddOn* addon,
									const uint32 internal_id,
									NodeManager* manager,
									VCTarget* target);
								~VideoConsumer();
	
/*	BMediaNode */
 public:
	
	virtual	BMediaAddOn			*AddOn(int32 *cookie) const;
	
 protected:

	virtual void				Start(bigtime_t performance_time);
	virtual void				Stop(bigtime_t performance_time, bool immediate);
	virtual void				Seek(bigtime_t media_time, bigtime_t performance_time);
	virtual void				TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time);
	// Workaround for a Metrowerks PPC compiler bug
	virtual	status_t			AddTimer(bigtime_t at_performance_time,
										 int32 cookie);

	virtual void				NodeRegistered();
	virtual	status_t		 	RequestCompleted(
									const media_request_info & info);
							
	virtual	status_t 			HandleMessage(int32 message, const void * data,
											  size_t size);

	// Workaround for a Metrowerks PPC compiler bug
	virtual	void				CleanUpEvent(const media_timed_event *event);

	// Workaround for a Metrowerks PPC compiler bug
	virtual	bigtime_t			OfflineTime();

	// Workaround for a Metrowerks PPC compiler bug
	virtual	void				ControlLoop();

	// Workaround for a Metrowerks PPC compiler bug
	virtual status_t			DeleteHook(BMediaNode * node);

/*  BMediaEventLooper */
 protected:
	virtual void				HandleEvent(const media_timed_event *event,
											bigtime_t lateness,
											bool realTimeEvent);
/*	BBufferConsumer */
 public:
	
	virtual	status_t			AcceptFormat(const media_destination &dest,
											 media_format * format);
	virtual	status_t			GetNextInput(int32 * cookie,
											 media_input * out_input);
							
	virtual	void				DisposeInputCookie(int32 cookie);
	
 protected:

	virtual	void				BufferReceived(BBuffer * buffer);
	
 private:

	virtual	void				ProducerDataStatus(
									const media_destination &for_whom,
									int32 status,
									bigtime_t at_media_time);									
	virtual	status_t			GetLatencyFor(
									const media_destination &for_whom,
									bigtime_t * out_latency,
									media_node_id * out_id);	
	virtual	status_t			Connected(const media_source &producer,
										  const media_destination &where,
										  const media_format & with_format,
										  media_input * out_input);
	virtual	void				Disconnected(const media_source &producer,
											 const media_destination &where);							
	virtual	status_t			FormatChanged(const media_source & producer,
									const media_destination & consumer, 
									int32 from_change_count,
									const media_format & format);
							
/*	implementation */

 public:
			status_t			CreateBuffers(
									const media_format & with_format);
							
			void				DeleteBuffers();

			void				SetTarget(VCTarget* target);
							
 private:
			uint32				fInternalID;
			BMediaAddOn*		fAddOn;

			bool				fConnectionActive;
			media_input			fIn;
			media_destination	fDestination;
			bigtime_t			fMyLatency;

			BBitmap*			fBitmap[kBufferCount];
			bool				fOurBuffers;
			BBufferGroup*		fBuffers;
			addr_t				fBufferMap[kBufferCount];	

			NodeManager*		fManager;
			BLocker				fTargetLock;	// locks the following variable
			VCTarget* volatile	fTarget;
			int32				fTargetBufferIndex;
};

#endif // VIDEO_CONSUMER_H
