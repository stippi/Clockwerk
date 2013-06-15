/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "JobConnection.h"

#include <signal.h>
#include <stdio.h>

#include "AutoDeleter.h"
#include "BlockingQueue.h"
#include "ConnectionFactory.h"
#include "Debug.h"
#include "RequestConnection.h"
#include "RequestMessageCodes.h"

#include "StatusOutput.h"


class JobConnection::ProxyStatusOutput : public StatusOutput {
 public:
								ProxyStatusOutput();
	virtual						~ProxyStatusOutput();

	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* format, va_list list);
	virtual	void				InternalPrintStatusMessage(uint32 type,
									const char* message);

			status_t			InitCheck() const;
			void				SetStatusOutput(StatusOutput* output);
 private:
			StatusOutput*		fConsoleStatusOutput;
			StatusOutput*		fStatusOutput;
			BLocker				fStatusOutputLock;
};

// constructor
JobConnection::ProxyStatusOutput::ProxyStatusOutput()
	: StatusOutput()
	, fConsoleStatusOutput(new (std::nothrow) ConsoleStatusOutput)
	, fStatusOutput(fConsoleStatusOutput)
	, fStatusOutputLock("job connection status output")
{
}

// destructor
JobConnection::ProxyStatusOutput::~ProxyStatusOutput()
{
	delete fConsoleStatusOutput;
}

// InternalPrintStatusMessage
void
JobConnection::ProxyStatusOutput::InternalPrintStatusMessage(uint32 type,
	const char* format, va_list list)
{
	if (!fStatusOutput || !fStatusOutputLock.Lock())
		return;

	fStatusOutput->InternalPrintStatusMessage(type, format, list);

	fStatusOutputLock.Unlock();
}

// InternalPrintStatusMessage
void
JobConnection::ProxyStatusOutput::InternalPrintStatusMessage(uint32 type,
	const char* message)
{
	if (!fStatusOutput || !fStatusOutputLock.Lock())
		return;

	fStatusOutput->InternalPrintStatusMessage(type, message);

	fStatusOutputLock.Unlock();
}

// InitCheck
status_t
JobConnection::ProxyStatusOutput::InitCheck() const
{
	if (!fConsoleStatusOutput)
		return B_NO_MEMORY;
	return B_OK;
}

// SetStatusOutput
void
JobConnection::ProxyStatusOutput::SetStatusOutput(StatusOutput* output)
{
	if (!fStatusOutputLock.Lock())
		return;

	if (!output)
		output = fConsoleStatusOutput;

	fStatusOutput = output;

	fStatusOutputLock.Unlock();
}

// #pragma mark - Job

// destructor
Job::~Job()
{
}

// CheckErrorReply
const char*
Job::CheckErrorReply(BMessage* reply)
{
	// check, if it is an error reply at all
	if (reply->what != REQUEST_ERROR_REPLY) {
		sprintf(fErrorBuffer, "Unexpected reply: code: 0x%lx\n",
			reply->what);
		return fErrorBuffer;
	}

	// get error message
	const char* errorMessage;
	if (reply->FindString("error", &errorMessage) == B_OK)
		return errorMessage;
	return NULL;
}

// #pragma mark - ConnectJob

class JobConnection::ConnectJob : public Job {
public:
	ConnectJob(bool keepTrying)
		: fKeepTrying(keepTrying)
	{
	}

	virtual ~ConnectJob()
	{
	}

	virtual status_t Execute(JobConnection* connection)
	{
		status_t error = B_OK;
		do {
			if (error < B_OK)
				snooze(10 * 1000000);
			error = connection->_Connect();
		} while (fKeepTrying
				 && error < B_OK
				 && error != B_NO_MEMORY
				 && !connection->fTerminating);
		return error;
	}
private:
	bool	fKeepTrying;
};

class JobConnection::DisconnectJob : public Job {
public:
	virtual ~DisconnectJob()
	{
	}

	virtual status_t Execute(JobConnection* connection)
	{
		connection->_Disconnect();
		return B_OK;
	}
};

// #pragma mark - JobConnectionListener

JobConnectionListener::JobConnectionListener()					{}
JobConnectionListener::~JobConnectionListener()					{}
void JobConnectionListener::Connecting(JobConnection*)			{}
void JobConnectionListener::FailedToConnect(JobConnection*)	{}
void JobConnectionListener::Connected(JobConnection*)			{}
void JobConnectionListener::Disconnected(JobConnection*)		{}
void JobConnectionListener::Deleted(JobConnection*)				{}

