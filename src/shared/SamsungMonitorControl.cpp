/*
 * Copyright 2006-2009, Axel Dörfler <axeld@pinc-software.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "SamsungMonitorControl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct tag {
	uint8		id;
	const char*	name;
};

#define BROADCAST_ID	0xfe

enum samsung_commands {
	COMMAND_HEADER	= 0xaa,
	COMMAND_REPLY	= 0xff,

	MODEL_NUMBER	= 0x10,
	POWER_CONTROL	= 0x11,
	REMOTE_CONTROL	= 0x36,
	AUTO_ADJUST		= 0x3d,
	INPUT_SOURCE_CONTROL = 0x14,
};

static const tag kSpecies[] = {
	{0x01, "PLP"},
	{0x02, "LCD"},
	{0x03, "DLP"},
	{0, NULL},
};

static const tag kModels[] = {
	{0x01, "PPM50H2"},
	{0x02, "PPM42S2"},
	{0x03, "PS-42P2ST"},
	{0x04, "PS-50P2HT"},
	{0x05, "SyncMaster 400T"},
	{0x06, "SyncMaster 403T"},
	{0x07, "PPM42S3, SPD-42P3SM"},
	{0x08, "PPM50H3, SPD-50P3HM"},
	{0x09, "PPM63H3, SPD-63P3HM"},
	{0x0a, "PS-42P3ST"},
	{0x0b, "SyncMaster 323T"},
	{0x0c, "SyncMaster 403T - CT40CS(N)"},
	{0x0d, "PPMxxM5x"},
	{0x0e, "SyncMaster 460P"},
	{0x10, "SyncMaster 400PX"},
	{0, NULL},
};

static const tag kInputSources[] = {
	{0x04, "S-Video"},
	{0x08, "Component"},
	{0x0c, "AV"},
	{0x14, "PC"},
	{0x18, "DVI"},
	{0x1e, "BNC"},
	{0, NULL},
};

struct samsung_command {
	samsung_command(uint8 command, uint8 id, const uint8* data, uint8 length);
	samsung_command();

	bool IsValid();
	bool IsValidReplyHeader();

	uint8 ComputeChecksum();
	void UpdateChecksum();

	uint8& Checksum()
		{ return data[length]; }

	status_t Send(BSerialPort& port);
	status_t Receive(BSerialPort& port);

	bool Ack()
		{ return data[0] == 'A'; }
	uint8 ReplyCommand()
		{ return data[1]; }
	size_t HeaderLength()
		{ return 4; }
	size_t CommandLength()
		{ return HeaderLength() + 1 + length; }

	uint8	header;
	uint8	command;
	uint8	id;
	uint8	length;
	uint8	data[256];
} _PACKED;

struct samsung_reply {
	bool Ack() { return ack == 'A'; }

	uint8	ack;
	uint8	command;
	uint8	data[254];
} _PACKED;


#if 0
static const char*
find_tag(const tag *tags, uint8 id)
{
	for (uint32 i = 0; tags[i].name; i++) {
		if (tags[i].id == id)
			return tags[i].name;
	}

	return "Unknown";
}
#endif


//	#pragma mark -


samsung_command::samsung_command(uint8 _command, uint8 _id,
		const uint8* _data, uint8 _length)
	:
	header(COMMAND_HEADER),
	command(_command),
	id(_id == 0 ? 0xff : _id),
	length(_length)
{
	if (length)
		memcpy(data, _data, length);

	UpdateChecksum();
}


samsung_command::samsung_command()
	:
	header(COMMAND_HEADER)
{
}


bool
samsung_command::IsValid()
{
	return header == COMMAND_HEADER
		&& ComputeChecksum() == Checksum();
}


bool
samsung_command::IsValidReplyHeader()
{
	return header == COMMAND_HEADER
		&& command == COMMAND_REPLY;
}


uint8
samsung_command::ComputeChecksum()
{
	uint32 sum = (uint32)command + (uint32)id + (uint32)length;

	for (uint32 i = 0; i < length; i++) {
		sum += (uint32)data[i];
	}

	return sum & 0xff;
}


void
samsung_command::UpdateChecksum()
{
	Checksum() = ComputeChecksum();
}


status_t
samsung_command::Send(BSerialPort& port)
{
	ssize_t bytesWritten = port.Write(this, CommandLength());
//uint8* a = (uint8*)this;
//printf("SEND %ld, %02x:%02x:%02x:%02x:%02x:%02x\n", bytesWritten, a[0], a[1], a[2], a[3], a[4], a[5]);
	if (bytesWritten == (ssize_t)CommandLength())
		return B_OK;
	if (bytesWritten < B_OK)
		return bytesWritten;

	return B_ERROR;
}


status_t
samsung_command::Receive(BSerialPort& port)
{
	ssize_t bytesRead = port.Read(this, HeaderLength());
//uint8* a = (uint8*)this;
//printf("RECEIVED %ld, %02x:%02x:%02x:%02x:%02x:%02x\n", bytesRead, a[0], a[1], a[2], a[3], a[4], a[5]);
	if (bytesRead != (ssize_t)HeaderLength()) {
		if (bytesRead < B_OK)
			return bytesRead;

		return B_ERROR;
	}

	if (!IsValidReplyHeader())
		return B_ERROR; //B_BAD_DATA;

	ssize_t bytesLeft = CommandLength() - HeaderLength();
	uint8* target = data;
	while (bytesLeft > 0) {
		bytesRead = port.Read(target, bytesLeft);
//a = (uint8*)target;
//printf("RECEIVED %ld, %02x:%02x:%02x:%02x:%02x:%02x\n", bytesRead, a[0], a[1], a[2], a[3], a[4], a[5]);
		if (bytesRead == bytesLeft)
			break;

		if (bytesRead < B_OK)
			return bytesRead;

		bytesLeft -= bytesRead;
		target += bytesRead;
	}

	return B_OK;
}


//	#pragma mark -


SamsungMonitorControl::SamsungMonitorControl(const char* device)
	:
	fID(BROADCAST_ID),
	fInitStatus(B_NO_INIT)
{
	SetTo(device);
}


SamsungMonitorControl::SamsungMonitorControl()
	:
	fID(BROADCAST_ID),
	fInitStatus(B_NO_INIT)
{
}


SamsungMonitorControl::~SamsungMonitorControl()
{
}


status_t
SamsungMonitorControl::InitCheck()
{
	return fInitStatus;
}


status_t
SamsungMonitorControl::SetTo(const char* device)
{
	fPort.SetFlowControl(0);

	fInitStatus = fPort.Open(device);
	if (fInitStatus < B_OK)
		return fInitStatus;

	fPort.SetTimeout(500000);
	fPort.SetDataBits(B_DATA_BITS_8);
	fPort.SetParityMode(B_NO_PARITY);
	fPort.SetStopBits(B_STOP_BITS_1);
	fPort.SetDataRate(B_9600_BPS);

#if 0
	// try to detect the monitor

	samsung_reply reply;
	uint8 replyLength;
	fInitStatus = SendCommand(MODEL_NUMBER, NULL, 0,
		reply, replyLength);
	if (fInitStatus < B_OK)
		return fInitStatus;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return B_ERROR;
	}

	printf("Found: Species %s, Model %s\n", find_tag(kSpecies, reply.data[0]),
		find_tag(kModels, reply.data[1]));
#endif
	return B_OK;
}


status_t
SamsungMonitorControl::SendCommand(uint8 command, const uint8* data,
	uint8 length, samsung_reply& reply, uint8& _replySize)
{
	return SendCommand(command, ID(), data, length, (uint8*)&reply, _replySize);
}


status_t
SamsungMonitorControl::SendCommand(uint8 command, uint8 id, const uint8* data,
	uint8 length, uint8* replyBuffer, uint8& _replySize)
{
	samsung_command request(command, id, data, length);
	samsung_command reply;

	status_t status = request.Send(fPort);
	if (status == B_OK) {
		// read reply, but ignore replies to other commands (or from other monitors)
		do {
			status = reply.Receive(fPort);
		} while (status == B_OK
			&& (reply.ReplyCommand() != command
				|| (id != BROADCAST_ID && reply.id != id)));
	}
	if (status == B_OK) {
		if (!reply.IsValid())
			return B_ERROR;

		_replySize = reply.length;
		memcpy(replyBuffer, reply.data, reply.length);
	}

	return status;
}


bool
SamsungMonitorControl::On()
{
	samsung_reply reply;
	uint8 replyLength;
	status_t status = SendCommand(POWER_CONTROL, NULL, 0, reply, replyLength);
	if (status < B_OK)
		return false;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return false;
	}

	return reply.data[0] != 0;
}


status_t
SamsungMonitorControl::TurnOn()
{
	samsung_reply reply;
	uint8 replyLength;
	uint8 power = 1;
	status_t status = SendCommand(POWER_CONTROL, &power,
		sizeof(power), reply, replyLength);
	if (status < B_OK)
		return status;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return B_ERROR;
	}

	return reply.data[0] != 0 ? B_OK : B_ERROR;
}


status_t
SamsungMonitorControl::TurnOff()
{
	samsung_reply reply;
	uint8 replyLength;
	uint8 power = 0;
	status_t status = SendCommand(POWER_CONTROL, &power,
		sizeof(power), reply, replyLength);
	if (status < B_OK)
		return status;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return B_ERROR;
	}

	return reply.data[0] == 0 ? B_OK : B_ERROR;
}


bool
SamsungMonitorControl::RemoteEnabled()
{
	samsung_reply reply;
	uint8 replyLength;
	status_t status = SendCommand(REMOTE_CONTROL, NULL, 0, reply, replyLength);
	if (status < B_OK)
		return false;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return false;
	}

	return reply.data[0] != 0;
}


status_t
SamsungMonitorControl::EnableRemote(bool enable)
{
	samsung_reply reply;
	uint8 replyLength;
	uint8 enabled = enable;
	status_t status = SendCommand(REMOTE_CONTROL, &enabled,
		sizeof(enabled), reply, replyLength);
	if (status < B_OK)
		return status;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return B_ERROR;
	}

	return reply.data[0] == enable ? B_OK : B_ERROR;
}


status_t
SamsungMonitorControl::AutoAdjust()
{
	samsung_reply reply;
	uint8 replyLength;
	uint8 alwaysZero = 0;
	status_t status = SendCommand(AUTO_ADJUST, &alwaysZero, sizeof(alwaysZero),
		reply, replyLength);
	if (status < B_OK)
		return false;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return false;
	}

	return reply.data[0] == alwaysZero ? B_OK : B_ERROR;
}


uint8
SamsungMonitorControl::InputSource()
{
	samsung_reply reply;
	uint8 replyLength;
	status_t status = SendCommand(INPUT_SOURCE_CONTROL, NULL, 0, reply,
		replyLength);
	if (status < B_OK)
		return 0;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return 0;
	}

	return reply.data[0];
}


status_t
SamsungMonitorControl::SetInputSource(uint32 id)
{
	static const struct source_ids {
		uint32	id;
		uint8	source_id;
	} kSourceIDs[] = {
		{DISPLAY_INPUT_SOURCE_VGA, 0x14},
		{DISPLAY_INPUT_SOURCE_VGA, 0x18},
	};
	uint8 sourceID = 0xff;

	for (uint32 i = 0; i < sizeof(kSourceIDs) / sizeof(kSourceIDs[0]); i++) {
		if (id == kSourceIDs[i].id) {
			sourceID = kSourceIDs[i].source_id;
			break;
		}
	}

	if (sourceID == 0xff) {
		if (id >= 0xff)
			return B_BAD_VALUE;

		// hack to allow setting the Samsung source IDs directly
		sourceID = (uint8)id;
	}

	samsung_reply reply;
	uint8 replyLength;
	status_t status = SendCommand(INPUT_SOURCE_CONTROL, &sourceID,
		sizeof(sourceID), reply, replyLength);
	if (status < B_OK)
		return false;

	if (!reply.Ack()) {
		fprintf(stderr, "Received error: %u\n", reply.data[0]);
		return false;
	}

	return reply.data[0] == sourceID ? B_OK : B_ERROR;
}

