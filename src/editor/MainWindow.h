/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <List.h>
#include <Window.h>

#include "common_constants.h"

#include "Observer.h"
#include "ServerObjectManager.h"

class BitmapClip;
class BFilePanel;
class BMenu;
class Clip;
class ClipListView;
class ClipGroup;
class Document;
class EditorApp;
class EditorVideoView;
class FileBasedClip;
class FilePanel;
class Group;
class IconButton;
class IconOptionsControl;
class LoopModeControl;
class MediaClip;
class PlaybackController;
class Playlist;
class PlaylistPlaybackManager;
class PropertyObject;
class PropertyListView;
class RenderSettingsWindow;
class SlideShowPlaylist;
class StageTool;
class TimelineView;
class TimeView;
class TimelineTool;
class TopView;
class TrackHeaderView;
class TrackInfoView;
class TrackView;

enum {
	MSG_SET_PLAYLIST			= 'stpl',
	MSG_REATTACH				= 'rtch',
};

class MainWindow : public BWindow,
				   public Observer,
				   public SOMListener {
 public:
								MainWindow(BRect frame, EditorApp* app,
									::Document* document);
	virtual						~MainWindow();

	// BWindow interface
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

	virtual	void				MenusBeginning();
	virtual	void				MenusEnded();

	// Observer interface
	virtual	void				ObjectChanged(const Observable* object);

	// SOMListener interface
	virtual	void				ObjectAdded(ServerObject* object,
									int32 index);
	virtual	void				ObjectRemoved(ServerObject* object);

	// MainWindow
			void				SetPlaylist(Playlist* playlist);
			void				PrepareForQuit();

			void				SetObjectManager(
									ServerObjectManager* manager);

			void				AddTool(TimelineTool* tool, bool addIcon);
			void				AddTool(StageTool* tool);

			void				StoreSettings(BMessage* archive);
			void				RestoreSettings(BMessage* archive);

 private:
			void				_Init();
			void				_InitTimelineTools();
			void				_InitStageTools();
			TopView*			_CreateGUI(BRect frame, BRect videoSize);
			BMenuBar*			_CreateMenuBar(BRect frame);

			void				_OpenPlaylist(const BString& playlistID);

			void				_UpdateClipboardItems();

			void				_RemoveSelectedClips();
			void				_DuplicateSelectedClips();

			// editing certain clip types
			void				_EditSelectedClips();
			void				_EditClip(Clip* clip);
			void				_EditBitmapClip(BitmapClip* bitmap);
			void				_EditMediaClip(MediaClip* clip);
			void				_EditSlideShowPlaylist(
									SlideShowPlaylist* playlist);
			void				_EditPlaylist(Playlist* playlist);

			void				_UpdateSelectedClips();
			void				_UpdateSelectedClipsFromFile(
									BMessage* message);

	// we need the application for some stuff
	EditorApp*					fApp;
	// object manager
	ServerObjectManager*		fObjectManager;
	// document
	Document*					fDocument;
	// playback framework
	PlaylistPlaybackManager*	fPlaybackManager;

	// views
	TopView*					fTopView;
	BMenuBar*					fMenuBar;
	BMenu*						fPlaylistMenu;
	BMenuItem*					fExportImageMI;
	BMenu*						fEditMenu;
	BMenuItem*					fUndoMI;
	BMenuItem*					fRedoMI;
	BMenuItem*					fCutMI;
	BMenuItem*					fCopyMI;
	BMenuItem*					fPasteMI;
	BMenuItem*					fDuplicateMI;
	BMenuItem*					fRemoveMI;
	BMenu*						fClipMenu;
	BMenu*						fCreateMenu;
	BMenuItem*					fDuplicateClipsMI;
	BMenuItem*					fEditClipMI;
	BMenuItem*					fRemoveClipsMI;
	BMenuItem*					fUpdateClipDataMI;
#ifndef CLOCKWERK_STAND_ALONE
	BMenu*						fWindowMenu;
#endif
	BMenu*						fPropertyMenu;

	ClipListView*				fClipListView;
	ClipGroup*					fClipGroup;
	PropertyListView*			fPropertyListView;
	TimelineView*				fTimelineView;
	TimeView*					fTimeView;
	TrackView*					fTrackView;
	TrackHeaderView*			fTrackHeaderView;
	TrackInfoView*				fTrackInfoView;
	LoopModeControl*			fLoopModeControl;
	PlaybackController*			fTransportGroup;
	EditorVideoView*			fVideoView;
	IconOptionsControl*			fToolIconContainer;
	IconOptionsControl*			fStageToolIconContainer;
	RenderSettingsWindow*		fRenderSettingsWindow;

	Group*						fCommandIconContainer;
	IconButton*					fCutIB;
	IconButton*					fDeleteIB;
	IconButton*					fZoomOutIB;
	IconButton*					fZoomInIB;
	IconButton*					fUndoIB;
	IconButton*					fRedoIB;

	BList						fTimelineTools;
	BList						fStageTools;

	BFilePanel*					fUpdateDataPanel;

	bool						fTextViewFocused;
	bool						fIgnoreFocusChange;
	bool						fPlaylistItemsSelected;
	bool						fPreparedForQuit;
};

#endif // MAIN_WINDOW_H
