/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2006, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

// This class controls our media nodes and general playback

#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include <MediaNode.h>

#include "PlaybackManager.h"

class AudioProducer;
class BMediaRoster;
class BMessenger;
class VCTarget;
class VideoProducer;
class VideoConsumer;
class AudioSupplier;
class VideoSupplier;

class NodeManager : public PlaybackManager {
 public:
								NodeManager();
	virtual						~NodeManager();

	// must be implemented in derived classes
	virtual	VCTarget*			CreateVCTarget() = 0;
	virtual	VideoSupplier*		CreateVideoSupplier() = 0;
	virtual	AudioSupplier*		CreateAudioSupplier() = 0;

	// NodeManager
			status_t			Init(BRect bounds, float fps,
									 int32 loopingMode = LOOPING_RANGE,
									 bool loopingEnabled = true,
									 float speed = 1.0);
			status_t			InitCheck();
								// only call this if the
								// media_server has died!
			status_t			CleanupNodes();

			status_t			FormatChanged(BRect movieBounds,
											  float framesPerSecond);

	virtual	bigtime_t			RealTimeForTime(bigtime_t time) const;
	virtual	bigtime_t			TimeForRealTime(bigtime_t time) const;

			void				SetMovieBounds(BRect bounds);
	virtual	BRect				MovieBounds() const;

			void				SetVCTarget(VCTarget* vcTarget);
			VCTarget*			GetVCTarget() const;

	virtual	void				SetVolume(float percent);

			void				SetPeakListener(const BMessenger& messenger);

 private:
			status_t			_SetUpNodes(float videoFrameRate);
			status_t			_TearDownNodes(bool disconnect = true);
			status_t			_StartNodes();
			void				_StopNodes();

 private:
	struct Connection {
			Connection();

			media_node			producer;
			media_node			consumer;
			media_source		source;
			media_destination	destination;
			media_format		format;
			bool				connected;
	};

private:
			BMediaRoster*		fMediaRoster;

			// media nodes
			AudioProducer*		fAudioProducer;
			VideoConsumer*		fVideoConsumer;
			VideoProducer*		fVideoProducer;
			media_node			fTimeSource;

			Connection			fAudioConnection;
			Connection			fVideoConnection;

			bigtime_t			fPerformanceTimeBase;

			status_t			fStatus;
			// 
			VCTarget*			fVCTarget;
			AudioSupplier*		fAudioSupplier;
			VideoSupplier*		fVideoSupplier;
			BRect				fMovieBounds;
};


#endif	// NODE_MANAGER_H
