/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2000-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "VideoProducer.h"

#include <stdio.h>
#include <string.h>

#include <Autolock.h>
#include <Buffer.h>
#include <BufferGroup.h>
#include <Debug.h>
#include <TimeSource.h>

#include "NodeManager.h"
#include "VideoSupplier.h"


// debugging
#include "Debug.h"
#define ldebug		debug
//#define ldebug		nodebug
#define	FUNCTION	nodebug


#define BUFFER_COUNT 3

#define TOUCH(x) ((void)(x))

#define PRINTF(a,b) \
		do { \
			if (a < 2) { \
				ldebug("VideoProducer::"); \
				ldebug b; \
			} \
		} while (0)

// constructor
VideoProducer::VideoProducer(
		BMediaAddOn *addon, const char *name, int32 internal_id,
		NodeManager* manager, VideoSupplier* supplier)
	: BMediaNode(name),
	  BMediaEventLooper(),
	  BBufferProducer(B_MEDIA_RAW_VIDEO),
	  fInitStatus(B_NO_INIT),
	  fInternalID(internal_id),
	  fAddOn(addon),
	  fBufferGroup(NULL),
	  fUsedBufferGroup(NULL),
	  fThread(-1),
	  fFrameSync(-1),
	  fRunning(false),
	  fConnected(false),
	  fEnabled(false),
	  fManager(manager),
	  fSupplier(supplier)
{
	fOutput.destination = media_destination::null;
	fInitStatus = B_OK;
}

// destructor
VideoProducer::~VideoProducer()
{
	if (fInitStatus == B_OK) {
		// Clean up after ourselves, in case the application didn't make us do so.
		if (fConnected)
			Disconnect(fOutput.source, fOutput.destination);
		if (fRunning)
			_HandleStop();
	}
	Quit();
}

// ControlPort
port_id
VideoProducer::ControlPort() const
{
	return BMediaNode::ControlPort();
}

// AddOn
BMediaAddOn*
VideoProducer::AddOn(int32 *internal_id) const
{
	if (internal_id)
		*internal_id = fInternalID;
	return fAddOn;
}

// HandleMessage
status_t 
VideoProducer::HandleMessage(int32 message, const void* data, size_t size)
{
	return B_ERROR;
}

// Preroll
void 
VideoProducer::Preroll()
{
	// This hook may be called before the node is started
	// to give the hardware a chance to start.
}

// SetTimeSource
void
VideoProducer::SetTimeSource(BTimeSource* time_source)
{
	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}

// NodeRegistered
status_t
VideoProducer::RequestCompleted(const media_request_info &info)
{
	return BMediaNode::RequestCompleted(info);
}

// NodeRegistered
void 
VideoProducer::NodeRegistered()
{
	if (fInitStatus != B_OK) {
		ReportError(B_NODE_IN_DISTRESS);
		return;
	}

	fOutput.node = Node();
	fOutput.source.port = ControlPort();
	fOutput.source.id = 0;
	fOutput.destination = media_destination::null;
	strcpy(fOutput.name, Name());	

	// Tailor these for the output of your device
	fOutput.format.type = B_MEDIA_RAW_VIDEO;
	// wild cards yet
	fOutput.format.u.raw_video = media_raw_video_format::wildcard;
	fOutput.format.u.raw_video.interlace = 1;
	fOutput.format.u.raw_video.display.format = B_NO_COLOR_SPACE;
	fOutput.format.u.raw_video.display.bytes_per_row = 0;
	fOutput.format.u.raw_video.display.line_width = 0;
	fOutput.format.u.raw_video.display.line_count = 0;

	// Start the BMediaEventLooper control loop running
	Run();
}

// Start
void
VideoProducer::Start(bigtime_t performance_time)
{
	// notify the manager in case we were started from the outside world
//	fManager->StartPlaying();

	BMediaEventLooper::Start(performance_time);
}

// Stop
void
VideoProducer::Stop(bigtime_t performance_time, bool immediate)
{
	// notify the manager in case we were stopped from the outside world
//	fManager->StopPlaying();

	BMediaEventLooper::Stop(performance_time, immediate);
}

// Seek
void
VideoProducer::Seek(bigtime_t media_time, bigtime_t performance_time)
{
	BMediaEventLooper::Seek(media_time, performance_time);
}

