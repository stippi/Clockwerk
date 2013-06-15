/*
 * Copyright 2006-2009, Axel Dörfler <axeld@pinc-software.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef MONITOR_CONTROL_H
#define MONITOR_CONTROL_H


#include "SamsungMonitorControl.h"

class DisplaySettings;


class SamsungBroadcastControl {
	public:
		SamsungBroadcastControl() {}

		status_t InitCheck()
			{ return fControl.InitCheck(); }
		status_t SetTo(const char* device)
			{ return fControl.SetTo(device); }

		bool On();
		status_t TurnOn();
		status_t TurnOff();

		bool RemoteEnabled();
		status_t EnableRemote(bool enable);

		status_t AutoAdjust();

		uint8 InputSource();
		status_t SetInputSource(uint32 id);

	private:
		SamsungMonitorControl	fControl;
};

class MonitorControl {
	public:
		MonitorControl();
		~MonitorControl();

		status_t Init();

		status_t SetMode(const DisplaySettings& settings);
		status_t SetSource(const DisplaySettings& settings);

		bool On();
		status_t TurnOn();
		status_t TurnOff();

		status_t EnableRemote(bool enable);
		status_t AutoAdjust();

	private:
		static status_t _InitControl(SamsungBroadcastControl& control);
		status_t _InitIfNeeded();

		SamsungBroadcastControl	fControl;
		bool					fInitialized;
};

#endif	// MONITOR_CONTROL_H
