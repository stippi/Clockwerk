/*	Copyright (c) 1998-99, Be Incorporated, All Rights Reserved.
 *	Distributed under the terms of the Be Sample Code license.
 *
 *	Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>,
 *	Copyright (c) 2000-2008, Stephan Aßmus <superstippi@gmx.de>,
 *	All Rights Reserved. Distributed under the terms of the MIT license.
 */
#ifndef AUDIO_PRODUCER_H
#define AUDIO_PRODUCER_H

#include <BufferProducer.h>
#include <MediaEventLooper.h>

class AudioSupplier;
class BMessenger;

enum {
	MSG_PEAK_NOTIFICATION		= 'pknt'
};

class AudioProducer : public BBufferProducer, public BMediaEventLooper {
public:
								AudioProducer(const char* name,
									AudioSupplier* supplier,
									bool lowLatency = true);
	virtual						~AudioProducer();

	// BMediaNode interface
	virtual	BMediaAddOn*		AddOn(int32 *internal_id) const;

	// BBufferProducer interface
	virtual	status_t			FormatSuggestionRequested(media_type type,
									int32 quality, media_format* _format);

	virtual	status_t			FormatProposal(const media_source& output,
									media_format* format);

	virtual	status_t			FormatChangeRequested(
									const media_source& source,
									const media_destination& destination,
									media_format* ioFormat,
									int32* _deprecated_);

	virtual	status_t			GetNextOutput(int32* cookie,
									media_output* _output);

	virtual	status_t			DisposeOutputCookie(int32 cookie);

	virtual	status_t			SetBufferGroup(const media_source& forSource,
									BBufferGroup* group);

	virtual	status_t			GetLatency(bigtime_t* _latency);

	virtual	status_t			PrepareToConnect(const media_source& what,
									const media_destination& where,
									media_format* format,
									media_source* outSource, char* outName);

	virtual	void				Connect(status_t error,
									const media_source& source,
									const media_destination& destination,
									const media_format& format,
									char* ioName);

	virtual	void				Disconnect(const media_source &what,
									const media_destination& where);

	virtual	void				LateNoticeReceived(const media_source& what,
									bigtime_t howMuch,
									bigtime_t performanceTime);

	virtual	void				EnableOutput(const media_source& what,
									bool enabled, int32* _deprecated_);

	virtual	status_t			SetPlayRate(int32 numer, int32 denom);

	virtual	status_t			HandleMessage(int32 message,
									const void* data, size_t size);

	virtual	void				AdditionalBufferRequested(
									const media_source& source,
									media_buffer_id prevBuffer,
									bigtime_t prevTime,
									const media_seek_tag* prevTag);
										// may be NULL

	virtual	void				LatencyChanged(const media_source& source,
									const media_destination& destination,
									bigtime_t newLatency, uint32 flags);

	// BMediaEventLooper interface
	virtual	void				NodeRegistered();

	virtual	void				SetRunMode(run_mode mode);

	virtual	void				HandleEvent(const media_timed_event* event,
									bigtime_t lateness,
									bool realTimeEvent = false);

			void				SetRunning(bool running);

	// AudioProducer
			void				SetPeakListener(const BMessenger& messenger);

private:
			status_t			_AllocateBuffers(media_format* format);
			BBuffer*			_FillNextBuffer(bigtime_t eventTime);

			void				_FillSampleBuffer(float* data,
									size_t numSamples);

			BBufferGroup*		fBufferGroup;
			bool				fUsingOurBuffers;
			bigtime_t			fLatency;
			bigtime_t			fInternalLatency;
			bool				fLowLatency;
			media_output		fOutput;
			bool				fOutputEnabled;
			media_format		fPreferredFormat;

			uint64				fFramesSent;
			bigtime_t			fStartTime;

			AudioSupplier*		fSupplier;
	volatile bool				fRunning;

			BMessenger*			fPeakListener;
};

#endif // AUDIO_PRODUCER_H
