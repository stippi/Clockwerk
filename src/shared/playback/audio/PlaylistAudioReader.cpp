/*
 * Copyright (c) 2000-2008, Ingo Weinhold <ingo_weinhold@gmx.de>.
 * All Rights Reserved. Distributed under the terms of the MIT license.
 */
#include "PlaylistAudioReader.h"

#include <algorithm>
#include <stdio.h>

#include <MediaTrack.h>

#include "AudioAdapter.h"
#include "AudioMixer.h"
#include "AutoLocker.h"
#include "ClipPlaylistItem.h"
#include "Observer.h"
#include "Playlist.h"
#include "RWLocker.h"

// debugging
#include "Debug.h"
//#define ldebug debug
#define ldebug nodebug


#define MAX_RECURSION_LEVEL 8

// SoundItem

class SoundItem : public Observer {
 public:
	SoundItem(PlaylistItem* item, int64 offset);
	virtual ~SoundItem();

	virtual void ObjectChanged(const Observable* object);
	virtual void ObjectDeleted(const Observable* object);

	inline bool operator==(const SoundItem& other) const
	{
		return (item == other.item && offset == other.offset);
	}

	inline bool operator<(const SoundItem& other) const
	{
		return ((uint32)item < (uint32)other.item
			|| ((uint32)item == (uint32)other.item && offset < other.offset));
	}

	inline bool operator>(const SoundItem& other) const
	{
		return other < *this;
	}

	PlaylistItem*		item;
	int64				offset;
};


SoundItem::SoundItem(PlaylistItem* item, int64 offset)
	: item(item), offset(offset)
{
	if (item)
		item->AddObserver(this);
}

SoundItem::~SoundItem()
{
	if (item)
		item->RemoveObserver(this);
}


void
SoundItem::ObjectChanged(const Observable* object)
{
}

void
SoundItem::ObjectDeleted(const Observable* object)
{
printf("SoundItem::ObjectDeleted()\n");
	if (item)
		item->RemoveObserver(this);
	item = NULL;
}

int
compare_sound_items(const void* item1, const void* item2)
{
	const SoundItem* soundItem1 = *(const SoundItem**)item1;
	const SoundItem* soundItem2 = *(const SoundItem**)item2;
	if (*soundItem1 < *soundItem2)
		return -1;
	if (*soundItem1 > *soundItem2)
		return 1;
	return 0;
}


// PlaylistAudioReader

// constructor
PlaylistAudioReader::PlaylistAudioReader(Playlist* playlist,
	RWLocker* locker, const media_format& format, float videoFrameRate)
	: AudioReader(format),
	  fPlaylist(playlist),
	  fLocker(locker),
	  fVideoFrameRate(videoFrameRate),
	  fSoundItems(10),
	  fAudioReaders(10),
	  fAdapter(NULL),
	  fMixer(NULL)
{
	uint32 hostByteOrder
		= (B_HOST_IS_BENDIAN) ? B_MEDIA_BIG_ENDIAN : B_MEDIA_LITTLE_ENDIAN;
	if (fFormat.type == B_MEDIA_RAW_AUDIO
		&& fFormat.u.raw_audio.byte_order == hostByteOrder) {
		// The mixer adjusts the format according to its needs. Thus we need an
		// adapter to ensure that we output the desired format.
		fMixer = new AudioMixer(format);
		fAdapter = new AudioAdapter(fMixer, format);
	}
}

// destructor
PlaylistAudioReader::~PlaylistAudioReader()
{
	delete fAdapter;
	// empty the audio mixer's sources list (delete the adapters)
	if (fMixer) {
		while (AudioReader* reader = fMixer->RemoveSource(0))
			delete reader;
		delete fMixer;
	}
	// delete SoundItems
	for (int32 i = 0;
		 SoundItem* item = (SoundItem*)fSoundItems.ItemAt(i);
		 i++) {
		item->Release();
	}
	// delete AudioReaders
	for (int32 i = 0;
		 AudioReader* reader = (AudioReader*)fAudioReaders.ItemAt(i);
		 i++) {
		delete reader;
	}
}

