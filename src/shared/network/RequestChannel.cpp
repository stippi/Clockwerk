/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <new>
#include <stdlib.h>
#include <string.h>

#include <ByteOrder.h>
#include <DataIO.h>
#include <Message.h>

#include "AutoDeleter.h"
#include "Channel.h"
#include "Compatibility.h"
#include "Debug.h"
#include "HashString.h"
#include "Logger.h"
#include "RequestChannel.h"
#include "RequestXMLConverter.h"
#include "XMLHelper.h"
#include "XMLSupport.h"


static Logger sLog("network.RequestChannel");

static const int32 kMaxSaneRequestSize	= 32 * 1024 * 1024;	// 32 MB
static const int32 kDefaultBufferSize	= 4096;			// 4 KB


// ChannelWriter
class RequestChannel::ChannelWriter : public BDataIO {
public:
	ChannelWriter(Channel* channel, void* buffer, int32 bufferSize)
		: fChannel(channel),
		  fBuffer(buffer),
		  fBufferSize(bufferSize),
		  fBytesWritten(0)
	{
	}

	virtual ssize_t Write(const void* buffer, size_t size)
	{
		status_t error = B_OK;
		// if the data don't fit into the buffer anymore, flush the buffer
		if (fBytesWritten + (int32)size > fBufferSize) {
			error = Flush();
			if (error != B_OK)
				return error;
		}

		// if the data don't even fit into an empty buffer, just send it,
		// otherwise append it to the buffer
		if ((int32)size > fBufferSize) {
			error = fChannel->Send(buffer, size);
			if (error != B_OK)
				return error;
		} else {
			memcpy((uint8*)fBuffer + fBytesWritten, buffer, size);
			fBytesWritten += size;
		}
		return error;
	}

	virtual ssize_t Read(void* buffer, size_t size)
	{
		return B_BAD_VALUE;
	}

	status_t Flush()
	{
		if (fBytesWritten == 0)
			return B_OK;
		status_t error = fChannel->Send(fBuffer, fBytesWritten);
		if (error != B_OK)
			return error;
		fBytesWritten = 0;
		return B_OK;
	}

private:
	Channel*	fChannel;
	void*		fBuffer;
	int32		fBufferSize;
	int32		fBytesWritten;
};


// RequestHeader
struct RequestChannel::RequestHeader {
	int32	size;
};


// constructor
RequestChannel::RequestChannel(Channel* channel)
	: fChannel(channel),
	  fBuffer(NULL),
	  fBufferSize(0)
{
	// allocate the send buffer
	fBuffer = malloc(kDefaultBufferSize);
	if (fBuffer)
		fBufferSize = kDefaultBufferSize;
}

// destructor
RequestChannel::~RequestChannel()
{
	free(fBuffer);
}


// IsClosed
bool
RequestChannel::IsClosed() const
{
	return fChannel->IsClosed();
}


// SendRawData
status_t
RequestChannel::SendRawData(const void* buffer, int32 size)
{
	if (!buffer || size < 0)
		return B_BAD_VALUE;

	return fChannel->Send(buffer, size);
}

// ReceiveRawData
status_t
RequestChannel::ReceiveRawData(void* buffer, int32 size)
{
	if (!buffer || size < 0)
		return B_BAD_VALUE;

	return fChannel->Receive(buffer, size);
}

// SendString
status_t
RequestChannel::SendString(const char* text, int32 size)
{
	if (!text)
		RETURN_ERROR(B_BAD_VALUE);

	// get string len
	if (size < 0)
		size = strlen(text);
	else
		size = strnlen(text, size);

	if (size < 0 || size > kMaxSaneRequestSize) {
		ERROR(("RequestChannel::SendString(): ERROR: Invalid message size: "
			"%ld\n", size));
		RETURN_ERROR(B_BAD_DATA);
	}

	// write the header
	RequestHeader header;
	header.size = B_HOST_TO_BENDIAN_INT32(size);
	ChannelWriter writer(fChannel, fBuffer, fBufferSize);
	status_t error = writer.Write(&header, sizeof(RequestHeader));
	if (error != B_OK)
		RETURN_ERROR(error);

	// write the text
	error = writer.Write(text, size);
	if (error != B_OK)
		RETURN_ERROR(error);
	error = writer.Flush();
	RETURN_ERROR(error);
}

