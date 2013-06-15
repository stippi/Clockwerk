/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REQUEST_CONNECTION_H
#define REQUEST_CONNECTION_H

#include <Locker.h>
#include <SupportDefs.h>


class BMessage;
class Connection;
class RequestChannel;
class RequestHandler;


class RequestConnection {
public:
								RequestConnection(Connection* connection,
									RequestHandler* requestHandler,
									bool ownsRequestHandler = false);
								~RequestConnection();

			status_t			Init();

			void				Close();

			Connection*			GetConnection() const	{ return fConnection; }

			int32				ProtocolVersion() const;

			// both on the upstream channel
			status_t			SendRawData(const void* buffer, int32 size);
			status_t			ReceiveRawData(void* buffer, int32 size);

			status_t			SendString(const char* text);

			status_t			SendRequest(BMessage* message,
									BMessage* reply = NULL);
			status_t			SendRequest(BMessage* message,
									RequestHandler* replyHandler);
			status_t			ReceiveRequest(BMessage* message);

private:
			status_t			_SendRequest(BMessage* message,
									BMessage* reply,
									RequestHandler* replyHandler);

			void				_Closed(status_t error);

private:
			Connection*			fConnection;
			RequestHandler*		fRequestHandler;
			bool				fOwnsRequestHandler;
			BLocker				fUpStreamLock;
			RequestChannel*		fUpStreamChannel;
			vint32				fTerminationCount;
};


#endif	// REQUEST_CONNECTION_H