// TimeWarp
void
VideoProducer::TimeWarp(bigtime_t at_real_time, bigtime_t to_performance_time)
{
	BMediaEventLooper::TimeWarp(at_real_time, to_performance_time);
}

// AddTimer
status_t
VideoProducer::AddTimer(bigtime_t at_performance_time, int32 cookie)
{
	return BMediaEventLooper::AddTimer(at_performance_time, cookie);
}

// SetRunMode
void
VideoProducer::SetRunMode(run_mode mode)
{
	BMediaEventLooper::SetRunMode(mode);
}

// HandleEvent
void 
VideoProducer::HandleEvent(const media_timed_event *event,
		bigtime_t lateness, bool realTimeEvent)
{
	TOUCH(lateness); TOUCH(realTimeEvent);

	switch(event->type)
	{
		case BTimedEventQueue::B_START:
ldebug("VideoProducer::HandleEvent(B_START)\n");
			_HandleStart(event->event_time);
ldebug("VideoProducer::HandleEvent(B_START) done\n");
			break;
		case BTimedEventQueue::B_STOP:
ldebug("VideoProducer::HandleEvent(B_STOP)\n");
			_HandleStop();
ldebug("VideoProducer::HandleEvent(B_STOP) done\n");
			break;
		case BTimedEventQueue::B_WARP:
			_HandleTimeWarp(event->bigdata);
			break;
		case BTimedEventQueue::B_SEEK:
			_HandleSeek(event->bigdata);
			break;
		case BTimedEventQueue::B_HANDLE_BUFFER:
		case BTimedEventQueue::B_DATA_STATUS:
		case BTimedEventQueue::B_PARAMETER:
		default:
			PRINTF(-1, ("HandleEvent: Unhandled event -- %lx\n", event->type));
			break;
	}
}

// CleanUpEvent
void 
VideoProducer::CleanUpEvent(const media_timed_event *event)
{
	BMediaEventLooper::CleanUpEvent(event);
}

// OfflineTime
bigtime_t
VideoProducer::OfflineTime()
{
	return BMediaEventLooper::OfflineTime();
}

// ControlLoop
void
VideoProducer::ControlLoop()
{
	BMediaEventLooper::ControlLoop();
}

// DeleteHook
status_t
VideoProducer::DeleteHook(BMediaNode* node)
{
	return BMediaEventLooper::DeleteHook(node);
}

// FormatSuggestionRequested
status_t 
VideoProducer::FormatSuggestionRequested(media_type type, int32 quality,
										 media_format* format)
{
	FUNCTION("VideoProducer::FormatSuggestionRequested\n");

	if (type != B_MEDIA_ENCODED_VIDEO)
		return B_MEDIA_BAD_FORMAT;

	TOUCH(quality);

	*format = fOutput.format;
	return B_OK;
}