// #pragma mark - JobConnection

// JobQueue
struct JobConnection::JobQueue : BlockingQueue<Job> {
	JobQueue() : BlockingQueue<Job>("job queue") {}
};

// constructor
JobConnection::JobConnection(bool keepAlive)
	: fServerAddress(""),
	  fKeepAlive(keepAlive),
	  fConnection(NULL),
	  fJobQueue(NULL),
	  fStatusOutput(new(std::nothrow) ProxyStatusOutput),
	  fJobThread(-1),
	  fTerminating(false)
{
}

// destructor
JobConnection::~JobConnection()
{
	UnInit();

	delete fStatusOutput;

	_NotifyDeleted();
}

// Init
status_t
JobConnection::Init(const char* clientID, const char* serverAddress)
{
	if (!fStatusOutput || fStatusOutput->InitCheck() < B_OK)
		return B_NO_MEMORY;

	UnInit();

	// create job queue
	fJobQueue = new(std::nothrow) JobQueue;
	if (!fJobQueue)
		return B_NO_MEMORY;

	status_t error = fJobQueue->InitCheck();
	if (error != B_OK)
		return error;

	fClientID = clientID;
	fServerAddress = serverAddress;

	// spawn job thread
	fJobThread = spawn_thread(_JobLoopEntry, "job thread",
		B_NORMAL_PRIORITY, this);
	if (fJobThread < 0)
		return fJobThread;

	fTerminating = false;

	// resume job thread
	resume_thread(fJobThread);

	return Open();		
}

void
JobConnection::UnInit()
{
	fTerminating = true;

	// close job queue
	if (fJobQueue)
		fJobQueue->Close(false);
		// TODO: "false" is correct, but also means the Jobs are leaked

	// close connection
	if (fConnection)
		fConnection->Close();

	// wait for job thread to finish
	if (fJobThread >= 0) {
		int32 result;
		wait_for_thread(fJobThread, &result);
		fJobThread = -1;
	}

	// delete objects
	delete fConnection;
	fConnection = NULL;
	delete fJobQueue;
	fJobQueue = NULL;
}

// Open
status_t
JobConnection::Open()
{
	return ScheduleJob(new (std::nothrow) ConnectJob(fKeepAlive), true);
}

// Close
status_t
JobConnection::Close()
{
	return ScheduleJob(new (std::nothrow) DisconnectJob(), true);
	// TODO: maybe close this the hard way if creating a job failed...
}

// IsOpen
bool
JobConnection::IsOpen() const
{
	// TODO: race condition with job thread
	return fConnection != NULL;
}

// GetRequestConnection
RequestConnection*
JobConnection::GetRequestConnection() const
{
	// TODO: should only be used by Jobs (ie in the job thread)
	return fConnection;
}

// SetStatusOutput
void
JobConnection::SetStatusOutput(StatusOutput* statusOutput)
{
	// NOTE: is threadsafe
	fStatusOutput->SetStatusOutput(statusOutput);
}

// GetStatusOutput
StatusOutput*
JobConnection::GetStatusOutput() const
{
	return fStatusOutput;
}

// ScheduleJob
status_t
JobConnection::ScheduleJob(Job* job, bool deleteIfFailed)
{
	if (!job)
		return B_NO_MEMORY;

	status_t error = fJobQueue->Push(job);
	if (error < B_OK) {
		if (deleteIfFailed)
			delete job;
		RETURN_ERROR(error);
	}

	return B_OK;
}

// HandleRequest
status_t
JobConnection::HandleRequest(BMessage* request, RequestChannel* channel)
{
	if (request->what == REQUEST_CONNECTION_BROKEN) {
		// TODO: Notify the app to inform the user.
		fStatusOutput->PrintErrorMessage("Connection to '%s' closed!\n",
			fServerAddress.String());
		fConnection->Close();
		// TODO: evaluate fKeepAlive here?
		Open();
	}
	return B_OK;
}

// #pragma mark -

// AddListener
bool
JobConnection::AddListener(JobConnectionListener* listener)
{
	// TODO: should lock, noticiations are send from job thread
	if (listener && !fListeners.HasItem(listener))
		return fListeners.AddItem(listener);
	return false;
}

