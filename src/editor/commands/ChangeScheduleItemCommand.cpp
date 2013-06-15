/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ChangeScheduleItemCommand.h"

#include <new>
#include <stdio.h>

#include "Schedule.h"
#include "ScheduleItem.h"
#include "Selection.h"

using std::nothrow;

// constructor
ChangeScheduleItemCommand::ChangeScheduleItemCommand(
		Schedule* schedule, Selection* selection, ScheduleItem* item,
		uint64 startFrame, uint64 duration, uint16 repeats,
		bool flexibleStartFrame, bool flexibleDuration)
	: Command()
	, fSchedule(schedule)
	, fSelection(selection)
	, fItem(item)

	, fStartFrame(startFrame)
	, fDuration(duration)
	, fExplicitRepeats(repeats)

	, fFlexibleStartFrame(flexibleStartFrame)
	, fFlexibleDuration(flexibleDuration)

	, fType(0)
{
	if (!fItem)
		return;

	if (fItem->StartFrame() != fStartFrame)
		fType |= COMMAND_STARTFRAME;
	if (fItem->Duration() != fDuration)
		fType |= COMMAND_DURATION;
	if (fItem->Repeats() != fExplicitRepeats)
		fType |= COMMAND_REPEATS;
	// NOTE: the flags will likely result in either startframe
	// or duration to be adjusted, so we make sure that we will
	// apply the saved 
	if (fItem->FlexibleStartFrame() != fFlexibleStartFrame)
		fType |= COMMAND_FLEXIBLE_STARTFRAME | COMMAND_STARTFRAME;
	if (fItem->FlexibleDuration() != fFlexibleDuration)
		fType |= COMMAND_FLEXIBLE_DURATION | COMMAND_DURATION;
}

// destructor
ChangeScheduleItemCommand::~ChangeScheduleItemCommand()
{
}

// InitCheck
status_t
ChangeScheduleItemCommand::InitCheck()
{
	if (fSchedule && fType != 0)
		return B_OK;
	return B_NO_INIT;
}

// Perform
status_t
ChangeScheduleItemCommand::Perform()
{
	ScheduleNotificationBlock _(fSchedule);

	if (fSelection && !fItem->IsSelected())
		fSelection->Select(fItem);

	// save the current values before applying flags
	uint64 startFrame = fItem->StartFrame();
	uint64 duration = fItem->Duration();
	uint16 repeats = fItem->ExplicitRepeats();


	// NOTE: it is important to set the flags
	// before setting the duration/startframe,
	// since it will prevent the duration to be
	// filtered in certain conditions

	if (fType & COMMAND_FLEXIBLE_STARTFRAME) {
		// change item flexible startframe
		bool flexible = fItem->FlexibleStartFrame();
		fItem->SetFlexibleStartFrame(fFlexibleStartFrame);
		fFlexibleStartFrame = flexible;
	}

	if (fType & COMMAND_FLEXIBLE_DURATION) {
		// change item flexible startframe
		bool flexible = fItem->FlexibleDuration();
		fItem->SetFlexibleDuration(fFlexibleDuration);
		fFlexibleDuration = flexible;
	}

	// now set the frame values, after having set the flags

	if (fType & COMMAND_STARTFRAME) {
		// change item startframe
		// (never filter, because than the startframe could
		// never be adjusted)
		fItem->SetStartFrame(fStartFrame);
		fStartFrame = startFrame;
	}

	if (fType & COMMAND_DURATION) {
		// change item duration
		fItem->FilterDuration(&fDuration);
		fItem->SetDuration(fDuration);
		fDuration = duration;
	}

	if (fType & COMMAND_REPEATS) {
		// change item repeats
		fItem->SetExplicitRepeats(fExplicitRepeats);
		fExplicitRepeats = repeats;
	}

	fSchedule->SanitizeStartFrames();

	return B_OK;
}

// Undo
status_t
ChangeScheduleItemCommand::Undo()
{
	return Perform();
}

// GetName
void
ChangeScheduleItemCommand::GetName(BString& name)
{
	if (fType & COMMAND_FLEXIBLE_STARTFRAME)
		name << "Toggle Fixed Start";
	else if (fType & COMMAND_FLEXIBLE_DURATION)
		name << "Toggle Flexible Duration";

	else if (fType == COMMAND_STARTFRAME)
		name << "Change Schedule Item Start";
	else if (fType == COMMAND_DURATION)
		name << "Change Schedule Item Duration";
	else if (fType == COMMAND_REPEATS)
		name << "Change Schedule Item Repeats";

	else
		name << "Modify Schedule Item";
}

// CombineWithNext
bool
ChangeScheduleItemCommand::CombineWithNext(const Command* next)
{
	const ChangeScheduleItemCommand* other
		= dynamic_cast<const ChangeScheduleItemCommand*>(next);
	if (other && other->fTimeStamp - fTimeStamp < 500000
		&& other->fSchedule == fSchedule
		&& other->fItem == fItem
		&& other->fType == fType
		&& (fType & (COMMAND_STARTFRAME | COMMAND_DURATION))) {
		fTimeStamp = other->fTimeStamp;
		return true;
	}
	return false;
}
