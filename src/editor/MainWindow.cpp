/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "MainWindow.h"

#include <stdio.h>

#include <Alert.h>
#include <Box.h>
#include <Clipboard.h>
#include <FilePanel.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Messenger.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Roster.h>
#include <ScrollBar.h>
#include <ScrollView.h>
#include <SeparatorView.h>
#include <TextView.h>

#include "common.h"
#include "ui_defines.h"

#include "AddObjectsCommand.h"
#include "AudioProducer.h"
#include "AutoDeleter.h"
#include "BBP.h"
#include "BitmapClip.h"
#include "ClipGroup.h"
#include "ClipListView.h"
#include "ClipPlaylistItem.h"
#include "CompoundCommand.h"
#include "CommandStack.h"
#include "DeleteCommand.h"
#include "Document.h"
#include "EditorApp.h"
#include "EditorVideoView.h"
#include "FileBasedClip.h"
#include "Group.h"
#include "IconBar.h"
#include "IconButton.h"
#include "IconOptionsControl.h"
#include "Icons.h"
#include "LoopModeControl.h"
#include "MediaClip.h"
#include "MessageConstants.h"
#include "NetworkStatusPanel.h"
#include "PeakView.h"
#include "PlaybackController.h"
#include "Playlist.h"
#include "PlaylistPlaybackManager.h"
#include "PlaylistRenderer.h"
#include "PropertyListView.h"
#include "RemoveObjectsCommand.h"
#include "RenderSettingsWindow.h"
#include "SavePlaylistSnapshot.h"
#include "ScrollView.h"
#include "ScrollingTextClip.h"
#include "ServerObjectFactory.h"
#include "Selection.h"
#include "SlideShowPlaylist.h"
#include "SlideShowWindow.h"
#include "TimelineMessages.h"
#include "TimelineView.h"
#include "TimeView.h"
#include "TopView.h"
#include "TrackHeaderView.h"
#include "TrackInfoView.h"
#include "TrackView.h"

// timeline tools
#include "CutTool.h"
#include "DeleteTool.h"
#include "PickTool.h"

// stage tools
#include "EditOnStageTool.h"
#include "NoneTool.h"
#include "TransformTool.h"

#include "PropertyObject.h"

enum {
	MSG_SET_TOOL					= 'sttl',
	MSG_SET_STAGE_TOOL				= 'stst',

	MSG_UNDO						= 'undo',
	MSG_REDO						= 'redo',

	MSG_REMOVE_CLIPS				= 'rcps',
	MSG_DUPLICATE_CLIPS				= 'dcps',
	MSG_EDIT_CLIPS					= 'edcl',
	MSG_UPDATE_CLIP_DATA			= 'upcl',
	MSG_FILE_FOR_CLIP_UPDATE		= 'fucl',

	MSG_CLIPS_SELECTED				= 'cpsl',
	MSG_CLIP_INVOKED				= 'cpiv',

	MSG_EXPORT_PLAYBACK_SNAPSHOT	= 'exps',
};


// constructor
MainWindow::MainWindow(BRect frame, EditorApp* app, ::Document* document)
	: BWindow(frame, "Clockwerk",
			  B_DOCUMENT_WINDOW, B_ASYNCHRONOUS_CONTROLS),
	  fApp(app),
	  fObjectManager(NULL),
	  fDocument(document),
	  fPlaybackManager(NULL),
	  fClipListView(NULL),
	  fTimelineView(NULL),
	  fTransportGroup(NULL),
	  fVideoView(NULL),
	  fUpdateDataPanel(NULL),
	  fTextViewFocused(false),
	  fIgnoreFocusChange(false),
	  fPlaylistItemsSelected(false),
	  fPreparedForQuit(false)
{
	_Init();
}

// destructor
MainWindow::~MainWindow()
{
	if (fDocument) {
		fDocument->CommandStack()->RemoveObserver(this);

		fDocument->ClipSelection()->RemoveObserver(this);
		fDocument->PlaylistSelection()->RemoveObserver(this);
		fDocument->VideoViewSelection()->RemoveObserver(this);

		fDocument->CurrentFrame()->RemoveObserver(this);
	}

	if (fObjectManager)
		fObjectManager->RemoveListener(this);

	int32 count = fTimelineTools.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (TimelineTool*)fTimelineTools.ItemAtFast(i);
	count = fStageTools.CountItems();
	for (int32 i = 0; i < count; i++)
		delete (StageTool*)fStageTools.ItemAtFast(i);

	delete fUpdateDataPanel;
}

// #pragma mark -

// MessageReceived
void
MainWindow::MessageReceived(BMessage* message)
{
	if (fPreparedForQuit) {
		BWindow::MessageReceived(message);
		return;
	}

	switch (message->what) {
		case MSG_PREPARE_FOR_SYNCHRONIZATION:
			SetPlaylist(NULL);
			fDocument->ClipSelection()->DeselectAll();
			fDocument->PlaylistSelection()->DeselectAll();
			fDocument->VideoViewSelection()->DeselectAll();
			SetObjectManager(NULL);

			message->SendReply(MSG_READY_FOR_SYNCHRONIZATION);
			break;
		case MSG_REATTACH:
			SetObjectManager(fApp->ObjectManager());
			SetPlaylist(fDocument->Playlist());
			break;
		case MSG_SET_PLAYLIST:
		{
			BString playlistID;
			if (message->FindString("playlist", &playlistID) == B_OK) {
				_OpenPlaylist(playlistID);
			} else {
				print_error("MSG_SET_PLAYLIST - no playlist id\n");
			}
			break;
		}

		case MSG_EXPORT_PLAYBACK_SNAPSHOT:
		{
			Playlist* playlist = fDocument->Playlist();
			if (!playlist || !fPlaybackManager || !fPlaybackManager->Lock())
				break;
			int64 currentFrame = fPlaybackManager->CurrentFrame();
			fPlaybackManager->Unlock();
			PlaybackSnapShotInfo* info
				= new (std::nothrow) PlaybackSnapShotInfo(playlist,
					currentFrame);
			if (!info)
				break;
			thread_id thread = spawn_thread(save_playback_snapshot,
				"snapshot saver", B_LOW_PRIORITY, info);
			if (thread < B_OK || resume_thread(thread) < B_OK)
				delete info;
			break;
		}
		case MSG_SET_TOOL:
		{
			int32 toolIndex;
			if (fTimelineView && message->FindInt32("tool", &toolIndex)
				== B_OK) {
				if (TimelineTool* tool
						= (TimelineTool*)fTimelineTools.ItemAt(toolIndex)) {
					fTimelineView->SetTool(tool);
				}
			}
			break;
		}
		case MSG_SET_STAGE_TOOL:
		{
			int32 toolIndex;
			if (fVideoView && message->FindInt32("tool", &toolIndex) == B_OK) {
				if (StageTool* tool = (StageTool*)fStageTools.ItemAt(toolIndex))
					fVideoView->SetTool(tool);
			}
			break;
		}
		case B_CLIPBOARD_CHANGED:
			_UpdateClipboardItems();
			break;
		case MSG_UNDO:
			if (fDocument->WriteLock()) {
				fDocument->CommandStack()->Undo();
				fDocument->WriteUnlock();
			}
			break;
		case MSG_REDO:
			if (fDocument->WriteLock()) {
				fDocument->CommandStack()->Redo();
				fDocument->WriteUnlock();
			}
			break;

		case MSG_PLAYBACK_FORCE_UPDATE:
		{
			if (fDocument->CurrentFrame()->BeingDragged())
				break;
			if (!fDocument->Playlist())
				break;
			if (!fPlaybackManager || !fPlaybackManager->Lock())
				break;
			if (fPlaybackManager->IsPlaying()) {
				fPlaybackManager->Unlock();
				break;
			}
			// force an update of the animated properties
			int64 frame = fDocument->CurrentFrame()->VirtualFrame();
			fDocument->Playlist()->SetCurrentFrame(frame);
			// force an update of the rendered video
			BMessage update(MSG_PLAYBACK_FORCE_UPDATE);
			update.AddInt64("frame", frame);
			fPlaybackManager->PostMessage(&update);

			fPlaybackManager->Unlock();
			break;
		}

		// clip stuff
		case MSG_CLIPS_SELECTED:
		{
			bool gotSelection = fClipListView->CurrentSelection(0) >= 0;
			fRemoveClipsMI->SetEnabled(gotSelection);
			fDuplicateClipsMI->SetEnabled(gotSelection);
			bool editingSupported = false;
			bool updatingExternalDataSupported = false;
			if (gotSelection) {
				ClipListItem* item
					= dynamic_cast<ClipListItem*>(fClipListView->ItemAt(
					 	fClipListView->CurrentSelection(0)));

				if (item) {
					// we support editing these types of clips:
					BitmapClip* bitmapClip
						= dynamic_cast<BitmapClip*>(item->clip);
					SlideShowPlaylist* slideShowClip
						= dynamic_cast<SlideShowPlaylist*>(item->clip);
					if (item->clip) {
						updatingExternalDataSupported
							= item->clip->IsExternalData();
					}

					editingSupported = bitmapClip || slideShowClip;
				}
			}
			fEditClipMI->SetEnabled(editingSupported);
			fUpdateClipDataMI->SetEnabled(updatingExternalDataSupported);
			break;
		}
		case MSG_CLIP_INVOKED:
		{
			// like MSG_EDIT_CLIPS but invoked through double click
			// with a pointer to the exact clip contained in the message
			Clip* clip;
			if (message->FindPointer("clip", (void**)&clip) < B_OK) {
				printf("MSG_CLIP_INVOKED - no clip\n");
				break;
			}
			AutoReadLocker locker(fObjectManager->Locker());
			if (!locker.IsLocked()) {
				printf("MSG_CLIP_INVOKED - no lock\n");
				break;
			}
			if (!fObjectManager->HasObject(clip)) {
				printf("MSG_CLIP_INVOKED - clip not in library\n");
				break;
			}

			// reference clip before unlocking
			Reference reference(clip);
			locker.Unlock();

			_EditClip(clip);
			break;
		}
		case MSG_SELECT_AND_SHOW_CLIP:
		{
			Clip* clip;
			if (message->FindPointer("clip", (void**)&clip) == B_OK)
				fDocument->ClipSelection()->Select(clip);
			break;
		}

		case MSG_CREATE_OBJECT:
			be_app->PostMessage(message);
			break;
		case MSG_REMOVE_CLIPS:
			_RemoveSelectedClips();
			break;
		case MSG_DUPLICATE_CLIPS:
			_DuplicateSelectedClips();
			break;
		case MSG_EDIT_CLIPS:
			_EditSelectedClips();
			break;
		case MSG_UPDATE_CLIP_DATA:
			_UpdateSelectedClips();
			break;
		case MSG_FILE_FOR_CLIP_UPDATE:
			_UpdateSelectedClipsFromFile(message);
			break;

		case MSG_RENDER_PLAYLIST:
			be_app->PostMessage(message);
			break;

		case B_CUT:
		case B_COPY:
		case B_PASTE:
			if (message->HasBool("ignore")) {
				// this message was here already, prevent
				// cyclic message posting...
				break;
			}
			message->AddBool("ignore", true);
			if (BTextView* focus = dynamic_cast<BTextView*>(CurrentFocus())) {
				PostMessage(message, focus);
			} else {
				PostMessage(message, fTimelineView);
			}
			break;
		case MSG_FOCUS_CHANGED:
		{
			if (fIgnoreFocusChange)
				break;
			BView* source;
			bool focus;
			if (message->FindPointer("source", (void**)&source) == B_OK
				&& message->FindBool("focus", &focus) == B_OK) {
				fTextViewFocused = focus && dynamic_cast<BTextView*>(source);
				_UpdateClipboardItems();
			}
			break;
		}
		default:
			BWindow::MessageReceived(message);
	}
}

