/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include <stdio.h>
#include <string.h>

#include <MediaRoster.h>
#include <scheduler.h>
#include <TimeSource.h>

#include "AudioProducer.h"
#include "AudioSupplier.h"
#include "VideoConsumer.h"
#include "VideoProducer.h"
#include "VideoSupplier.h"

#include "NodeManager.h"

// debugging
#include "Debug.h"
//#define ldebug	debug
#define ldebug	nodebug

#define print_error(str, status) printf(str ", error: %s\n", strerror(status))

NodeManager::Connection::Connection()
	: connected(false)
{
	memset(&format, 0, sizeof(media_format));
}

// constructor
NodeManager::NodeManager()
	: PlaybackManager(),
	  fMediaRoster(NULL),
	  fAudioProducer(NULL),
	  fVideoConsumer(NULL),
	  fVideoProducer(NULL),
	  fTimeSource(media_node::null),
	  fAudioConnection(),
	  fVideoConnection(),
	  fPerformanceTimeBase(0),
	  fStatus(B_NO_INIT),
	  fVCTarget(NULL),
	  fAudioSupplier(NULL),
	  fVideoSupplier(NULL),
	  fMovieBounds(0, 0, -1, -1)
{
}

// destructor
NodeManager::~NodeManager()
{
	_StopNodes();
	_TearDownNodes();
	// fAudioSupplier is deleted when the nodes go down
	delete fVideoSupplier;
}

// Init
status_t
NodeManager::Init(BRect bounds, float fps, int32 loopingMode,
				  bool loopingEnabled, float speed)
{
	// init base class
	PlaybackManager::Init(fps, loopingMode, loopingEnabled, speed);

	// get some objects from a derived class
	if (!fVCTarget)
		fVCTarget = CreateVCTarget();

	if (!fVideoSupplier)
		fVideoSupplier = CreateVideoSupplier();

	return FormatChanged(bounds, fps);
}

// InitCheck
status_t
NodeManager::InitCheck()
{
	return fStatus;
}

// CleanupNodes
status_t
NodeManager::CleanupNodes()
{
	_StopNodes();
	return _TearDownNodes(false);
}

// FormatChanged
status_t
NodeManager::FormatChanged(BRect movieBounds, float framesPerSecond)
{
	if (movieBounds == MovieBounds() && framesPerSecond == FramesPerSecond())
		return B_OK;

	PlaybackManager::Init(framesPerSecond,
						  LoopMode(), IsLoopingEnabled(), Speed(),
						  MODE_PLAYING_PAUSED_FORWARD, CurrentFrame());

	_StopNodes();
	_TearDownNodes();

	SetMovieBounds(movieBounds);

	status_t ret = _SetUpNodes(framesPerSecond);
	if (ret == B_OK)
		_StartNodes();

	return ret;
}

// RealTimeForTime
bigtime_t
NodeManager::RealTimeForTime(bigtime_t time) const
{
	bigtime_t result = 0;
	if (fVideoProducer) {
		result = fVideoProducer->TimeSource()->RealTimeFor(fPerformanceTimeBase + time,
														   0);
	}
	return result;
}

// TimeForRealTime
bigtime_t
NodeManager::TimeForRealTime(bigtime_t time) const
{
	bigtime_t result = 0;
	if (fVideoProducer) {
		result = fVideoProducer->TimeSource()->PerformanceTimeFor(time)
				 - fPerformanceTimeBase;
	}
	return result;
}

// SetMovieBounds
void
NodeManager::SetMovieBounds(BRect bounds)
{
	if (bounds != fMovieBounds) {
		fMovieBounds = bounds;
		TriggerMovieBoundsChanged(fMovieBounds);
	}
}

// MovieBounds
BRect
NodeManager::MovieBounds() const
{
	return fMovieBounds;
}

