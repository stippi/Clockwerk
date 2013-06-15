/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REQUEST_CHANNEL_H
#define REQUEST_CHANNEL_H

#include <SupportDefs.h>

class BMessage;

class Channel;
class XMLHelper;

class RequestChannel {
public:
								RequestChannel(Channel* channel);
								~RequestChannel();

			bool				IsClosed() const;

			status_t			SendRawData(const void* data, int32 size);
			status_t			ReceiveRawData(void* data, int32 size);

			status_t			SendString(const char* text, int32 size = -1);
			status_t			ReceiveString(char** text);

			status_t			SendXML(XMLHelper* xmlHelper);
			status_t			ReceiveXML(XMLHelper* xmlHelper);

			status_t			SendRequest(BMessage* message);
			status_t			ReceiveRequest(BMessage* message);

private:
			class ChannelWriter;
			struct RequestHeader;

			Channel*			fChannel;
			void*				fBuffer;
			int32				fBufferSize;
};

#endif	// REQUEST_CHANNEL_H