// QuitRequested
bool
MainWindow::QuitRequested()
{
	fApp->PostMessage(B_QUIT_REQUESTED);
	return false;
}

// MenusBeginning
void
MainWindow::MenusBeginning()
{
	fIgnoreFocusChange = true;
	BWindow::MenusBeginning();
}

// MenusEnded
void
MainWindow::MenusEnded()
{
	BWindow::MenusEnded();
	fIgnoreFocusChange = false;
}

// #pragma mark -

// ObjectChanged
void
MainWindow::ObjectChanged(const Observable* object)
{
	if (!fDocument)
		return;

	if (!Lock())
		return;

	if (object == fDocument->CommandStack()) {
		// relable Undo item and update enabled status
		BString label("Undo");
		fUndoMI->SetEnabled(fDocument->CommandStack()->GetUndoName(label));
		fUndoIB->SetEnabled(fUndoMI->IsEnabled());
		if (fUndoMI->IsEnabled())
			fUndoMI->SetLabel(label.String());
		else
			fUndoMI->SetLabel("<nothing to undo>");

		// relable Redo item and update enabled status
		label.SetTo("Redo");
		fRedoMI->SetEnabled(fDocument->CommandStack()->GetRedoName(label));
		fRedoIB->SetEnabled(fRedoMI->IsEnabled());
		if (fRedoMI->IsEnabled())
			fRedoMI->SetLabel(label.String());
		else
			fRedoMI->SetLabel("<nothing to redo>");

		PostMessage(MSG_PLAYBACK_FORCE_UPDATE);
	}

	else if (object == fDocument->ClipSelection()
			|| object == fDocument->PlaylistSelection()
			|| object == fDocument->VideoViewSelection()) {

		const Selection* selection = dynamic_cast<const Selection*>(object);
		// point the property list view to the first item of the selection
		int32 count = selection->CountSelected();
		Selectable* lastSelected = selection->SelectableAt(count - 1);
		fPlaylistItemsSelected = false;
		if (lastSelected) {
			// also means, we have a selection at all
			fPropertyListView->SetTo(lastSelected->GetPropertyObject());

			if (object == fDocument->ClipSelection()) {
				Clip* clip = dynamic_cast<Clip*>(lastSelected);
				fClipGroup->MakeSureClipShows(clip);
			}

			// make sure, there is nothing selected in the other selections
			// NOTE: this will trigger new notifications which utlimately
			// enter this code before the rest is executed below!
			if (selection != fDocument->ClipSelection())
				fDocument->ClipSelection()->DeselectAll();
			if (selection != fDocument->PlaylistSelection()) {
				// the video view selection has an associated
				// selectable item in the playlist selection,
				// if that item is still contained in the playlist
				// selection, then we should not deselect anything
				bool deselect = true;
				if (selection == fDocument->VideoViewSelection()) {
					Selection* playlistSelection
						= fDocument->PlaylistSelection();
					int32 playlistSelectionCount
						= playlistSelection->CountSelected();
					for (int32 i = 0; i < playlistSelectionCount; i++) {
						Selectable* playlistSelectable
							= playlistSelection->SelectableAt(i);
						if (playlistSelectable
							== fDocument->VideoViewSelection()
								->AssociatedSelectable()) {
							deselect = false;
							break;
						}
					}
				}
				if (deselect)
					fDocument->PlaylistSelection()->DeselectAll();
			}
			if (selection != fDocument->VideoViewSelection())
				fDocument->VideoViewSelection()->DeselectAll();

			// find out if at least one PlaylistItem is selected
			count = selection->CountSelected();
			for (int32 i = 0; i < count; i++) {
				if (dynamic_cast<PlaylistItem*>(
						selection->SelectableAtFast(i))) {
					fPlaylistItemsSelected = true;
					break;
				}
			}

		} else {
			if (fDocument->ClipSelection()->IsEmpty()
				&& fDocument->PlaylistSelection()->IsEmpty()
				&& fDocument->VideoViewSelection()->IsEmpty()) {
				// suspend deselecting in the property
				// list view until all three selections are empty
				fPropertyListView->SetTo(NULL);
			}
		}

		_UpdateClipboardItems();

		fDuplicateMI->SetEnabled(fPlaylistItemsSelected);
		fRemoveMI->SetEnabled(fPlaylistItemsSelected);
		fDeleteIB->SetEnabled(fPlaylistItemsSelected);
		fCutIB->SetEnabled(fPlaylistItemsSelected);
	}

	else if (object == fDocument->CurrentFrame()) {
		int64 frame = fDocument->CurrentFrame()->Frame();
		// update editor video view
		fVideoView->SetCurrentFrame(frame);
		// update all animators to the new current frame
		if (fDocument->Playlist())
			fDocument->Playlist()->SetCurrentFrame(frame);
	}

	else if (const Playlist* pl = dynamic_cast<const Playlist*>(object)) {
		// if this is the current document playlist, wee need to
		// check the video resolution
// TODO: is there a chance for deadlocking here?
		if (pl == fDocument->Playlist() && fPlaybackManager->Lock()) {
			int32 videoWidth = pl->Width();
			int32 videoHeight = pl->Height();
			BRect videoBounds(0, 0, videoWidth - 1, videoHeight - 1);
			if (videoBounds != fPlaybackManager->MovieBounds()) {
				fPlaybackManager->StopPlaying();
				// TODO: something is fishy with the locking, if we don't wait,
				// the frame_generator thread in the VideoProducer can cause
				// a deadlock when trying the format switch later
				snooze(40000);

				fVideoView->SetBitmap(NULL);
				fVideoView->DisableOverlay();

				fPlaybackManager->FormatChanged(videoBounds,
					pl->VideoFrameRate());
			}
			fPlaybackManager->Unlock();

			fTopView->SetVideoSize(videoWidth,
				(float)videoWidth / (float)videoHeight);
			fVideoView->SetVideoSize(videoWidth, videoHeight);
		}
	}

	Unlock();
}

