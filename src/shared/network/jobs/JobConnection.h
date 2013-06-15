/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef JOB_CONNECTION_H
#define JOB_CONNECTION_H

#include <OS.h>
#include <String.h>
#include <List.h>
#include <Locker.h>

#include "RequestHandler.h"


class Connection;
class JobConnection;
class RequestConnection;
class StatusOutput;

// Job
class Job {
public:
	virtual						~Job();

	virtual	status_t			Execute(JobConnection* connection) = 0;

protected:
			const char*			CheckErrorReply(BMessage* reply);

			char				fErrorBuffer[128];
};

class JobConnection;

// JobConnectionListener
class JobConnectionListener {
public:
								JobConnectionListener();
	virtual						~JobConnectionListener();

	virtual	void				Connecting(JobConnection* connection);
	virtual	void				FailedToConnect(JobConnection* connection);
	virtual	void				Connected(JobConnection* connection);
	virtual	void				Disconnected(JobConnection* connection);
	virtual	void				Deleted(JobConnection* connection);
};


// JobConnection
class JobConnection : private RequestHandler {
public:
								JobConnection(bool keepAlive = true);
								~JobConnection();

			status_t			Init(const char* clientID,
									const char* serverAddress);
			void				UnInit();

			const char*			ClientID() const
									{ return fClientID.String(); }
			const BString&		ServerAddress() const
									{ return fServerAddress; }

			status_t			Open();
			status_t			Close();
			bool				IsOpen() const;

			RequestConnection*	GetRequestConnection() const;

			void				SetStatusOutput(StatusOutput* statusOutput);
			StatusOutput*		GetStatusOutput() const;

			status_t			ScheduleJob(Job* job, bool deleteIfFailed);


	static	JobConnection*		CreateConnection(bool keepAlive = true);


			bool				AddListener(JobConnectionListener* listener);
			void				RemoveListener(JobConnectionListener* listener);

private:
			struct JobQueue;
			class ConnectJob;
			friend class ConnectJob;
			class DisconnectJob;
			friend class DisconnectJob;

	virtual	status_t			HandleRequest(BMessage* request,
									RequestChannel* channel);

	static	int32				_JobLoopEntry(void* data);
			int32				_JobLoop();

			status_t			_Connect();
			void				_Disconnect();

			void				_NotifyConnecting();
			void				_NotifyConnectionFailed();
			void				_NotifyConnected();
			void				_NotifyDisconnected();
			void				_NotifyDeleted();

private:
			class ProxyStatusOutput;

			BString				fServerAddress;
			BString				fClientID;
			bool				fKeepAlive;
			RequestConnection*	fConnection;
			JobQueue*			fJobQueue;

			ProxyStatusOutput*	fStatusOutput;

			thread_id			fJobThread;
	volatile bool				fTerminating;

			BList				fListeners;
};


#endif	// JOB_CONNECTION_H