// Read
status_t
PlaylistAudioReader::Read(void* buffer, int64 pos, int64 frames)
{
ldebug("PlaylistAudioReader::Read(%p, %Ld, %Ld)\n", buffer, pos, frames);
	status_t error = InitCheck();
	if (error != B_OK)
{
ldebug("PlaylistAudioReader::Read() done\n");
		return error;
}
//############### TODO: way too much SoundItem construction/deletion


	pos += fOutOffset;
	int64 sampleSize = fFormat.u.raw_audio.format 
		& media_raw_audio_format::B_AUDIO_SIZE_MASK;
	int64 frameSize = sampleSize * fFormat.u.raw_audio.channel_count;
	// read lock the document
	AutoReadLocker locker(fLocker);
	if (fLocker && !locker.IsLocked()) {
		ldebug("PlaylistAudioReader: locking the RWLocker failed\n");
		return B_ERROR;
	}
	// get the video frame at which we have to start
	int64 videoFrame = VideoFrameForAudioFrame(pos);
	while (frames > 0) {
		int64 nextAudioFrame = AudioFrameForVideoFrame(videoFrame + 1);
		int64 framesToRead = std::min(frames, nextAudioFrame - pos);
ldebug("videoFrame: %Ld, nextAudioFrame: %Ld, framesToRead: %Ld\n",
videoFrame, nextAudioFrame, framesToRead);
		// We try to avoid creating and deleting the AudioTrackReaders
		// each video frame. Therefore we maintain a list of SoundItems
		// and a list of AudioTrackReaders (with corresponding indexes).
		// Each video frame we create a new SoundItem list and compare
		// it to the old one to find out which of the AudioTrackReaders
		// can be reused.
		// empty the audio mixer's sources list (delete the adapters)
		while (AudioReader* reader = fMixer->RemoveSource(0))
			delete reader;
		// get the Playlist's sound items at the current video frame
		BList oldSoundItems(fSoundItems);
		BList oldAudioReaders(fAudioReaders);
		fSoundItems.MakeEmpty();
		fAudioReaders.MakeEmpty();

		// NOTE: maybe copy playlist in case this takes too long
		if (fPlaylist)
			_GetActiveItemsAtFrame(fPlaylist, videoFrame, 0, 0);

		// sort the new list
		fSoundItems.SortItems(&compare_sound_items);
		// compare the new list with the old one
		for (int32 i = 0, k = 0; ; ) {
		 	SoundItem* oldItem = (SoundItem*)oldSoundItems.ItemAt(i);
		 	SoundItem* newItem = (SoundItem*)fSoundItems.ItemAt(k);
		 	if (!oldItem && !newItem)
		 		break;
			if (!newItem || (oldItem && *oldItem < *newItem)) {
				// dispose old reader
				AudioReader* reader = (AudioReader*)oldAudioReaders.ItemAt(i);
ldebug("  dispose old reader: %p\n", reader);
				delete reader;
				oldItem->Release();
				i++;
			} else if (!oldItem || (newItem && *oldItem > *newItem)) {
				// create new reader
				AudioReader* reader = newItem->item->CreateAudioReader();
				if (reader) {
ldebug("  create new reader: %p (init check: %ld)\n", reader,
reader->InitCheck());
					reader->SetOutOffset(reader->FrameForTime(
						TimeForFrame(-newItem->offset)));
					fAudioReaders.AddItem(reader);
				}
				k++;
			} else {
				// reuse the old reader
ldebug("  reuse old reader: %p\n", oldAudioReaders.ItemAt(i));
				fAudioReaders.AddItem(oldAudioReaders.ItemAt(i));
				oldItem->Release();
				i++; k++;
			}
		}
		// add the readers to the mixer's sources
		for (int32 i = 0;
			 AudioReader* reader = (AudioReader*)fAudioReaders.ItemAt(i);
			 i++) {
			fMixer->AddSource(new AudioAdapter(reader, fMixer->Format()));
		}
		// finally read from the mixer
ldebug("  fAdapter->Read(%p, %Ld, %Ld)\n", buffer, pos, framesToRead);
		fAdapter->Read(buffer, pos, framesToRead);

		buffer = (char*)buffer + framesToRead * frameSize;
		pos += framesToRead;
		frames -= framesToRead;
		videoFrame++;
	}
ldebug("PlaylistAudioReader::Read() done\n");
	return B_OK;
}

// InitCheck
status_t
PlaylistAudioReader::InitCheck() const
{
	status_t error = AudioReader::InitCheck();
	if (error == B_OK && (!fMixer || !fAdapter))
		error = B_NO_INIT;
	if (error == B_OK)
		error = fAdapter->InitCheck();
	return error;
}

// SetPlaylist
void
PlaylistAudioReader::SetPlaylist(Playlist* playlist)
{
	fPlaylist = playlist;
	// TODO: maybe empty out any previous stuff?
}

// Source
Playlist*
PlaylistAudioReader::Source() const
{
	return fPlaylist;
}

// AudioFrameForVideoFrame
int64
PlaylistAudioReader::AudioFrameForVideoFrame(int64 frame) const
{
	return (int64)((double)frame * (double)fFormat.u.raw_audio.frame_rate
		/ (double)fVideoFrameRate);
}

// VideoFrameForAudioFrame
int64
PlaylistAudioReader::VideoFrameForAudioFrame(int64 frame) const
{
	return (int64)((double)frame * (double)fVideoFrameRate
		/ (double)fFormat.u.raw_audio.frame_rate);
}

// SetVolume
void
PlaylistAudioReader::SetVolume(float percent)
{
	fMixer->SetVolume(percent);
}

// _GetActiveItemsAtFrame
void
PlaylistAudioReader::_GetActiveItemsAtFrame(Playlist* playlist,
	int64 videoFrame, int64 levelZeroOffset, int32 recursionLevel)
{
	if (recursionLevel > MAX_RECURSION_LEVEL)
		return;

	int32 count = playlist->CountItems();
	for (int32 i = 0; i < count; i++) {
		ClipPlaylistItem* item
			= dynamic_cast<ClipPlaylistItem*>(playlist->ItemAtFast(i));
		if (!item || !item->Clip()
			|| item->StartFrame() > videoFrame
			|| item->EndFrame() < videoFrame
			|| !playlist->IsTrackEnabled(item->Track())
			|| item->IsAudioMuted())
			continue;
		
		Clip* clip = item->Clip();
		clip->Acquire();

		int64 startFrameWithOffset = item->StartFrame() - item->ClipOffset();
		if (Playlist* subPlaylist = dynamic_cast<Playlist*>(clip)) {
			// recurse into sub playlist
			int64 subVideoFrame = videoFrame - startFrameWithOffset;
			int64 subLevelOffset = levelZeroOffset + startFrameWithOffset;
			_GetActiveItemsAtFrame(subPlaylist, subVideoFrame, subLevelOffset,
				recursionLevel + 1);
		} else if (item->HasAudio()) {
			// add a sound item for this clip
			int64 offset = AudioFrameForVideoFrame(
				startFrameWithOffset + levelZeroOffset);
			SoundItem* soundItem = new (std::nothrow) SoundItem(item, offset);
			if (soundItem)
				fSoundItems.AddItem(soundItem);
		}

		clip->Release();
	}
}