// ObjectAdded
void
MainWindow::ObjectAdded(ServerObject* object, int32 index)
{
}

// ObjectRemoved
void
MainWindow::ObjectRemoved(ServerObject* object)
{
}

// #pragma mark -

// SetPlaylist
void
MainWindow::SetPlaylist(Playlist* playlist)
{
	if (!fPlaybackManager->Lock()) {
		printf("MainWindow::SetPlaylist() - "
			"unable to lock playback manager\n");
		return;
	}

	fPlaybackManager->StopPlaying();

	AutoWriteLocker locker(fDocument);
	Playlist* previousPlaylist = fTimelineView->Playlist();
	if (previousPlaylist)
		previousPlaylist->RemoveObserver(this);

	if (playlist) {
		// cause a format change if applicable
		uint32 videoWidth = playlist->Width();
		uint32 videoHeight = playlist->Height();
		BRect videoBounds(0, 0, videoWidth - 1, videoHeight - 1);
		if (videoBounds != fPlaybackManager->MovieBounds()) {
			// TODO: something is fishy with the locking, if we don't wait,
			// the frame_generator thread in the VideoProducer can cause
			// a deadlock when trying the format switch later
			snooze(40000);
			// make sure the video view doesn't have the overlay bitmap
			// set anymore! otherwise we cannot get overlay to work in
			// another format
			fVideoView->SetBitmap(NULL);
			fVideoView->DisableOverlay();

			fPlaybackManager->FormatChanged(videoBounds,
				playlist->VideoFrameRate());

			// relayout views to accomodate new video size
			fTopView->SetVideoSize(videoWidth, (float)videoWidth
				/ (float)videoHeight);
			fVideoView->SetVideoSize(videoWidth, videoHeight);
			// only reset auto zoom if playlist size changed
			// user might want to keep zoom level otherwise to
			// compare something zoomed in
			fVideoView->SetAutoZoomToAll(true);
		}

		playlist->AddObserver(this);
	}

	fClipListView->SetPlaylist(playlist);
	fTimelineView->SetPlaylist(playlist);
	fTrackView->SetPlaylist(playlist);
	fTrackHeaderView->SetPlaylist(playlist);

	fVideoView->SetPlaylist(playlist);
	fPlaybackManager->SetPlaylist(playlist);

	locker.Unlock();
	fPlaybackManager->Unlock();
}

// SetObjectManager
void
MainWindow::SetObjectManager(ServerObjectManager* manager)
{
	if (fObjectManager == manager)
		return;

	if (fObjectManager) {
		AutoWriteLocker locker(fObjectManager->Locker());
		fObjectManager->RemoveListener(this);
	}

	fObjectManager = manager;
	AutoWriteLocker locker(manager ? manager->Locker() : NULL);

	// hook up some views to clip library
	fClipListView->SetObjectLibrary(fObjectManager);
	fTimelineView->SetClipLibrary(fObjectManager);

	if (fObjectManager)
		fObjectManager->AddListener(this);
}

// PrepareForQuit
void
MainWindow::PrepareForQuit()
{
	Hide();

	SetPlaylist(NULL);

	// work arround a stupid BListView bug
	fClipListView->SetTarget(NULL);
	fClipListView->SetSelection(NULL);
	fClipListView->SetObjectLibrary(NULL);

	fPropertyListView->RemoveListener(fTimeView);

	fTimelineView->Clipboard()->StopWatching(BMessenger(this, this));

	// there are views in the hierarchy that remove themselfs
	// as observer from objects that are deleted here, so it
	// would be too late to delete these views in the BWindow
	// destructor
	fPlaybackManager->Lock();
	fPlaybackManager->SetVCTarget(NULL);
	fVideoView->SetBitmap(NULL);

	fTimelineView->SetTool(NULL);
	fVideoView->SetTool(NULL);

	fTopView->RemoveSelf();
	delete fTopView;

	// shutdown playback framework
	fPlaybackManager->Quit();

	// Don't process any pending messages after having just deleted the GUI...
	fPreparedForQuit = true;
}

// AddTool
void
MainWindow::AddTool(TimelineTool* tool, bool addIcon)
{
	if (!tool)
		return;

	int32 count = fTimelineTools.CountItems();
		// check the count before adding tool

	if (!fTimelineTools.AddItem((void*)tool)) {
		delete tool;
		return;
	}

	if (addIcon) {
		// add the tools icon
		IconButton* icon = tool->Icon();
		BMessage* message = new BMessage(MSG_SET_TOOL);
		message->AddInt32("tool", count);
		icon->SetMessage(message);
		fToolIconContainer->AddOption(icon);
	}

	if (count == 0) {
		// this was the first tool
		fTimelineView->SetTool(tool);
	}
}

// AddTool
void
MainWindow::AddTool(StageTool* tool)
{
	if (!tool)
		return;

	int32 count = fStageTools.CountItems();
		// check the count before adding tool

	if (!fStageTools.AddItem((void*)tool)) {
		delete tool;
		return;
	}

	// add the tools icon
	IconButton* icon = tool->Icon();
	BMessage* message = new BMessage(MSG_SET_STAGE_TOOL);
	message->AddInt32("tool", count);
	icon->SetMessage(message);
	fStageToolIconContainer->AddOption(icon);

	if (count == 0) {
		// this was the first tool
		fVideoView->SetTool(tool);
	}
}

// #pragma mark -

// StoreSettings
void
MainWindow::StoreSettings(BMessage* archive)
{
	if (archive->ReplaceFloat("video scale", fTopView->VideoScale()) < B_OK)
		archive->AddFloat("video scale", fTopView->VideoScale());

	if (archive->ReplaceFloat("list prop", fTopView->ListProportion()) < B_OK)
		archive->AddFloat("list prop", fTopView->ListProportion());

	if (archive->ReplaceFloat("track prop", fTopView->TrackProportion())
			< B_OK) {
		archive->AddFloat("track prop", fTopView->TrackProportion());
	}

	if (archive->ReplaceFloat("volume", fTransportGroup->Volume()) < B_OK)
		archive->AddFloat("volume", fTransportGroup->Volume());

	int32 i = fToolIconContainer->Value();
	if (archive->ReplaceInt32("timeline tool", i) < B_OK)
		archive->AddInt32("timeline tool", i);

	i = fStageToolIconContainer->Value();
	if (archive->ReplaceInt32("stage tool", i) < B_OK)
		archive->AddInt32("stage tool", i);

	i = fDocument->LoopMode()->Mode();
	if (archive->ReplaceInt32("loop mode", i) < B_OK)
		archive->AddInt32("loop mode", i);

	i = fClipGroup->ClipType();
	if (archive->ReplaceInt32("show clip type", i) < B_OK)
		archive->AddInt32("show clip type", i);

	bool b = fClipGroup->PlaylistClipsOnly();
	if (archive->ReplaceBool("show playlist clips only", b) < B_OK)
		archive->AddBool("show playlist clips only", b);

	b = fClipGroup->NameContainsOnly();
	if (archive->ReplaceBool("show name contains clips only", b) < B_OK)
		archive->AddBool("show name contains clips only", b);

	const char* string = fClipGroup->NameContainsString();
	if (archive->ReplaceString("show name contains clips string",
		string) < B_OK)
		archive->AddString("show name contains clips string", string);

}

