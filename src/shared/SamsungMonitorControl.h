/*
 * Copyright 2006-2009, Axel Dörfler <axeld@pinc-software.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SAMSUNG_MONITOR_CONTROL_H
#define SAMSUNG_MONITOR_CONTROL_H


#include "CommonPropertyIDs.h"

#include <SerialPort.h>


struct samsung_reply;

class SamsungMonitorControl {
	public:
		SamsungMonitorControl(const char* device);
		SamsungMonitorControl();
		~SamsungMonitorControl();

		status_t InitCheck();
		status_t SetTo(const char* device);

		void SetID(uint8 id) { fID = id; }
		uint8 ID() const { return fID; }

		bool On();
		status_t TurnOn();
		status_t TurnOff();

		bool RemoteEnabled();
		status_t EnableRemote(bool enable);

		status_t AutoAdjust();

		uint8 InputSource();
		status_t SetInputSource(uint32 id);

		status_t SendCommand(uint8 command, const uint8* data,
			uint8 length, samsung_reply& reply, uint8& _replySize);
		status_t SendCommand(uint8 command, uint8 id, const uint8* data,
			uint8 length, uint8* reply, uint8& _replySize);

	private:
		BSerialPort	fPort;
		uint8		fID;
		status_t	fInitStatus;
};

#endif	// SAMSUNG_MONITOR_CONTROL_H