// SetVCTarget
void
NodeManager::SetVCTarget(VCTarget* vcTarget)
{
	if (vcTarget != fVCTarget) {
		fVCTarget = vcTarget;
		if (fVideoConsumer)
			fVideoConsumer->SetTarget(fVCTarget);
	}
}

// GetVCTarget
VCTarget*
NodeManager::GetVCTarget() const
{
	return fVCTarget;
}

// SetVolume
void
NodeManager::SetVolume(float percent)
{
	// TODO: would be nice to set the volume on the system mixer input of
	// our audio node...
}

// SetPeakListener
void
NodeManager::SetPeakListener(const BMessenger& messenger)
{
	if (fAudioProducer)
		fAudioProducer->SetPeakListener(messenger);
	else {
		fprintf(stderr, "NodeManager::SetPeakListener() - no audio "
			"producer!\n");
	}
}

// #pragma mark -

// _SetUpNodes
status_t
NodeManager::_SetUpNodes(float videoFrameRate)
{
	// find the media roster
	fStatus = B_OK;
	fMediaRoster = BMediaRoster::Roster(&fStatus);
	if (fStatus != B_OK) {
		print_error("Can't find the media roster", fStatus);
		fMediaRoster = NULL;
		return fStatus;
	}
	if (!fMediaRoster->Lock())
		return B_ERROR;

	// find the time source
	fStatus = fMediaRoster->GetTimeSource(&fTimeSource);
	if (fStatus != B_OK) {
		print_error("Can't get a time source", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	// create the video producer node
	fVideoProducer = new VideoProducer(NULL, "Clockwerk Video Out", 0,
									   this, fVideoSupplier);
	
	// register the producer node
	fStatus = fMediaRoster->RegisterNode(fVideoProducer);
	if (fStatus != B_OK) {
		print_error("Can't register the video producer", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	
	// make sure the Media Roster knows that we're using the node
//	fMediaRoster->GetNodeFor(fVideoProducer->Node().node,
//							 &fVideoConnection.producer);
	fVideoConnection.producer = fVideoProducer->Node();

	// create the video consumer node
	fVideoConsumer = new VideoConsumer("Clockwerk Video In", NULL, 0, this, fVCTarget);
	
	// register the consumer node
	fStatus = fMediaRoster->RegisterNode(fVideoConsumer);
	if (fStatus != B_OK) {
		print_error("Can't register the video consumer", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}

	// make sure the Media Roster knows that we're using the node
//	fMediaRoster->GetNodeFor(fVideoConsumer->Node().node,
//							 &fVideoConnection.consumer);
	fVideoConnection.consumer = fVideoConsumer->Node();
	
	// find free producer output
	media_input videoInput;
	media_output videoOutput;
	int32 count = 1;
	fStatus = fMediaRoster->GetFreeOutputsFor(fVideoConnection.producer, &videoOutput, 1,
											 &count, B_MEDIA_RAW_VIDEO);
	if (fStatus != B_OK || count < 1) {
		fStatus = B_RESOURCE_UNAVAILABLE;
		print_error("Can't find an available video stream", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}

	// find free consumer input
	count = 1;
	fStatus = fMediaRoster->GetFreeInputsFor(fVideoConnection.consumer, &videoInput, 1,
											&count, B_MEDIA_RAW_VIDEO);
	if (fStatus != B_OK || count < 1) {
		fStatus = B_RESOURCE_UNAVAILABLE;
		print_error("Can't find an available connection to the video window", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}

	// Connect The Nodes!!!
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	media_raw_video_format videoFormat = {
		FramesPerSecond(), 1, 0,
		fMovieBounds.IntegerWidth(),
		B_VIDEO_TOP_LEFT_RIGHT, 1, 1,
		{
			B_YCbCr422,
			fMovieBounds.IntegerWidth() + 1,
			fMovieBounds.IntegerHeight() + 1,
			0, 0, 0
		}
	};
	format.u.raw_video = videoFormat;
	
	// connect producer to consumer
	fStatus = fMediaRoster->Connect(videoOutput.source, videoInput.destination,
								   &format, &videoOutput, &videoInput);

	if (fStatus != B_OK) {
		print_error("Can't connect the video source to the video window... "
					"trying B_RGB32", fStatus);
		format.u.raw_video.display.format = B_RGB32;
		// connect producer to consumer
		fStatus = fMediaRoster->Connect(videoOutput.source, videoInput.destination,
									   &format, &videoOutput, &videoInput);
	}
	// bail if second attempt failed too
	if (fStatus != B_OK) {
		print_error("Can't connect the video source to the video window", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	
	// the inputs and outputs might have been reassigned during the
	// nodes' negotiation of the Connect().  That's why we wait until
	// after Connect() finishes to save their contents.
	fVideoConnection.format = format;
	fVideoConnection.source = videoOutput.source;
	fVideoConnection.destination = videoInput.destination;
	fVideoConnection.connected = true;

	// set time sources
	fStatus = fMediaRoster->SetTimeSourceFor(fVideoConnection.producer.node, fTimeSource.node);
	if (fStatus != B_OK) {
		print_error("Can't set the timesource for the video source", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	
	fStatus = fMediaRoster->SetTimeSourceFor(fVideoConsumer->ID(), fTimeSource.node);
	if (fStatus != B_OK) {
		print_error("Can't set the timesource for the video window", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	
	// the fAudioProducer connection

	// NOTE: the supplier is created here because it needs to
	// know the video frame rate. The supplier is deleted in
	// _TearDownNodes()
	fAudioSupplier = CreateAudioSupplier();

	fAudioProducer = new AudioProducer("Clockwerk Audio Out", fAudioSupplier);
	fStatus = fMediaRoster->RegisterNode(fAudioProducer);
	if (fStatus != B_OK) {
		print_error("unable to register audio producer node!\n", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	// make sure the Media Roster knows that we're using the node
//	fMediaRoster->GetNodeFor(fAudioProducer->Node().node,
//							 &fAudioConnection.producer);
	fAudioConnection.producer = fAudioProducer->Node();

	// connect to the mixer
	fStatus = fMediaRoster->GetAudioMixer(&fAudioConnection.consumer);
	if (fStatus != B_OK) {
		print_error("unable to get the system mixer", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}

	fMediaRoster->SetTimeSourceFor(fAudioConnection.producer.node, fTimeSource.node);

	// got the nodes; now we find the endpoints of the connection
	media_input mixerInput;
	media_output soundOutput;
	count = 1;
	fStatus = fMediaRoster->GetFreeOutputsFor(fAudioConnection.producer, &soundOutput, 1, &count);
	if (fStatus != B_OK) {
		print_error("unable to get a free output from the producer node", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}
	count = 1;
	fStatus = fMediaRoster->GetFreeInputsFor(fAudioConnection.consumer, &mixerInput, 1, &count);
	if (fStatus != B_OK) {
		print_error("unable to get a free input to the mixer", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}

	// got the endpoints; now we connect it!
	media_format audio_format;
	audio_format.type = B_MEDIA_RAW_AUDIO;	
	audio_format.u.raw_audio = media_raw_audio_format::wildcard;
	fStatus = fMediaRoster->Connect(soundOutput.source, mixerInput.destination,
									&audio_format, &soundOutput, &mixerInput);
	if (fStatus != B_OK) {
		print_error("unable to connect audio nodes", fStatus);
		fMediaRoster->Unlock();
		return fStatus;
	}

	// the inputs and outputs might have been reassigned during the
	// nodes' negotiation of the Connect().  That's why we wait until
	// after Connect() finishes to save their contents.
	fAudioConnection.format = audio_format;
	fAudioConnection.source = soundOutput.source;
	fAudioConnection.destination = mixerInput.destination;
	fAudioConnection.connected = true;

	// Set an appropriate run mode for the producer
	fMediaRoster->SetRunModeNode(fAudioConnection.producer, BMediaNode::B_INCREASE_LATENCY);

	// we're done mocking with the media roster
	fMediaRoster->Unlock();

	return fStatus;
}

// _TearDownNodes
status_t
NodeManager::_TearDownNodes(bool disconnect)
{
ldebug("NodeManager::_TearDownNodes()\n");
	status_t err = B_OK;
	fMediaRoster = BMediaRoster::Roster(&err);
	if (err != B_OK) {
		fprintf(stderr, "NodeManager::_TearDownNodes() - error getting media roster: %s\n", strerror(err));
		fMediaRoster = NULL;
	}
	// begin mucking with the media roster
	bool mediaRosterLocked = false;
	if (fMediaRoster && fMediaRoster->Lock())
		mediaRosterLocked = true;

	if (fVideoConsumer && fVideoProducer && fVideoConnection.connected) {
		// disconnect
		if (fMediaRoster) {
ldebug("  disconnecting video...\n");
			err = fMediaRoster->Disconnect(fVideoConnection.producer.node,
										   fVideoConnection.source,
										   fVideoConnection.consumer.node,
										   fVideoConnection.destination);
			if (err < B_OK)
				print_error("unable to disconnect video nodes", err);
		} else {
			fprintf(stderr, "NodeManager::_TearDownNodes() - cannot disconnect video nodes, no media server!\n");
		}
		fVideoConnection.connected = false;
	}
	if (fVideoProducer) {
ldebug("  releasing video producer...\n");
		fVideoProducer->Release();
		fVideoProducer = NULL;
	}
	if (fVideoConsumer) {
ldebug("  releasing video consumer...\n");
		fVideoConsumer->Release();
		fVideoConsumer = NULL;
	}
	if (fAudioProducer) {
		disconnect = fAudioConnection.connected;
		// Ordinarily we'd stop *all* of the nodes in the chain at this point.  However,
		// one of the nodes is the System Mixer, and stopping the Mixer is a Bad Idea (tm).
		// So, we just disconnect from it, and release our references to the nodes that
		// we're using.  We *are* supposed to do that even for global nodes like the Mixer.
		if (fMediaRoster && disconnect) {
ldebug("  disconnecting audio...\n");
			err = fMediaRoster->Disconnect(fAudioConnection.producer.node,
										   fAudioConnection.source,
										   fAudioConnection.consumer.node,
										   fAudioConnection.destination);
			if (err < B_OK) {
				print_error("unable to disconnect audio nodes", err);
				disconnect = false;
			}
		} else {
			fprintf(stderr, "NodeManager::_TearDownNodes() - cannot disconnect audio nodes, no media server!\n");
		}

ldebug("  releasing audio producer...\n");
		fAudioProducer->Release();
		fAudioProducer = NULL;
		fAudioConnection.connected = false;

		if (fMediaRoster && disconnect) {
ldebug("  releasing audio consumer...\n");
			fMediaRoster->ReleaseNode(fAudioConnection.consumer);
		} else {
			fprintf(stderr, "NodeManager::_TearDownNodes() - cannot release audio consumer (system mixer)!\n");
		}
		snooze(20000LL);
		// delete audio supplier
		delete fAudioSupplier;
		fAudioSupplier = NULL;
	}
	// we're done mucking with the media roster
	if (mediaRosterLocked && fMediaRoster)
		fMediaRoster->Unlock();
ldebug("NodeManager::_TearDownNodes() done\n");
	return err;
}

// _StartNodes
status_t
NodeManager::_StartNodes()
{
	status_t status = B_NO_INIT;
	if (!fMediaRoster || !fVideoProducer || !fVideoConsumer || !fAudioProducer)
		return status;
	// begin mucking with the media roster
	if (fMediaRoster->Lock()) {
		// figure out what recording delay to use
		bigtime_t latency = 0;
		status = fMediaRoster->GetLatencyFor(fVideoConnection.producer, &latency);
		if (status < B_OK) {
			print_error("error getting latency for video producer", status);	
		}
else ldebug("video latency: %Ld\n", latency);
		status = fMediaRoster->SetProducerRunModeDelay(fVideoConnection.producer, latency);
		if (status < B_OK) {
			print_error("error settings run mode delay for video producer", status);	
		}
	
		// start the nodes
		bigtime_t initLatency = 0;
		status = fMediaRoster->GetInitialLatencyFor(fVideoConnection.producer, &initLatency);
		if (status < B_OK) {
			print_error("error getting initial latency for video producer", status);	
		}
		initLatency += estimate_max_scheduling_latency();
	
		bigtime_t audio_latency = 0;
		status = fMediaRoster->GetLatencyFor(fAudioConnection.producer, &audio_latency);
ldebug("audio latency: %Ld\n", audio_latency);
		
		BTimeSource *timeSource = fMediaRoster->MakeTimeSourceFor(fVideoConnection.producer);
		bool running = timeSource->IsRunning();
		
		// workaround for people without sound cards
		// because the system time source won't be running
		bigtime_t real = BTimeSource::RealTime();
		if (!running) {
			status = fMediaRoster->StartTimeSource(fTimeSource, real);
			if (status != B_OK) {
				timeSource->Release();
				print_error("cannot start time source!", status);
				return status;
			}
			status = fMediaRoster->SeekTimeSource(fTimeSource, 0, real);
			if (status != B_OK) {
				timeSource->Release();
				print_error("cannot seek time source!", status);
				return status;
			}
		}
	
		bigtime_t perf = timeSource->PerformanceTimeFor(real + latency + initLatency);
		
		timeSource->Release();
	
		// start the nodes
		status = fMediaRoster->StartNode(fVideoConnection.consumer, perf);
		if (status != B_OK) {
			print_error("Can't start the video consumer", status);
			return status;
		}
		status = fMediaRoster->StartNode(fVideoConnection.producer, perf);
		if (status != B_OK) {
			print_error("Can't start the video producer", status);
			return status;
		}
	
		fAudioProducer->SetRunning(true);
		status = fMediaRoster->StartNode(fAudioConnection.producer, perf);
		if (status != B_OK) {
			print_error("Can't start the audio producer", status);
			return status;
		}
		fPerformanceTimeBase = perf;
		// done mucking with the media roster
		fMediaRoster->Unlock();
	}
	return status;
}

// _StopNodes
void
NodeManager::_StopNodes()
{
ldebug("NodeManager::_StopNodes()\n");
//	if (PlayMode() == MODE_PLAYING_PAUSED_FORWARD
//		|| PlayMode() == MODE_PLAYING_PAUSED_FORWARD)
//		return;
	fMediaRoster = BMediaRoster::Roster();
	if (!fMediaRoster) {
		if (fAudioProducer)
			fAudioProducer->SetRunning(false);
		return;
	} else if (fMediaRoster->Lock()) {
		// begin mucking with the media roster
		if (fVideoProducer != NULL) {
			ldebug("  stopping video producer...\n");
			fMediaRoster->StopNode(fVideoConnection.producer, 0, true);
		}
		if (fAudioProducer != NULL) {
			ldebug("  stopping audio producer...\n");
			fMediaRoster->StopNode(fAudioConnection.producer, 0, true);
				// synchronous stop
		}
		if (fVideoConsumer != NULL) {
			ldebug("  stopping video consumer...\n");
			fMediaRoster->StopNode(fVideoConnection.consumer, 0, true);
		}
		ldebug("  all nodes stopped\n");
		// done mucking with the media roster
		fMediaRoster->Unlock();
	}
ldebug("NodeManager::_StopNodes() done\n");
}