// RestoreSettings
void
MainWindow::RestoreSettings(BMessage* archive)
{
	float f;

	if (archive->FindFloat("video scale", &f) >= B_OK)
		fTopView->SetVideoScale(f);

	if (archive->FindFloat("list prop", &f) >= B_OK)
		fTopView->SetListProportion(f);

	if (archive->FindFloat("track prop", &f) >= B_OK)
		fTopView->SetTrackProportion(f);

	if (archive->FindFloat("volume", &f) >= B_OK) {
		if (fPlaybackManager->Lock()) {
			fPlaybackManager->SetVolume(f);
			fPlaybackManager->Unlock();
		}
		fTransportGroup->SetVolume(f);
	}

	int32 i;

	if (archive->FindInt32("timeline tool", &i) >= B_OK) {
		if (TimelineTool* tool = (TimelineTool*)fTimelineTools.ItemAt(i)) {
			fToolIconContainer->SetValue(i);
			fTimelineView->SetTool(tool);
		}
	}

	if (archive->FindInt32("stage tool", &i) >= B_OK) {
		if (StageTool* tool = (StageTool*)fStageTools.ItemAt(i)) {
			fStageToolIconContainer->SetValue(i);
			fVideoView->SetTool(tool);
		}
	}

	if (archive->FindInt32("loop mode", &i) >= B_OK)
		fDocument->LoopMode()->SetMode(i);

	if (archive->FindInt32("show clip type", &i) >= B_OK)
		fClipGroup->SetClipType(i);

	bool b;
	if (archive->FindBool("show playlist clips only", &b) >= B_OK)
		fClipGroup->SetPlaylistClipsOnly(b);

	if (archive->FindBool("show name contains clips only", &b) >= B_OK)
		fClipGroup->SetNameContainsOnly(b);

	const char* string;
	if (archive->FindString("show name contains clips string",
		&string) >= B_OK)
		fClipGroup->SetNameContainsString(string);
}

// #pragma mark -

// _Init
void
MainWindow::_Init()
{
	BRect videoBounds(0, 0, 683, 383);
	float videoFrameRate = 25.0;
	if (fDocument->Playlist()) {
		videoBounds = BRect(0, 0, fDocument->Playlist()->Width() - 1,
							fDocument->Playlist()->Height() - 1);

		videoFrameRate = fDocument->Playlist()->VideoFrameRate();
	}

	// create the GUI
	fTopView = _CreateGUI(Bounds(), videoBounds);
	AddChild(fTopView);

	// init layout of top view
	uint32 videoWidth = videoBounds.IntegerWidth() + 1;
	uint32 videoHeight = videoBounds.IntegerHeight() + 1;
	fTopView->SetVideoSize(videoWidth, (float)videoWidth / videoHeight);

	// take some additional care of the scroll bar that scrolls
	// the clip list view
	if (BScrollBar* scrollBar = fClipListView->ScrollBar(B_VERTICAL)) {
		scrollBar->MoveBy(0, -1);
		scrollBar->ResizeBy(0, 2);
	}

	// set some arbitrary size limits (TODO: real layout stuff)
	float minWidth, maxWidth, minHeight, maxHeight;
	GetSizeLimits(&minWidth, &maxWidth, &minHeight, &maxHeight);
	minWidth = 780;
	minHeight = 570;
	SetSizeLimits(minWidth, maxWidth, minHeight, maxHeight);

	// create and run the media node playback frame work
	fPlaybackManager = new PlaylistPlaybackManager(fVideoView,
												   fDocument);

	fPlaybackManager->Init(videoBounds, videoFrameRate, LOOPING_RANGE, true);
	fPlaybackManager->SetPlaylist(fDocument->Playlist());
	PeakView* peakView = fTransportGroup->GetPeakView();
	peakView->SetPeakNotificationWhat(MSG_PEAK_NOTIFICATION);
	fPlaybackManager->SetPeakListener(BMessenger(peakView));
	fPlaybackManager->Run();

// TODO: Read from playlist settings
fDocument->PlaybackRange()->SetFirstFrame(0);
fDocument->PlaybackRange()->SetLastFrame(100);
	// hook up the playback manager to the control group
	fTransportGroup->SetPlaybackManager(fPlaybackManager);
	fTransportGroup->SetPlaybackRange(fDocument->PlaybackRange());
	fTransportGroup->SetDisplayRange(fDocument->DisplayRange());
	fTransportGroup->SetLoopMode(fDocument->LoopMode());

	// hook up the clip listview
	fClipListView->SetSelection(fDocument->ClipSelection());
	fClipListView->SetPlaylist(fDocument->Playlist());

	// hook up the time line view to the document
	fTimelineView->SetTimeView(fTimeView);
	fTimelineView->SetTrackView(fTrackView);
	fTimelineView->SetPlaylist(fDocument->Playlist());
	fTimelineView->SetLocker(fDocument);
	fTimelineView->SetCommandStack(fDocument->CommandStack());
	fTimelineView->SetCurrentFrame(fDocument->CurrentFrame());
	fTimelineView->SetDisplayRange(fDocument->DisplayRange());
	fTimelineView->SetPlaybackManager(fPlaybackManager);
	fTimelineView->SetLoopMode(fDocument->LoopMode());
	fTimelineView->SetSelection(fDocument->PlaylistSelection());
	fTimelineView->SetUpdateTarget(this, MSG_PLAYBACK_FORCE_UPDATE);
	fTimelineView->SetCatchAllEventsKinds(StateView::KEY_EVENTS
		| StateView::MODIFIER_EVENTS | StateView::MOUSE_WHEEL_EVENTS);

	fTrackView->SetTimelineView(fTimelineView);
	fTrackView->SetPlaylist(fDocument->Playlist());
	fTrackView->SetCommandStack(fDocument->CommandStack());
	fTrackHeaderView->SetPlaylist(fDocument->Playlist());
	fTrackView->SetCommandStack(fDocument->CommandStack());

	fLoopModeControl->SetLoopMode(fDocument->LoopMode());

	// adjust target of some menu items
	fCutMI->SetTarget(this);
	fCopyMI->SetTarget(this);
	fPasteMI->SetTarget(this);
	fDuplicateMI->SetTarget(fTimelineView);
	fRemoveMI->SetTarget(fTimelineView);

	// subscribe to clipboard events and sync to initial contents
	fTimelineView->Clipboard()->StartWatching(BMessenger(this, this));
	_UpdateClipboardItems();

	// hook up the time view to the document
	fTimeView->SetDisplayRange(fDocument->DisplayRange());
	fTimeView->SetPlaybackRange(fDocument->PlaybackRange());
	fTimeView->SetLoopMode(fDocument->LoopMode());
	fTimeView->SetCommandStack(fDocument->CommandStack());
	fTimeView->SetInsets(0, B_V_SCROLL_BAR_WIDTH - 1);
	fTimeView->SetTimelineView(fTimelineView);
		// this makes sure that the frame<->pos conversion
		// is guaranteed to be the same in TimeView and TimelineView...

	// hook up the track view to the timeline view
	fTrackView->SetTimelineView(fTimelineView);

	// hook up the video view to the document
	fVideoView->SetVideoSize(videoWidth, videoHeight);
	fVideoView->SetPlaylist(fDocument->Playlist());
	fVideoView->SetSelection(fDocument->PlaylistSelection(),
							 fDocument->VideoViewSelection());
	fVideoView->SetLocker(fDocument);
	fVideoView->SetCommandStack(fDocument->CommandStack());
	fVideoView->SetUpdateTarget(this, MSG_PLAYBACK_FORCE_UPDATE);
	fVideoView->SetCatchAllEventsKinds(StateView::MODIFIER_EVENTS
		| StateView::MOUSE_WHEEL_EVENTS);

	// hook up the property list view to the command stack
	fPropertyListView->SetCommandStack(fDocument->CommandStack());
	// allow the property list view to install some menu items
	fPropertyListView->SetMenu(fPropertyMenu);
	// allow the property list view to know the current playback frame
	fPropertyListView->SetCurrentFrame(fDocument->CurrentFrame());
	// hook up time view to property listview, so that it displays
	// the keyframes of the currently selected property
	fPropertyListView->AddListener(fTimeView);

	// connect to documents UndoStack and Selection
	fDocument->CommandStack()->AddObserver(this);

	fDocument->ClipSelection()->AddObserver(this);
	fDocument->PlaylistSelection()->AddObserver(this);
	fDocument->VideoViewSelection()->AddObserver(this);

	fDocument->CurrentFrame()->AddObserver(this);

	// init command icon
	fCutIB->SetMessage(new BMessage(MSG_CUT_ITEMS));
	fCutIB->SetTarget(fTimelineView);
	fCutIB->SetEnabled(false);

	fDeleteIB->SetMessage(new BMessage(MSG_REMOVE_ITEMS));
	fDeleteIB->SetTarget(fTimelineView);
	fDeleteIB->SetEnabled(false);

	// init zoom icons
	fZoomOutIB->SetMessage(new BMessage(MSG_ZOOM_OUT));
	fZoomOutIB->SetTarget(fTimelineView);
	fZoomInIB->SetMessage(new BMessage(MSG_ZOOM_IN));
	fZoomInIB->SetTarget(fTimelineView);

	// init undo icons
	fUndoIB->SetMessage(new BMessage(MSG_UNDO));
	fUndoIB->SetTarget(this);
	fRedoIB->SetMessage(new BMessage(MSG_REDO));
	fRedoIB->SetTarget(this);

	// init tools
	_InitTimelineTools();
	_InitStageTools();

	// init additional shortcuts
	AddShortcut('Y', 0, new BMessage(MSG_UNDO));
	AddShortcut('Y', B_SHIFT_KEY, new BMessage(MSG_REDO));
}

