/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCHEDULE_WINDOW_H
#define SCHEDULE_WINDOW_H

#include <Window.h>

#include "Observer.h"
#include "ServerObjectManager.h"

class BMenu;
class BMenuItem;
class CommandStack;
class PlaylistObjectListView;
class Schedule;
class ScheduleListGroup;
class ScheduleObjectListView;
class ScheduleTopView;
class ScheduleView;
class SchedulePropertiesView;
class Selection;
class ServerObjectFactory;
class StatusBar;
class TimeRangePanel;

class ScheduleWindow : public BWindow,
					   public Observer {
 public:
								ScheduleWindow(BRect frame);
	virtual						~ScheduleWindow();

	// BWindow interface
	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	virtual	void				Hide();

	// Observer
	virtual	void				ObjectChanged(const Observable* object);

	// ScheduleWindow
			void				PrepareForQuit();

			void				SetObjectManager(
									ServerObjectManager* manager);
			void				SetObjectFactory(
									ServerObjectFactory* factory);

			void				SetSchedule(Schedule* schedule);
			void				SetScopes(const BMessage* scopes);

			void				StoreSettings(BMessage* archive) const;
			void				RestoreSettings(BMessage* archive);

 private:
			void				_CreateGUI(BRect frame);
			BMenuBar*			_CreateMenuBar(BRect frame);

			void				_CreateSchedule();
			void				_DuplicateSchedule();
			void				_RemoveSchedule();

			void				_ObjectAdded(ServerObject* object,
									int32 index);
			void				_ObjectRemoved(ServerObject* object);

			void				_RenderSchedule(const BMessage* message);

			void				_MarkScheduleUnsavedIfNecessary();
			void				_ForceUpdate();

	static	int32				_ExportScheduleEntry(void* cookie);
	static	status_t			_ExportScheduleCSV(
									const Schedule* schedule);

	typedef enum {
		SORT_BY_NAME = 0,
		SORT_BY_PORTION,
	} SortMode;

	static	status_t			_ExportScheduleSummary(
									const Schedule* schedule,
									SortMode sortMode);

	// object manager
	ServerObjectManager*		fObjectManager;
	ServerObjectFactory*		fObjectFactory;
	AsyncSOMListener*			fSOMListener;
	Schedule*					fSchedule;
	CommandStack*				fCommandStack;
	Selection*					fSelection;

	// views
	BMenu*						fScheduleMenu;
	BMenuItem*					fCreateMI;
	BMenuItem*					fDuplicateMI;
	BMenuItem*					fForceUpdateMI;
	BMenuItem*					fRenderMI;
	BMenuItem*					fExportCSVMI;
	BMenuItem*					fExportSummaryNameMI;
	BMenuItem*					fExportSummaryPortionMI;
	BMenuItem*					fRemoveMI;
	BMenuItem*					fUndoMI;
	BMenuItem*					fRedoMI;

	PlaylistObjectListView*		fPlaylistListView;
	ScheduleObjectListView*		fScheduleListView;
	ScheduleListGroup*			fScheduleListGroup;
	ScheduleView*				fScheduleView;
	SchedulePropertiesView*		fPropertiesView;

	ScheduleTopView*			fTopView;

	StatusBar*					fStatusBar;

	// for DispatchMessage() to work correctly
	BView*						fLastMouseMovedView;

	TimeRangePanel*				fTimeRangePanel;
};

#endif // SCHEDULE_WINDOW_H
