/*
 * Copyright 2006-2009, Axel Dörfler <axeld@pinc-software.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "MonitorControl.h"

#include "DisplaySettings.h"
#include "ScreenMode.h"

#include <stdio.h>


int32 kMaxID = 25;


// Wrapper class to work-around the apparently broken broadcast mode

bool
SamsungBroadcastControl::On()
{
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		if (fControl.On())
			return true;
	}

	return false;
}


status_t
SamsungBroadcastControl::TurnOn()
{
	status_t status = B_ERROR;
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		status_t oneStatus = fControl.TurnOn();
		if (status < B_OK)
			status = oneStatus;
	}

	return status;
}


status_t
SamsungBroadcastControl::TurnOff()
{
	status_t status = B_ERROR;
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		status_t oneStatus = fControl.TurnOff();
		if (status < B_OK)
			status = oneStatus;
	}

	snooze(200000);
		// wait a bit to give the monitor some time to process any other command
	return status;
}


bool
SamsungBroadcastControl::RemoteEnabled()
{
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		if (fControl.InputSource())
			return true;
	}

	return false;
}


status_t
SamsungBroadcastControl::EnableRemote(bool enable)
{
	status_t status = B_ERROR;
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		status_t oneStatus = fControl.EnableRemote(enable);
		if (status < B_OK)
			status = oneStatus;
	}

	return status;
}


status_t
SamsungBroadcastControl::AutoAdjust()
{
	status_t status = B_ERROR;
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		status_t oneStatus = fControl.AutoAdjust();
		if (status < B_OK)
			status = oneStatus;
	}

	return status;
}


uint8
SamsungBroadcastControl::InputSource()
{
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		uint8 id = fControl.InputSource();
		if (id != 0)
			return id;
	}

	return 0;
}


status_t
SamsungBroadcastControl::SetInputSource(uint32 id)
{
	status_t status = B_ERROR;
	for (int32 i = 1; i < kMaxID; i++) {
		fControl.SetID(i);

		status_t oneStatus = fControl.SetInputSource(id);
		if (status < B_OK)
			status = oneStatus;
	}

	return status;
}


//	#pragma mark -


MonitorControl::MonitorControl()
	:
	fInitialized(false)
{
}


MonitorControl::~MonitorControl()
{
}


status_t
MonitorControl::Init()
{
	return _InitIfNeeded();
}


/* static */ status_t
MonitorControl::_InitControl(SamsungBroadcastControl& control)
{
	BSerialPort serial;
	uint32 count = serial.CountDevices();

	for (uint32 i = 0; i < count; i++) {
		char name[B_FILE_NAME_LENGTH];
		if (serial.GetDeviceName(i, name, sizeof(name)) != B_OK)
			continue;

		if (control.SetTo(name) == B_OK)
			return B_OK;
	}

	return B_ENTRY_NOT_FOUND;
}


status_t
MonitorControl::_InitIfNeeded()
{
	if (fInitialized)
		return B_OK;

	status_t status = _InitControl(fControl);
	if (status < B_OK)
		return status;

	fInitialized = true; 
	return B_OK;
}


status_t
MonitorControl::SetMode(const DisplaySettings& settings)
{
	// construct display mode
	screen_mode mode;
	mode.width = settings.DisplayWidth();
	mode.height = settings.DisplayHeight();
	mode.space = B_RGB32;
	mode.refresh = settings.DisplayFrequency();

	printf("switch resolution: %ld x %ld @ %g Hz\n",
		mode.width, mode.height, mode.refresh);

	ScreenMode screen(NULL);
	return screen.Set(mode);
}


status_t
MonitorControl::SetSource(const DisplaySettings& settings)
{
	status_t status = _InitIfNeeded();
	if (status < B_OK)
		return status;

	uint32 source = settings.InputSource();
	printf("switch input source to %.4s\n", (char*)&source);

	// control monitor input source
	return fControl.SetInputSource(settings.InputSource());
}


bool
MonitorControl::On()
{
	if (_InitIfNeeded() < B_OK)
		return true;

	return fControl.On();
}


status_t
MonitorControl::TurnOn()
{
	status_t status = _InitIfNeeded();
	if (status < B_OK)
		return status;

	return fControl.TurnOn();
}


status_t
MonitorControl::TurnOff()
{
	status_t status = _InitIfNeeded();
	if (status < B_OK)
		return status;

	return fControl.TurnOff();
}



status_t
MonitorControl::EnableRemote(bool enable)
{
	status_t status = _InitIfNeeded();
	if (status < B_OK)
		return status;

	return fControl.EnableRemote(enable);
}


status_t
MonitorControl::AutoAdjust()
{
	status_t status = _InitIfNeeded();
	if (status < B_OK)
		return status;

	return fControl.AutoAdjust();
}