// _InitTimelineTools
void
MainWindow::_InitTimelineTools()
{
	// create timeline tools
	AddTool(new PickTool(), false);
//	AddTool(new CutTool(), false);
//	AddTool(new DeleteTool(), false);
}

// _InitStageTools
void
MainWindow::_InitStageTools()
{
	// create stage tools
	AddTool(new NoneTool());
	AddTool(new TransformTool());
	AddTool(new EditOnStageTool());
}

#include "AllocationChecker.h"

// _CreateGUI
TopView*
MainWindow::_CreateGUI(BRect bounds, BRect videoSize)
{
	// create container for all other views
	TopView* bg = new TopView(bounds);

	videoSize.right += B_V_SCROLL_BAR_WIDTH;
	videoSize.bottom += B_H_SCROLL_BAR_HEIGHT;

	BRect menuFrame(bounds);
	menuFrame.right = bounds.right - (videoSize.Width() + 1);
	fMenuBar = _CreateMenuBar(menuFrame);
	bg->AddMenuBar(fMenuBar);
	fMenuBar->ResizeToPreferred();

	// clip list view
	fClipListView = new ClipListView("clip library",
		new BMessage(MSG_CLIPS_SELECTED), new BMessage(MSG_CLIP_INVOKED),
		this);
	fClipGroup = new ClipGroup(fClipListView);

	fPropertyListView = new PropertyListView(fDocument);

	// transport control group
	fTransportGroup = new PlaybackController(BRect(0, 0, 100, 20),
											 fDocument->CurrentFrame());

	float width, height;
	fTransportGroup->GetPreferredSize(&width, &height);
	fTransportGroup->ResizeTo(width, height);

	// video view
	// layout to the right of clip list view
	videoSize.right -= B_V_SCROLL_BAR_WIDTH;
	videoSize.bottom -= B_H_SCROLL_BAR_HEIGHT;
	fVideoView = new EditorVideoView(videoSize, "video view");

	fVideoView->SetResizingMode(B_FOLLOW_RIGHT | B_FOLLOW_TOP);

	videoSize.bottom += B_H_SCROLL_BAR_HEIGHT;
	videoSize.right += B_V_SCROLL_BAR_WIDTH;
	ScrollView* videoScrollView = new ScrollView(fVideoView,
		SCROLL_HORIZONTAL | SCROLL_VERTICAL
			| SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS,
		videoSize, "video scroll view", B_FOLLOW_ALL,
		B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER);

	bg->AddVideoView(fVideoView, videoScrollView);
	bg->AddTransportGroup(fTransportGroup);
	bg->AddClipListGroup(fClipGroup);

	// scroll view around property list view
	bounds.left = ceilf(menuFrame.Width() / 2.5) + B_V_SCROLL_BAR_WIDTH + 2;
	bounds.right = bg->Bounds().right - (videoSize.Width() + 1);
	bounds.top = fMenuBar->Frame().bottom + 1;
	bounds.bottom = bg->Bounds().top + videoSize.Height();

	BGroupView* propertyGroup = new BGroupView(B_HORIZONTAL, 0);
	propertyGroup->AddChild(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER));
	propertyGroup->AddChild(new ScrollView(fPropertyListView, SCROLL_VERTICAL,
		bounds, "property scroll view", B_FOLLOW_NONE,
		B_WILL_DRAW | B_FRAME_EVENTS, B_PLAIN_BORDER, BORDER_RIGHT));
	bg->AddPropertyListGroup(propertyGroup);


	// area below clip listview
	// used for tool icons and command icons
	bounds.left = 0;
	bounds.top = 0;
	bounds.bottom = height;

	// tool icon bar
	BRect toolBounds = bounds;
	toolBounds.top += 2;
	toolBounds.left = 0;
	toolBounds.bottom -= 1;
	toolBounds.right = 0;
		// we don't know how big the tool area might grow...
		// TODO: use layouting for that
	fToolIconContainer = new IconOptionsControl("tool icon container", NULL,
		new BMessage(MSG_SET_TOOL), this);
	fToolIconContainer->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);
	fToolIconContainer->MoveTo(toolBounds.LeftTop());
	fToolIconContainer->ResizeTo(toolBounds.Width(), toolBounds.Height());
		// see note above about real layouting...

	// command icon bar
	BRect commandBounds = toolBounds;
	commandBounds.OffsetTo(toolBounds.RightTop() + BPoint(2, 0));
	fCommandIconContainer = new Group(commandBounds, "command icon container");
	fCommandIconContainer->SetResizingMode(B_FOLLOW_LEFT | B_FOLLOW_TOP);

	// cut
	fCutIB = new IconButton("cut icon", 0, NULL, NULL);
	fCutIB->SetIcon(kCutIcon, kIconWidth, kIconHeight, kIconFormat);
	fCutIB->ResizeToPreferred();
	fCommandIconContainer->AddChild(fCutIB);

	// delete
	fDeleteIB = new IconButton("delete icon", 0, NULL, NULL);
	fDeleteIB->SetIcon(kTrashIcon, kIconWidth, kIconHeight, kIconFormat);
	fDeleteIB->ResizeToPreferred();
	fCommandIconContainer->AddChild(fDeleteIB);

	// zoom out
	fZoomOutIB = new IconButton("zoom out icon", 0, NULL, NULL);
	fZoomOutIB->SetIcon(kZoomOutIcon, kIconWidth, kIconHeight, kIconFormat);
	fZoomOutIB->ResizeToPreferred();
	fCommandIconContainer->AddChild(fZoomOutIB);

	// zoom in
	fZoomInIB = new IconButton("zoom in icon", 0, NULL, NULL);
	fZoomInIB->SetIcon(kZoomInIcon, kIconWidth, kIconHeight, kIconFormat);
	fZoomInIB->ResizeToPreferred();
	fCommandIconContainer->AddChild(fZoomInIB);

	// undo
	fUndoIB = new IconButton("undo icon", 0, NULL, NULL);
	fUndoIB->SetIcon(kUndoIcon, kIconWidth, kIconHeight, kIconFormat);
	fUndoIB->ResizeToPreferred();
	fUndoIB->SetEnabled(false);
	fCommandIconContainer->AddChild(fUndoIB);

	// redo
	fRedoIB = new IconButton("redo icon", 0, NULL, NULL);
	fRedoIB->SetIcon(kRedoIcon, kIconWidth, kIconHeight, kIconFormat);
	fRedoIB->ResizeToPreferred();
	fRedoIB->SetEnabled(false);
	fCommandIconContainer->AddChild(fRedoIB);

	fCommandIconContainer->ResizeToPreferred();
	commandBounds = fCommandIconContainer->Frame();
		// we know how big the command icon container needs to be

	// loop mode popup
	BRect loopModeBounds = commandBounds;
	loopModeBounds.OffsetTo(commandBounds.RightTop() + BPoint(15, 0));
	fLoopModeControl = new LoopModeControl();
	float loopWidth = max_c(fLoopModeControl->Frame().Width(),
		loopModeBounds.Width());
	float loopHeight = fLoopModeControl->Frame().Height();
	fLoopModeControl->MoveTo(BPoint(loopModeBounds.left, loopModeBounds.top
		+ (loopModeBounds.Height() - loopHeight) / 2));
	fLoopModeControl->ResizeTo(loopWidth, loopHeight);

	bounds.right = fLoopModeControl->Frame().right + 5;
	BView* iconBar = new IconBar(bounds);
	iconBar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	iconBar->AddChild(fToolIconContainer);
	iconBar->AddChild(fCommandIconContainer);
	iconBar->AddChild(fLoopModeControl);

	bg->AddIconBar(iconBar);


	// stage tool icon bar
	bounds.left = 0;
	bounds.top = 0;
	bounds.right = height;
	bounds.bottom = videoSize.bottom;

	toolBounds.left = 2;
	toolBounds.top = 5;
	toolBounds.right = bounds.right - 4;
	toolBounds.bottom = bounds.bottom - 1;
	fStageToolIconContainer = new IconOptionsControl(
		"stage tool icon container", NULL,
		new BMessage(MSG_SET_STAGE_TOOL), this, B_VERTICAL);
	fStageToolIconContainer->SetResizingMode(
		B_FOLLOW_LEFT | B_FOLLOW_TOP_BOTTOM);
	fStageToolIconContainer->MoveTo(toolBounds.LeftTop());
	fStageToolIconContainer->ResizeTo(toolBounds.Width(), toolBounds.Height());

	iconBar = new IconBar(bounds, B_VERTICAL);
	iconBar->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	iconBar->AddChild(fStageToolIconContainer);

	bg->AddStageIconBar(iconBar);

	// time view
	fTimeView = new TimeView(fDocument->CurrentFrame());
	fTimeView->SetExplicitMinSize(BSize(10.0f, 20.0f));
	fTimeView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, 20.0f));

	// time line view
	fTimelineView = new TimelineView("timeline view");

	ScrollView* scrollView = new ScrollView(fTimelineView,
		SCROLL_HORIZONTAL | SCROLL_VERTICAL
			| SCROLL_VISIBLE_RECT_IS_CHILD_BOUNDS,
		"timeline scroll view", B_WILL_DRAW | B_FRAME_EVENTS, B_NO_BORDER);
	BGroupView* timelineGroup = new BGroupView(B_HORIZONTAL, 0);
	BGroupLayoutBuilder(timelineGroup->GroupLayout())
		.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER))
		.AddGroup(B_VERTICAL, 0)
			.Add(fTimeView, 0.0f)
			.Add(scrollView)
		.End()
	;
	bg->AddTimelineGroup(timelineGroup);

	// track group
	bounds = BRect(0, 0, 100, 19);
	fTrackHeaderView = new TrackHeaderView(bounds);

	// track view
	fTrackView = new TrackView(bounds);

	// track info view
	bounds.bottom = B_H_SCROLL_BAR_HEIGHT - 1;
	fTrackInfoView = new TrackInfoView(bounds);

	BGroupView* trackGroup = new BGroupView(B_HORIZONTAL, 0);
	BGroupLayoutBuilder(trackGroup->GroupLayout())
		.AddGroup(B_VERTICAL, 0)
			.Add(fTrackHeaderView, 0.0f)
			.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
			.Add(fTrackView)
			.Add(new BSeparatorView(B_HORIZONTAL, B_PLAIN_BORDER))
			.Add(fTrackInfoView, 0.0f)
		.End()
		.Add(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER));
	;

	bg->AddTrackGroup(trackGroup);