// RemoveListener
void
JobConnection::RemoveListener(JobConnectionListener* listener)
{
	// TODO: should lock, noticiations are send from job thread
	fListeners.AddItem(listener);
}

// #pragma mark -

// _JobLoopEntry
int32
JobConnection::_JobLoopEntry(void* data)
{
	return ((JobConnection*)data)->_JobLoop();
}

// _JobLoop
int32
JobConnection::_JobLoop()
{
	// We need to ignore SIGPIPE, since we set SO_KEEPALIVE on our sockets
	// and writing to them might raise a SIGPIPE.
	sigset_t signalMask;
	sigemptyset(&signalMask);
	sigaddset(&signalMask, SIGPIPE);
	sigprocmask(SIG_BLOCK, &signalMask, NULL);

	while (!fTerminating) {
		// get next job
		Job* job;
		status_t error = fJobQueue->Pop(&job);
		if (error != B_OK) {
			// B_BAD_SEM_ID means the connection was just closed
			if (error != B_BAD_SEM_ID)
				printf("fJobQueue->Pop(): %s\n", strerror(error));
			break;
		}
		ObjectDeleter<Job> _(job);

		// execute job
		error = job->Execute(this);
		if (error != B_OK) {
			// ...
		}
	}
//printf("JobConnection::_JobLoop() - terminated\n");

	return 0;
}

// _Connect
status_t
JobConnection::_Connect()
{
	// NOTE: always executed in job thread

	_Disconnect();

	_NotifyConnecting();

	// establish the connection
	Connection* connection;
	ConnectionFactory factory;
	status_t error = factory.CreateConnection("insecure",
//	status_t error = factory.CreateConnection("ssl",
		fClientID.String(),
		fServerAddress.String(),
		&connection);
	if (error != B_OK) {
		fStatusOutput->PrintWarningMessage("Failed creating connection "
			"to '%s': %s\n", fServerAddress.String(), strerror(error));
		_NotifyConnectionFailed();
		RETURN_ERROR(error);
	}

	// create request connection
	fConnection = new(std::nothrow) RequestConnection(connection, this);
	if (!fConnection) {
		fStatusOutput->PrintErrorMessage("Failed creating connection "
			"to '%s': no memory!\n", fServerAddress.String());
		_NotifyConnectionFailed();
		RETURN_ERROR(B_NO_MEMORY);
	}

	error = fConnection->Init();
	if (error != B_OK) {
		fStatusOutput->PrintWarningMessage("Failed to init connection: %s\n",
			strerror(error));
		delete fConnection;
		fConnection = NULL;
		_NotifyConnectionFailed();
		return error;
	}

//	fStatusOutput->PrintInfoMessage("Connection to '%s' created "
//		"successfully.\n", fServerAddress.String());

	_NotifyConnected();
	return B_OK;
}

// _Disconnect
void
JobConnection::_Disconnect()
{
	// NOTE: always executed in job thread

	if (fConnection) {
		delete fConnection;
		fConnection = NULL;
		_NotifyDisconnected();
	}
}

// #pragma mark -

// _NotifyConnecting
void
JobConnection::_NotifyConnecting()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		JobConnectionListener* listener
			= (JobConnectionListener*)listeners.ItemAtFast(i);
		listener->Connecting(this);
	}
}

// _NotifyConnectionFailed
void
JobConnection::_NotifyConnectionFailed()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		JobConnectionListener* listener
			= (JobConnectionListener*)listeners.ItemAtFast(i);
		listener->FailedToConnect(this);
	}
}

// _NotifyConnected
void
JobConnection::_NotifyConnected()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		JobConnectionListener* listener
			= (JobConnectionListener*)listeners.ItemAtFast(i);
		listener->Connected(this);
	}
}

// _NotifyDisconnected
void
JobConnection::_NotifyDisconnected()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		JobConnectionListener* listener
			= (JobConnectionListener*)listeners.ItemAtFast(i);
		listener->Disconnected(this);
	}
}

// _NotifyDeleted
void
JobConnection::_NotifyDeleted()
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		JobConnectionListener* listener
			= (JobConnectionListener*)listeners.ItemAtFast(i);
		listener->Deleted(this);
	}
}

// #pragma mark -

// CreateConnection
JobConnection*
JobConnection::CreateConnection(bool keepAlive)
{
	// create job connection
	return new(std::nothrow) JobConnection(keepAlive);
}