// ReceiveString
status_t
RequestChannel::ReceiveString(char** text)
{
	if (!text)
		RETURN_ERROR(B_BAD_VALUE);

	// get the header
	RequestHeader header;
	status_t error = fChannel->Receive(&header, sizeof(RequestHeader));
	if (error != B_OK)
		RETURN_ERROR(error);

	header.size = B_HOST_TO_BENDIAN_INT32(header.size);
	if (header.size <= 0 || header.size > kMaxSaneRequestSize) {
		ERROR(("RequestChannel::ReceiveString(): ERROR: Invalid message size: "
			"%ld\n", header.size));
		RETURN_ERROR(B_BAD_DATA);
	}
	if (header.size >= 5 * 1024 * 1024) {
		WARN(("RequestChannel::ReceiveString(): very large message size: "
			"%ldB or %.2fMB\n", header.size, header.size / (1024.0 * 1024)));
	}

	// allocate a buffer for the data and read them
	char* buffer = (char*)malloc(header.size + 1);
	if (!buffer)
		RETURN_ERROR(B_NO_MEMORY);
	MemoryDeleter bufferDeleter(buffer);

	// receive the data
	error = fChannel->Receive(buffer, header.size);
	if (error != B_OK)
		RETURN_ERROR(error);

	// null-terminate
	buffer[header.size] = '\0';

	// done
	*text = buffer;
	bufferDeleter.Detach();

	return B_OK;
}

// SendXML
status_t
RequestChannel::SendXML(XMLHelper* xmlHelper)
{
	if (!xmlHelper)
		RETURN_ERROR(B_BAD_VALUE);

	// flatten the XML data
	BMallocIO output;
	status_t error = xmlHelper->Save(output);
	if (error != B_OK)
		RETURN_ERROR(error);

	size_t bufferLength = output.BufferLength();
	if (sLog.IsTraceEnabled()) {
		sLog.LogUnformatted(LOG_LEVEL_TRACE, "SendXML(): sending:\n");
		output.Write("", 1);
		sLog.LogUnformatted(LOG_LEVEL_TRACE, (const char*)output.Buffer());
	}

	// send the data
	return SendString((const char*)output.Buffer(), bufferLength);
}

// ReceiveXML
status_t
RequestChannel::ReceiveXML(XMLHelper* xmlHelper)
{
	if (!xmlHelper)
		RETURN_ERROR(B_BAD_VALUE);

	// receive the data
	char* text;
	status_t error = ReceiveString(&text);
	if (error != B_OK)
		RETURN_ERROR(error);
	MemoryDeleter _(text);

	if (sLog.IsTraceEnabled()) {
		sLog.LogUnformatted(LOG_LEVEL_TRACE, "ReceiveXML(): received:\n");
		sLog.LogUnformatted(LOG_LEVEL_TRACE, text);
	}

	// build XML
	BMemoryIO input(text, strlen(text));
	return xmlHelper->Load(input);
}

// SendRequest
status_t
RequestChannel::SendRequest(BMessage* message)
{
	if (!message)
		RETURN_ERROR(B_BAD_VALUE);

	// convert to XML
	XMLHelper* xmlHelper = create_xml_helper();
	if (!xmlHelper) {
		printf("client: failed to create XMLHelper\n");
		RETURN_ERROR(B_ERROR);
	}
	ObjectDeleter<XMLHelper> _(xmlHelper);

	status_t error = RequestXMLConverter().ConvertToXML(message, xmlHelper);
	if (error != B_OK)
		RETURN_ERROR(error);

	return SendXML(xmlHelper);
}

// ReceiveRequest
status_t
RequestChannel::ReceiveRequest(BMessage* message)
{
	if (!message)
		RETURN_ERROR(B_BAD_VALUE);

	// read XML
	XMLHelper* xmlHelper = create_xml_helper();
	if (!xmlHelper) {
		printf("client: failed to create XMLHelper\n");
		RETURN_ERROR(B_ERROR);
	}
	ObjectDeleter<XMLHelper> _(xmlHelper);

	status_t error = ReceiveXML(xmlHelper);
	if (error != B_OK)
		RETURN_ERROR(error);
	
	// convert to message
	return RequestXMLConverter().ConvertFromXML(xmlHelper, message);
}