//AllocationChecker::GetDefault()->CheckWalls();

	return bg;
}

// _CreateMenuBar
BMenuBar*
MainWindow::_CreateMenuBar(BRect frame)
{
	BMenuBar* menuBar = new BMenuBar(frame, "menu bar");

	BMenuItem* item;
	BMessage* message;

	// Document
	fPlaylistMenu = new BMenu("Playlist");
	item = new BMenuItem("New", new BMessage(MSG_NEW));
	fPlaylistMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Save Objects", new BMessage(MSG_SAVE), 'S');
	fPlaylistMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Re-Init Dependencies",
		new BMessage(MSG_RESOLVE_DEPENDENCIES), 'D', B_SHIFT_KEY);
	fPlaylistMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Render"B_UTF8_ELLIPSIS,
		new BMessage(MSG_RENDER_PLAYLIST), 'R');
	fPlaylistMenu->AddItem(item);
	item->SetTarget(this);

	fExportImageMI = new BMenuItem("Export Playback Snapshot",
		new BMessage(MSG_EXPORT_PLAYBACK_SNAPSHOT), 'S',
		B_SHIFT_KEY | B_COMMAND_KEY);
	fPlaylistMenu->AddItem(fExportImageMI);
	fExportImageMI->SetTarget(this);

	fPlaylistMenu->AddSeparatorItem();

	item = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED), 'Q');
	fPlaylistMenu->AddItem(item);
	item->SetTarget(this);

	menuBar->AddItem(fPlaylistMenu);

	// Edit
	fEditMenu = new BMenu("Edit");
	fUndoMI = new BMenuItem("<nothing to undo>", new BMessage(MSG_UNDO), 'Z');
	fUndoMI->SetEnabled(false);
	fEditMenu->AddItem(fUndoMI);
	fRedoMI = new BMenuItem("<nothing to redo>", new BMessage(MSG_REDO),
							'Z', B_SHIFT_KEY);
	fRedoMI->SetEnabled(false);
	fEditMenu->AddItem(fRedoMI);

	fEditMenu->AddSeparatorItem();

	fCutMI = new BMenuItem("Cut", new BMessage(B_CUT), 'X');
	fCutMI->SetEnabled(false);
	fEditMenu->AddItem(fCutMI);

	fCopyMI = new BMenuItem("Copy", new BMessage(B_COPY), 'C');
	fCopyMI->SetEnabled(false);
	fEditMenu->AddItem(fCopyMI);

	fPasteMI = new BMenuItem("Paste", new BMessage(B_PASTE), 'V');
	fPasteMI->SetEnabled(false);
	fEditMenu->AddItem(fPasteMI);

	fDuplicateMI = new BMenuItem("Duplicate",
								 new BMessage(MSG_DUPLICATE_ITEMS), 'D');
	fDuplicateMI->SetEnabled(false);
	fEditMenu->AddItem(fDuplicateMI);

	fEditMenu->AddSeparatorItem();

	fRemoveMI = new BMenuItem("Remove",
							  new BMessage(MSG_REMOVE_ITEMS),
							  B_DELETE, 0);
	fRemoveMI->SetEnabled(false);
	fEditMenu->AddItem(fRemoveMI);

	fEditMenu->SetTargetForItems(this);

	menuBar->AddItem(fEditMenu);

	// Clip
	fClipMenu = new BMenu("Clip");
	fCreateMenu = new BMenu("Create");

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "Playlist");
	fCreateMenu->AddItem(new BMenuItem("Playlist", message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "SlideShowPlaylist");
	fCreateMenu->AddItem(new BMenuItem("Slide Show Playlist", message));

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "StretchingPlaylist");
	fCreateMenu->AddItem(new BMenuItem("Stretching Playlist", message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "CollectablePlaylist");
	fCreateMenu->AddItem(new BMenuItem("Template Playlist", message));

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "CollectingPlaylist");
	fCreateMenu->AddItem(new BMenuItem("Template Collector", message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "SequenceContainerPlaylist");
	fCreateMenu->AddItem(new BMenuItem("Sequence Container Playlist",
		message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "ColorClip");
	fCreateMenu->AddItem(new BMenuItem("Color", message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "ClockClip");
	fCreateMenu->AddItem(new BMenuItem("Clock", message));

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "TimerClip");
	fCreateMenu->AddItem(new BMenuItem("Timer", message));

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "WeatherClip");
	fCreateMenu->AddItem(new BMenuItem("Weather", message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "ScrollingTextClip");
	fCreateMenu->AddItem(new BMenuItem("Ticker", message));

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "TextClip");
	fCreateMenu->AddItem(new BMenuItem("Text", message));

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "TableClip");
	fCreateMenu->AddItem(new BMenuItem("Table", message));

	fCreateMenu->AddSeparatorItem();

	message = new BMessage(MSG_CREATE_OBJECT);
	message->AddString("type", "ExecuteClip");
	fCreateMenu->AddItem(new BMenuItem("Executer", message));

	fCreateMenu->SetTargetForItems(this);
	fClipMenu->AddItem(fCreateMenu);

	message = new BMessage(MSG_DUPLICATE_CLIPS);
	fDuplicateClipsMI = new BMenuItem("Duplicate", message);
	fDuplicateClipsMI->SetEnabled(false);
	fClipMenu->AddItem(fDuplicateClipsMI);

	message = new BMessage(MSG_EDIT_CLIPS);
	fEditClipMI = new BMenuItem("Edit"B_UTF8_ELLIPSIS, message);
	fEditClipMI->SetEnabled(false);
	fClipMenu->AddItem(fEditClipMI);

	message = new BMessage(MSG_UPDATE_CLIP_DATA);
	fUpdateClipDataMI = new BMenuItem("Update External Data", message);
	fUpdateClipDataMI->SetEnabled(false);
	fClipMenu->AddItem(fUpdateClipDataMI);

	fClipMenu->AddSeparatorItem();

	message = new BMessage(MSG_REMOVE_CLIPS);
	fRemoveClipsMI = new BMenuItem("Remove", message);
	fRemoveClipsMI->SetEnabled(false);
	fClipMenu->AddItem(fRemoveClipsMI);

	menuBar->AddItem(fClipMenu);

	// Property
	fPropertyMenu = new BMenu("Property");
	menuBar->AddItem(fPropertyMenu);

