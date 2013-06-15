/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ExecuteRenderer.h"

#include <new>
#include <stdlib.h>

#include <Application.h>
#include <Roster.h>

#include "common.h"

#include "CommonPropertyIDs.h"
#include "ExecuteClip.h"

using std::nothrow;

struct CommandThreadInfo {
	BString		commandPath;
};

// constructor
ExecuteRenderer::ExecuteRenderer(ClipPlaylistItem* item,
		ExecuteClip* clip)
	: ClipRenderer(item, clip)
	, fClip(clip)

	, fCommandPath(clip ? clip->CommandPath() : "")
	, fCommandExecuted(false)
{
	if (fClip)
		fClip->Acquire();
}

// destructor
ExecuteRenderer::~ExecuteRenderer()
{
	if (fClip)
		fClip->Release();
}

// Generate
status_t
ExecuteRenderer::Generate(Painter* painter, double frame,
	const RenderPlaylistItem* item)
{
	if (fCommandExecuted)
		return B_OK;

	if (fCommandPath.Length() <= 0)
		return B_OK;

	// execute the command if running in Player
	// (launch a thread)

	app_info appInfo;
	if (be_app->GetAppInfo(&appInfo) < B_OK)
		return B_OK;

	if (strcmp(appInfo.signature, kPlayerMimeSig) != 0) {
		fCommandExecuted = true;
			// prevent any further checks
		return B_OK;
	}

	BString threadName("execute: ");
	threadName << fCommandPath;

	thread_id thread = find_thread(threadName.String());
	if (thread >= 0) {
		fCommandExecuted = true;
		return B_OK;
	}

	CommandThreadInfo* info = new (nothrow) CommandThreadInfo;
	if (!info)
		return B_NO_MEMORY;

	info->commandPath = fCommandPath;

	thread = spawn_thread(_CommandThread, threadName.String(),
		B_NORMAL_PRIORITY, info);
	if (thread < B_OK) {
		delete info;
		return thread;
	}

	fCommandExecuted = true;
	resume_thread(thread);

	return B_OK;
}

// Sync
void
ExecuteRenderer::Sync()
{
	ClipRenderer::Sync();

	if (fClip)
		fCommandPath = fClip->CommandPath();
}

// #pragma mark -

/*static*/ int32
ExecuteRenderer::_CommandThread(void* cookie)
{
	CommandThreadInfo* info = (CommandThreadInfo*)cookie;
	if (!info)
		return B_BAD_VALUE;

	print_info("executing: %s\n", info->commandPath.String());
	system(info->commandPath.String());

	delete info;

	return B_OK;
}