// FormatProposal
status_t 
VideoProducer::FormatProposal(const media_source &output, media_format *format)
{
	char string[256];		
	string_for_format(*format, string, 256);
	FUNCTION("VideoProducer::FormatProposal(%s)\n", string);

	status_t err;

	if (!format)
		return B_BAD_VALUE;

	if (output != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

	err = format_is_compatible(*format, fOutput.format) ?
			B_OK : B_MEDIA_BAD_FORMAT;
if (err != B_OK) {
printf("VideoProducer::FormatProposal() error\n");
}
	// change any wild cards to specific values
	

	return err;
		
}

// FormatChangeRequested
status_t 
VideoProducer::FormatChangeRequested(const media_source& source,
									 const media_destination& destination,
									 media_format* io_format,
									 int32 *_deprecated_)
{
	TOUCH(destination); TOUCH(io_format); TOUCH(_deprecated_);
	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
		
	return B_ERROR;	
}

// GetNextOutput
status_t 
VideoProducer::GetNextOutput(int32* cookie, media_output* out_output)
{
	if (!out_output)
		return B_BAD_VALUE;

	if ((*cookie) != 0)
		return B_BAD_INDEX;
	
	*out_output = fOutput;
	(*cookie)++;
	return B_OK;
}

// DisposeOutputCookie
status_t 
VideoProducer::DisposeOutputCookie(int32 cookie)
{
	TOUCH(cookie);

	return B_OK;
}

// SetBufferGroup
status_t 
VideoProducer::SetBufferGroup(const media_source &for_source,
		BBufferGroup *group)
{
	if (for_source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;

ldebug("VideoProducer::SetBufferGroup() - using buffer group of consumer.\n");
	fUsedBufferGroup = group;

	return B_OK;
}

// VideoClippingChanged
status_t 
VideoProducer::VideoClippingChanged(const media_source& for_source,
									int16 num_shorts, int16* clip_data,
									const media_video_display_info& display,
									int32* _deprecated_)
{
	TOUCH(for_source); TOUCH(num_shorts); TOUCH(clip_data);
	TOUCH(display); TOUCH(_deprecated_);

	return B_ERROR;
}

// GetLatency
status_t 
VideoProducer::GetLatency(bigtime_t* out_latency)
{
	*out_latency = EventLatency() + SchedulingLatency();
	return B_OK;
}

// PrepareToConnect
status_t 
VideoProducer::PrepareToConnect(const media_source& source,
								const media_destination& destination,
								media_format* format,
								media_source* out_source, char* out_name)
{
	FUNCTION("VideoProducer::PrepareToConnect() %ldx%ld\n",
			 format->u.raw_video.display.line_width,
			 format->u.raw_video.display.line_count);

	if (fConnected) {
		printf("VideoProducer::PrepareToConnect() - already fConnected\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	if (source != fOutput.source)
		return B_MEDIA_BAD_SOURCE;
	
	if (fOutput.destination != media_destination::null) {
		printf("VideoProducer::PrepareToConnect() - destination != null\n");
		return B_MEDIA_ALREADY_CONNECTED;
	}

	// The format parameter comes in with the suggested format, and may be
	// specialized as desired by the node
	if (!format_is_compatible(*format, fOutput.format)) {
		printf("VideoProducer::PrepareToConnect() - incompatible format\n");
		*format = fOutput.format;
		return B_MEDIA_BAD_FORMAT;
	}

	if (format->u.raw_video.display.line_width == 0)
		format->u.raw_video.display.line_width = 384;
	if (format->u.raw_video.display.line_count == 0)
		format->u.raw_video.display.line_count = 288;
	if (format->u.raw_video.field_rate == 0)
		format->u.raw_video.field_rate = 25.0;
	if (format->u.raw_video.display.bytes_per_row == 0)
		format->u.raw_video.display.bytes_per_row = format->u.raw_video.display.line_width * 4;

	*out_source = fOutput.source;
	strcpy(out_name, fOutput.name);

// TODO: why was this set here? It is set again in Connect() and
// if an error happens after PrepareToConnect, any further atempt
// to connect will fail
//	fOutput.destination = destination;

	return B_OK;
}

#define NODE_LATENCY 20000

// Connect
void 
VideoProducer::Connect(status_t error, const media_source& source,
					   const media_destination& destination,
					   const media_format& format, char* io_name)
{
	FUNCTION("VideoProducer::Connect() %ldx%ld\n", format.u.raw_video.display.line_width,
			 format.u.raw_video.display.line_count);

	if (fConnected) {
		printf("VideoProducer::Connect() - already connected\n");
		return;
	}

	if (source != fOutput.source) {
		printf("VideoProducer::Connect() - wrong source\n");
		return;
	}
	if (error < B_OK) {
		printf("VideoProducer::Connect() - consumer error: %s\n", strerror(error));
		return;
	}
	if (!const_cast<media_format*>(&format)->Matches(&fOutput.format)) {
		printf("VideoProducer::Connect() - format mismatch\n");
		return;
	}

	fOutput.destination = destination;
	strcpy(io_name, fOutput.name);

	if (fOutput.format.u.raw_video.field_rate != 0.0f) {
		fPerformanceTimeBase = fPerformanceTimeBase +
				(bigtime_t)
					((fFrame - fFrameBase) *
					(1000000 / fOutput.format.u.raw_video.field_rate));
		fFrameBase = fFrame;
	}
	
	fConnectedFormat = format.u.raw_video;
	if (fConnectedFormat.display.bytes_per_row == 0) {
		printf("VideoProducer::Connect() - connected format still has BPR wildcard!\n");
		fConnectedFormat.display.bytes_per_row = 4 * fConnectedFormat.display.line_width;
	}

	// get the latency
	bigtime_t latency = 0;
	media_node_id tsID = 0;
	FindLatencyFor(fOutput.destination, &latency, &tsID);
	SetEventLatency(latency + NODE_LATENCY);

	// Create the buffer group
	if (!fUsedBufferGroup) {
		fBufferGroup = new BBufferGroup(fConnectedFormat.display.bytes_per_row *
										fConnectedFormat.display.line_count,
										BUFFER_COUNT);
		status_t err = fBufferGroup->InitCheck();
		if (err < B_OK) {
			delete fBufferGroup;
			fBufferGroup = NULL;
			printf("VideoProducer::Connect() - buffer group error: %s\n", strerror(err));
			return;
		}
		fUsedBufferGroup = fBufferGroup;
	}

	fConnected = true;
	fEnabled = true;

	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}

// Disconnect
void 
VideoProducer::Disconnect(const media_source& source,
						  const media_destination& destination)
{
	FUNCTION("VideoProducer::Disconnect()\n");

	if (!fConnected) {
		printf("VideoProducer::Disconnect() - Not connected\n");
		return;
	}

	if ((source != fOutput.source) || (destination != fOutput.destination)) {
		printf("VideoProducer::Disconnect() - Bad source and/or destination\n");
		return;
	}

	fEnabled = false;
	fOutput.destination = media_destination::null;

	if (fLock.Lock()) {
		// Always delete the buffer group, even if it is not ours.
		// (See BeBook::SetBufferGroup()).
		delete fUsedBufferGroup;
		fUsedBufferGroup = NULL;
		fBufferGroup = NULL;
		fLock.Unlock();
	}

	fConnected = false;
	PRINTF(1, ("Disconnect() done\n"));
}

// LateNoticeReceived
void 
VideoProducer::LateNoticeReceived(const media_source &source,
		bigtime_t how_much, bigtime_t performance_time)
{
	TOUCH(source); TOUCH(how_much); TOUCH(performance_time);
	ldebug("Late!!!\n");
}

// EnableOutput
void 
VideoProducer::EnableOutput(const media_source& source, bool enabled,
							int32* _deprecated_)
{
	TOUCH(_deprecated_);

	if (source != fOutput.source)
		return;

	fEnabled = enabled;
}

// SetPlayRate
status_t 
VideoProducer::SetPlayRate(int32 numer, int32 denom)
{
	TOUCH(numer); TOUCH(denom);

	return B_ERROR;
}

// AdditionalBufferRequested
void 
VideoProducer::AdditionalBufferRequested(const media_source& source,
										 media_buffer_id prev_buffer,
										 bigtime_t prev_time,
										 const media_seek_tag* prev_tag)
{
	TOUCH(source); TOUCH(prev_buffer); TOUCH(prev_time); TOUCH(prev_tag);
}

// LatencyChanged
void 
VideoProducer::LatencyChanged(const media_source& source,
							  const media_destination& destination,
							  bigtime_t new_latency, uint32 flags)
{
	TOUCH(source); TOUCH(destination); TOUCH(new_latency); TOUCH(flags);
	ldebug("Latency changed!\n");
}

// _HandleStart
void
VideoProducer::_HandleStart(bigtime_t performance_time)
{
	// Start producing frames, even if the output hasn't been connected yet.
	PRINTF(1, ("_HandleStart(%Ld)\n", performance_time));

	if (fRunning) {
		PRINTF(-1, ("_HandleStart: Node already started\n"));
		return;
	}

	fFrame = 0;
	fFrameBase = 0;
	fPerformanceTimeBase = performance_time;

	fFrameSync = create_sem(0, "frame synchronization");
	if (fFrameSync < B_OK)
		goto err1;

	fThread = spawn_thread(_frame_generator_, "frame generator",
			B_NORMAL_PRIORITY, this);
	if (fThread < B_OK)
		goto err2;

	resume_thread(fThread);

	fRunning = true;
	return;

err2:
	delete_sem(fFrameSync);
err1:
	return;
}

// _HandleStop
void
VideoProducer::_HandleStop(void)
{
	PRINTF(1, ("_HandleStop()\n"));

	if (!fRunning) {
		PRINTF(-1, ("_HandleStop: Node isn't running\n"));
		return;
	}

ldebug("VideoProducer::_HandleStop(): delete_sem()\n");
	delete_sem(fFrameSync);
	wait_for_thread(fThread, &fThread);

	fRunning = false;
}

// _HandleTimeWarp
void
VideoProducer::_HandleTimeWarp(bigtime_t performance_time)
{
	fPerformanceTimeBase = performance_time;
	fFrameBase = fFrame;

	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}

// _HandleSeek
void
VideoProducer::_HandleSeek(bigtime_t performance_time)
{
	fPerformanceTimeBase = performance_time;
	fFrameBase = fFrame;

	// Tell frame generation thread to recalculate delay value
	release_sem(fFrameSync);
}

// _frame_generator_
//
// The following functions form the thread that generates frames. You should
// replace this with the code that interfaces to your hardware.
int32
VideoProducer::_frame_generator_(void *data)
{
	return ((VideoProducer *)data)->_FrameGenerator();
}

// _FrameGenerator
int32 
VideoProducer::_FrameGenerator()
{
	bool forceSendingBuffer = true;
	bigtime_t lastFrameSentAt = 0;
	bool running = true;
	while (running) {
ldebug("VideoProducer: loop: %Ld\n", fFrame);
		// lock the node manager
		status_t err = fManager->LockWithTimeout(10000);
		bool ignoreEvent = false;
		// Data to be retrieved from the node manager.
		bigtime_t performanceTime = 0;
		bigtime_t nextPerformanceTime = 0;
		bigtime_t waitUntil = 0;
		bigtime_t nextWaitUntil = 0;
		bigtime_t maxRenderTime = 0;
		int32 playingDirection = 0;
		int64 playlistFrame = 0;
		switch (err) {
			case B_OK: {
ldebug("VideoProducer: node manager successfully locked\n");
				// get the times for the current and the next frame
				performanceTime = fManager->TimeForFrame(fFrame);
				nextPerformanceTime = fManager->TimeForFrame(fFrame + 1);
				maxRenderTime = min_c(bigtime_t(33334 * 0.9),
									  max_c(fSupplier->ProcessingLatency(), maxRenderTime));

				waitUntil = TimeSource()->RealTimeFor(fPerformanceTimeBase
													+ performanceTime, 0) - maxRenderTime;
				nextWaitUntil = TimeSource()->RealTimeFor(fPerformanceTimeBase
													+ nextPerformanceTime, 0) - maxRenderTime;
				// get playing direction and playlist frame for the current frame
				bool newPlayingState;
				playlistFrame = fManager->PlaylistFrameAtFrame(fFrame,
															   playingDirection,
															   newPlayingState);
ldebug("VideoProducer: performance time: %Ld, playlist frame: %Ld\n",
performanceTime, playlistFrame);
				forceSendingBuffer |= newPlayingState;
				fManager->SetCurrentVideoTime(nextPerformanceTime);
				fManager->Unlock();
				break;
			}
			case B_TIMED_OUT:
ldebug("VideoProducer: Couldn't lock the node manager.\n");
				ignoreEvent = true;
				waitUntil = system_time() - 1;
				break;
			default:
				printf("Couldn't lock the node manager. Terminating video producer "
					   "frame generator thread.\n");
				ignoreEvent = true;
				waitUntil = system_time() - 1;
				running = false;
				break;
		}
		// Force sending a frame, if the last one has been sent more than
		// one second ago.
		if (lastFrameSentAt + 1000000 < performanceTime)
			forceSendingBuffer = true;

ldebug("VideoProducer: waiting (%Ld)...\n", waitUntil);
		// wait until...
		err = acquire_sem_etc(fFrameSync, 1, B_ABSOLUTE_TIMEOUT, waitUntil);
		// The only acceptable responses are B_OK and B_TIMED_OUT. Everything
		// else means the thread should quit. Deleting the semaphore, as in
		// VideoProducer::_HandleStop(), will trigger this behavior.
		switch (err) {
			case B_OK:
ldebug("VideoProducer::_FrameGenerator - going back to sleep.\n");
				break;
			case B_TIMED_OUT:
ldebug("VideoProducer: timed out => event\n");
				// Catch the cases in which the node manager could not be
				// locked and we therefore have no valid data to work with,
				// or the producer is not running or enabled.
				if (ignoreEvent || !fRunning || !fEnabled) {
ldebug("VideoProducer: ignore event\n");
					// nothing to do
				// Drop frame if it's at least a frame late.
				} else if (nextWaitUntil < system_time()) {
//printf("VideoProducer: dropped frame (%ld)\n", fFrame);
					if (fManager->LockWithTimeout(10000) == B_OK) {
						fManager->FrameDropped();
						fManager->Unlock();
					}
					// next frame
					fFrame++;
				// Send buffers only, if playing, the node is running and the
				// output has been enabled
				} else if (playingDirection != 0 || forceSendingBuffer) {
ldebug("VideoProducer: produce frame\n");
					BAutolock _(fLock);
					// Fetch a buffer from the buffer group
					BBuffer *buffer = fUsedBufferGroup->RequestBuffer(
						fConnectedFormat.display.bytes_per_row
						* fConnectedFormat.display.line_count, 0LL);
					if (buffer) {
						// Fill out the details about this buffer.
						media_header *h = buffer->Header();
						h->type = B_MEDIA_RAW_VIDEO;
						h->time_source = TimeSource()->ID();
						h->size_used = fConnectedFormat.display.bytes_per_row
									   * fConnectedFormat.display.line_count;
						// For a buffer originating from a device, you might
						// want to calculate this based on the
						// PerformanceTimeFor the time your buffer arrived at
						// the hardware (plus any applicable adjustments).
						h->start_time = fPerformanceTimeBase + performanceTime;
						h->file_pos = 0;
						h->orig_size = 0;
						h->data_offset = 0;
						h->u.raw_video.field_gamma = 1.0;
						h->u.raw_video.field_sequence = fFrame;
						h->u.raw_video.field_number = 0;
						h->u.raw_video.pulldown_number = 0;
						h->u.raw_video.first_active_line = 1;
						h->u.raw_video.line_count
							= fConnectedFormat.display.line_count;
						// Fill in a frame
						media_format mf;
						mf.type = B_MEDIA_RAW_VIDEO;
						mf.u.raw_video = fConnectedFormat;
ldebug("VideoProducer: frame: %Ld, playlistFrame: %Ld\n", fFrame, playlistFrame);
						bool forceOrWasCached = forceSendingBuffer;
	
//						if (fManager->LockWithTimeout(5000) == B_OK) {
							// we need to lock the manager, or our
							// fSupplier might work on bad data
							err = fSupplier->FillBuffer(playlistFrame,
														buffer->Data(), &mf,
														forceOrWasCached);
//							fManager->Unlock();
//						} else {
//							err = B_ERROR;
//						}
						// clean the buffer if something went wrong
						if (err != B_OK) {
							memset(buffer->Data(), 0, h->size_used);
							err = B_OK;
						}
						// Send the buffer on down to the consumer
						if (!forceOrWasCached) {
							if (SendBuffer(buffer, fOutput.source,
									fOutput.destination) != B_OK) {
								printf("_FrameGenerator: Error sending buffer\n");
								// If there is a problem sending the buffer,
								// or if we don't send the buffer because its
								// contents are the same as the last one,
								// return it to its buffer group.
								buffer->Recycle();
								// we tell the supplier to delete
								// its caches if there was a problem sending
								// the buffer
								fSupplier->DeleteCaches();
							}
						} else
							buffer->Recycle();
						// Only if everything went fine we clear the flag
						// that forces us to send a buffer even if not
						// playing.
						if (err == B_OK) {
							forceSendingBuffer = false;
							lastFrameSentAt = performanceTime;
						}
					}
else ldebug("no buffer!\n");
					// next frame
					fFrame++;
				} else {
ldebug("VideoProducer: not playing\n");
					// next frame
					fFrame++;
				}
				break;
			default:
ldebug("Couldn't acquire semaphore. Error: %s\n", strerror(err));
				running = false;
				break;
		}
	}
ldebug("VideoProducer: frame generator thread done.\n");
	return B_OK;
}