#ifndef CLOCKWERK_STAND_ALONE
	// Window
	fWindowMenu = new BMenu("Window");
	item = new BMenuItem("Network"B_UTF8_ELLIPSIS,
		new BMessage(MSG_NETWORK_SHOW_STATUS), 'N');
	fWindowMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Schedules"B_UTF8_ELLIPSIS,
		new BMessage(MSG_SHOW_SCHEDULES), 'T');
	fWindowMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Client Settings"B_UTF8_ELLIPSIS,
		new BMessage(MSG_SHOW_CLIENT_SETTINGS));
	fWindowMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Display Settings"B_UTF8_ELLIPSIS,
		new BMessage(MSG_SHOW_DISPLAY_SETTINGS));
	fWindowMenu->AddItem(item);
	item->SetTarget(be_app);

	item = new BMenuItem("Users"B_UTF8_ELLIPSIS,
		new BMessage(MSG_SHOW_USERS));
	fWindowMenu->AddItem(item);
	item->SetTarget(be_app);

	menuBar->AddItem(fWindowMenu);
#endif // CLOCKWERK_STAND_ALONE

	return menuBar;
}

// #pragma mark -

// _OpenPlaylist
void
MainWindow::_OpenPlaylist(const BString& playlistID)
{
	// NOTE: document and object library share the same lock now
	AutoWriteLocker locker(fDocument);
	if (!locker.IsLocked()) {
		print_error("MainWindow::_OpenPlaylist() - "
					"unable to lock to switch to playlist %s\n",
					playlistID.String());
		return;
	}

	Playlist* playlist = dynamic_cast<Playlist*>(
		fObjectManager->FindObject(playlistID));

	if (!playlist) {
		print_error("MainWindow::_OpenPlaylist() - "
					"playlist %s not found in object lib\n",
					playlistID.String());
		return;
	}

	fDocument->MakeEmpty();

	fDocument->SetPlaylist(playlist);
	SetPlaylist(playlist);
}

// _UpdateClipboardItems
void
MainWindow::_UpdateClipboardItems()
{
	BClipboard* clipboard = fTimelineView->Clipboard();
	if (!clipboard->Lock())
		return;

	const BMessage* data = clipboard->Data();

	if (fTextViewFocused) {
		// we don't track if the text view has a selection
		fCutMI->SetEnabled(true);
		fCopyMI->SetEnabled(true);
		const void* mimeData;
		ssize_t numBytes;
		bool hasTextClip = data && data->FindData("text/plain", B_MIME_TYPE, 0,
			&mimeData, &numBytes) == B_OK;
		fPasteMI->SetEnabled(hasTextClip);
	} else {
		fCutMI->SetEnabled(fPlaylistItemsSelected);
		fCopyMI->SetEnabled(fPlaylistItemsSelected);
		fPasteMI->SetEnabled(data
			&& data->HasMessage("clockwerk:playlist_items"));
	}

	clipboard->Unlock();
}

// _RemoveSelectedClips
void
MainWindow::_RemoveSelectedClips()
{
	// * iterate over selected Clips of ClipListView
	// * for each Playlist in the object library, build
	//   a list of PlaylistItems that reference the Clip
	// * (optionally) warn the user if at least one of
	//   the PlaylistItem lists is not empty
	// * perform a CompoundCommand removing the Clips
	//   and the PlaylistItems from each Playlist

	AutoReadLocker locker(fObjectManager->Locker());
	if (!fObjectManager || !locker.IsLocked())
		return;

	// put all selected clips into a list
	BList clips;
	for (int32 i = 0;
		 ClipListItem* item
		 	= dynamic_cast<ClipListItem*>(fClipListView->ItemAt(
		 	fClipListView->CurrentSelection(i)));
		 i++) {
		clips.AddItem((void*)item->clip);
	}

	if (clips.CountItems() <= 0)
		return;

	// put all existing Playlists into a list
	BList playlists;
	int32 count = fObjectManager->CountObjects();
	for (int32 i = 0; i < count; i++) {
		Playlist* l = dynamic_cast<Playlist*>(fObjectManager->ObjectAtFast(i));
		if (l && !playlists.AddItem((void*)l))
			return;
	}

	// generate DeleteCommands for each playlist
	// with the items that reference one of the removed clips
	BList deleteCommands;
	count = playlists.CountItems();
	for (int32 i = 0; i < count; i++) {
		BList items;
		Playlist* playlist = (Playlist*)playlists.ItemAtFast(i);
		int32 itemCount = playlist->CountItems();
		for (int32 j = 0; j < itemCount; j++) {
			ClipPlaylistItem* item = dynamic_cast<ClipPlaylistItem*>(
				playlist->ItemAtFast(j));
			if (item && clips.HasItem((void*)item->Clip())) {
				if (!items.AddItem((void*)item))
					return;
			}
		}
		if (items.CountItems() > 0) {
			DeleteCommand* command = new DeleteCommand(
				playlist, (const PlaylistItem**)items.Items(),
				items.CountItems());
			if (!deleteCommands.AddItem((void*)command)) {
				delete command;
				return;
			}
		}
	}
	count = deleteCommands.CountItems() + 1;
		// the DeleteCommands plus the RemoveObjectsCommand

	if (count > 1) {
		const char* message;
		if (clips.CountItems() > 1) {
			message = "The Clips are contained in at least one Playlist. "
					  "Are you sure you want to remove them?";
		} else {
			message = "The Clip is contained in at least one Playlist. "
					  "Are you sure you want to remove it?";
		}
		BAlert* alert = new BAlert("warning", message,
								   "Remove anyways", "Cancel", NULL,
								   B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		int32 button = alert->Go();
		if (button == 1) {
			// operation canceled
			// don't leak the DeleteCommands
			for (int32 i = 0; i < count - 1; i++)
				delete (DeleteCommand*)deleteCommands.ItemAtFast(i);
			return;
		}
	}

	Command** commands = new (std::nothrow) Command*[count];
	if (commands) {
		for (int32 i = 0; i < count - 1; i++)
			commands[i] = (DeleteCommand*)deleteCommands.ItemAtFast(i);

		commands[count - 1] = new (std::nothrow) RemoveObjectsCommand(
			fObjectManager, (ServerObject**)clips.Items(),
			clips.CountItems(), fDocument->ClipSelection());
	} else {
		// don't leak the DeleteCommands
		for (int32 i = 0; i < count - 1; i++)
			delete (DeleteCommand*)deleteCommands.ItemAtFast(i);
	}

	const char* commandName = clips.CountItems() > 1 ?
		"Remove Clips" : "Remove Clip";

	CompoundCommand* command = new (std::nothrow) CompoundCommand(
		commands, count, commandName, -1);

	locker.Unlock();

	AutoWriteLocker _(fDocument);
	fDocument->CommandStack()->Perform(command);
}

// _DuplicateSelectedClips
void
MainWindow::_DuplicateSelectedClips()
{
	// * iterate over selected Clips of ClipListView
	// * create a cloned clip and put it into a list
	// * create an AddObjectsCommand for the list of clones

	ServerObjectFactory* factory = fApp->ObjectFactory();

	AutoReadLocker locker(fObjectManager->Locker());
	if (!factory || !fObjectManager || !locker.IsLocked())
		return;

	// put all selected clips into a list
	BList clips;
	for (int32 i = 0;
		 ClipListItem* item
		 	= dynamic_cast<ClipListItem*>(fClipListView->ItemAt(
		 	fClipListView->CurrentSelection(i)));
		 i++) {
		ServerObject* clip = factory->InstantiateClone(item->clip,
													   fObjectManager);
		if (clip && !clips.AddItem((void*)clip))
			delete clip;
	}

	if (clips.CountItems() <= 0)
		return;

	AddObjectsCommand* command = new (std::nothrow) AddObjectsCommand(
		fObjectManager, (ServerObject**)clips.Items(),
		clips.CountItems(), fDocument->ClipSelection());

	locker.Unlock();

	AutoWriteLocker _(fDocument);
	fDocument->CommandStack()->Perform(command);
}

// _EditSelectedClips
void
MainWindow::_EditSelectedClips()
{
	// invoke proper editor for first selected clip

	AutoReadLocker locker(fObjectManager->Locker());
	if (!fObjectManager || !locker.IsLocked())
		return;

	ClipListItem* item
		= dynamic_cast<ClipListItem*>(fClipListView->ItemAt(
		 	fClipListView->CurrentSelection(0)));

	if (!item || !item->clip)
		return;

	// reference clip before unlocking
	Reference reference(item->clip);
	locker.Unlock();

	_EditClip(item->clip);
}

// _EditClip
void
MainWindow::_EditClip(Clip* clip)
{
	// these are the types of clips that we know how to edit

	// try BitmapClip
	BitmapClip* bitmap = dynamic_cast<BitmapClip*>(clip);
	if (bitmap) {
		_EditBitmapClip(bitmap);
		return;
	}

	// try MediaClip
	MediaClip* media = dynamic_cast<MediaClip*>(clip);
	if (media) {
		_EditMediaClip(media);
		return;
	}

	// try SlideShowPlaylist
	SlideShowPlaylist* slideShow = dynamic_cast<SlideShowPlaylist*>(clip);
	if (slideShow) {
		_EditSlideShowPlaylist(slideShow);
		return;
	}

	// try Playlist
	Playlist* playlist = dynamic_cast<Playlist*>(clip);
	if (playlist) {
		_EditPlaylist(playlist);
		return;
	}

	// TODO: support more editable clip types...

	// display alert telling the user that editing this
	// clip type is not supported
	BAlert* alert = new BAlert("info",
		"Editing this type of clip is currently not supported. Sorry.",
		"Bummer", NULL, NULL, B_WIDTH_AS_USUAL, B_INFO_ALERT);
	alert->Go(NULL);
}

// _EditBitmapClip
void
MainWindow::_EditBitmapClip(BitmapClip* bitmap)
{
	if (!bitmap->Ref())
		return;

//	const char* externalEditorSig
//		= GlobalSettings::Default().ExternalBitmapEditor();
const char* externalEditorSig = "application/x.vnd-YellowBites.WonderBrush";
	BMessenger replyTarget(be_app);
	// build message for external bitmap editor
	BMessage message(BBP_OPEN_BBITMAP);
	message.AddMessenger("target", replyTarget);
	message.AddRef("ref", bitmap->Ref());
	// get the external bitmap editor messenger
	bool needsLaunch = true;
	{
		BMessenger editor(externalEditorSig);
		if (editor.IsValid()) {
			editor.SendMessage(&message);
			needsLaunch = false;
		}
	}
	if (needsLaunch) {
		entry_ref ref;
		if (be_roster->FindApp(externalEditorSig, &ref) == B_OK)
			be_roster->Launch(&ref, &message);
	}
}

// _EditMediaClip
void
MainWindow::_EditMediaClip(MediaClip* clip)
{
	if (!clip->Ref())
		return;

	BEntry entry(clip->Ref(), true);
	if (entry.InitCheck() < B_OK) {
		print_error("unable to obtain file entry for clip '%s': %s\n",
			clip->Name().String(), strerror(entry.InitCheck()));
		return;
	}

	BPath path(&entry);

	int argc = 1;
	char* argv[argc];
	argv[0] = strdup(path.Path());
		// are we leaking here?!?

	status_t ret = be_roster->Launch("video", argc, argv);
	if (ret < B_OK) {
		print_error("unable to launch player for clip '%s': %s\n",
			clip->Name().String(), strerror(ret));
	}
}

// _EditSlideShowPlaylist
void
MainWindow::_EditSlideShowPlaylist(SlideShowPlaylist* playlist)
{
	SlideShowWindow* window = new SlideShowWindow(
		BRect(50, 50, 450, 250), fDocument);
	window->SetPlaylist(playlist);
	window->Show();
}

// _EditPlaylist
void
MainWindow::_EditPlaylist(Playlist* playlist)
{
	BMessage message(MSG_OPEN);
	message.AddString("soid", playlist->ID());
	be_app->PostMessage(&message);
}

// _UpdateSelectedClips
void
MainWindow::_UpdateSelectedClips()
{
	// invoke proper editor for first selected clip

	AutoReadLocker locker(fObjectManager->Locker());
	if (!fObjectManager || !locker.IsLocked())
		return;

	ClipListItem* item
		= dynamic_cast<ClipListItem*>(fClipListView->ItemAt(
		 	fClipListView->CurrentSelection(0)));

	if (!item || !item->clip)
		return;

	BMessage* message = new BMessage(MSG_FILE_FOR_CLIP_UPDATE);
	message->AddPointer("clip", item->clip);

	if (!fUpdateDataPanel) {
		BMessenger messenger(this, this);
		fUpdateDataPanel = new BFilePanel(B_OPEN_PANEL, &messenger,
			NULL, 0, false, message);
	} else {
		fUpdateDataPanel->SetMessage(message);
	}
	fUpdateDataPanel->Show();
}

// _UpdateSelectedClipsFromFile
void
MainWindow::_UpdateSelectedClipsFromFile(BMessage* message)
{
	entry_ref ref;
	Clip* originalClip;
	if (message->FindRef("refs", &ref) < B_OK
		|| message->FindPointer("clip", (void**)&originalClip) < B_OK) {
		print_warning("malformed file panel message (replace clip data)\n");
		return;
	}

	BEntry source(&ref, true);
	if (!source.IsFile()) {
		print_warning("supposed to replace clip data, but the source "
			"is not a file\n");
		return;
	}

	AutoWriteLocker locker(fObjectManager->Locker());
	if (!locker.IsLocked()) {
		print_error("failed to lock object manager (replace clip data)\n");
		return;
	}

	if (!fObjectManager->HasObject(originalClip)) {
		print_warning("supposed to replace clip data, but the clip "
			"is no longer there\n");
		return;
	}

	FileBasedClip* fileClip = dynamic_cast<FileBasedClip*>(originalClip);
	if (!fileClip) {
		print_error("supposed to replace clip data, but clip is not "
			"'external file' based\n");
		return;
	}

	entry_ref originalRef;
	if (fObjectManager->GetRef(originalClip, originalRef) < B_OK) {
		print_error("failed to find original file for clip '%s'\n",
			originalClip->Name().String());
		return;
	}

	BFile newFile(&source, B_READ_ONLY);
	if (newFile.InitCheck() < B_OK) {
		print_error("failed to open new file '%s' to copy new data "
			"from: %s\n", ref.name, strerror(newFile.InitCheck()));
		return;
	}

	BFile oldFile(&originalRef, B_WRITE_ONLY);
	if (oldFile.InitCheck() < B_OK) {
		print_error("failed to open original file '%s' to copy new data "
			"to: %s\n", originalRef.name, strerror(oldFile.InitCheck()));
		return;
	}

	off_t newSize;
	if (newFile.GetSize(&newSize) < B_OK) {
		print_error("failed to extract new file's size (replace clip data)\n");
		return;
	}

	off_t copied = copy_data(newFile, oldFile, newSize);
	if (newSize != copied) {
		print_error("failed to copy %lld bytes from %s to %s\n",
			newSize, ref.name, originalRef.name);
		return;
	}

	fileClip->Reload();
}




