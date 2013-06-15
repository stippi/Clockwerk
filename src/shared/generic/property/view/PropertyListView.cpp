/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PropertyListView.h"

#include <stdio.h>
#include <string.h>

#include <ClassInfo.h>
#include <Clipboard.h>
#include <Font.h>
#ifdef __HAIKU__
#	include <LayoutUtils.h>
#endif
#include <Menu.h>
#include <MenuItem.h>
#include <Message.h>
#include <MessageRunner.h>
#include <Messenger.h>
#include <Window.h>

#include "AddKeyFrameCommand.h"
#include "CommandStack.h"
#include "CompoundCommand.h"
#include "CurrentFrame.h"
#include "KeyFrame.h"
#include "ModifyKeyFrameCommand.h"
//#include "PastePropertiesAction.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "PropertyCommand.h"
#include "PropertyItemView.h"
#include "PropertyObject.h"
#include "RWLocker.h"
#include "Scrollable.h"
#include "Scroller.h"
#include "ScrollView.h"

enum {
	MSG_COPY_PROPERTIES			= 'cppr',
	MSG_PASTE_PROPERTIES		= 'pspr',

	MSG_ADD_KEYFRAME			= 'adkf',

	MSG_SELECT_ALL				= B_SELECT_ALL,
	MSG_SELECT_NONE				= 'slnn',
	MSG_INVERT_SELECTION		= 'invs',

	MSG_UPDATE_PULSE			= 'uppu',
};

static const bigtime_t kUpdateDelay = 125000;

// #pragma mark - SelectedPropertyListener

SelectedPropertyListener::SelectedPropertyListener() {}
SelectedPropertyListener::~SelectedPropertyListener() {}

// #pragma mark - TabFilter

class TabFilter : public BMessageFilter {
 public:
	TabFilter(PropertyListView* target)
		: BMessageFilter(B_ANY_DELIVERY, B_ANY_SOURCE),
		  fTarget(target)
		{
		}
	virtual	~TabFilter()
		{
		}
	virtual	filter_result	Filter(BMessage* message, BHandler** target)
		{
			filter_result result = B_DISPATCH_MESSAGE;
			switch (message->what) {
				case B_UNMAPPED_KEY_DOWN:
				case B_KEY_DOWN: {
					uint32 key;
					uint32 modifiers;
					if (message->FindInt32("raw_char", (int32*)&key) >= B_OK
						&& message->FindInt32("modifiers", (int32*)&modifiers) >= B_OK)
						if (key == B_TAB && fTarget->TabFocus(modifiers & B_SHIFT_KEY))
							result = B_SKIP_MESSAGE;
					break;
				}
				default:
					break;
			}
			return result;
		}
 private:
 	PropertyListView*		fTarget;
};


// constructor
PropertyListView::PropertyListView(RWLocker* locker)
	: BView(BRect(0.0, 0.0, 100.0, 100.0), NULL, B_FOLLOW_NONE,
			B_WILL_DRAW | B_FRAME_EVENTS | B_NAVIGABLE),
	  Scrollable(),
	  BList(20),
	  fClipboard(new BClipboard("Clockwerk properties")),
	  fCommandStack(NULL),
	  fLocker(locker),
	  fCurrentFrame(NULL),

	  fMouseWheelFilter(new MouseWheelFilter(this)),
	  fTabFilter(new TabFilter(this)),

	  fPropertyM(NULL),

	  fPropertyObject(NULL),
	  fSavedProperties(new PropertyObject()),
	  fObject(NULL),
	  fObjects(NULL),
	  fObjectCount(0),

	  fLastClickedItem(NULL),
	  fLastSelectedProperty(NULL),

	  fSuspendUpdates(false),

	  fLastObjectUpdate(0),
	  fUpdateSkipped(false),
	  fUpdatePulse(NULL)
{
	SetLowColor(255, 255, 255, 255);
	SetHighColor(0, 0, 0, 255);
	SetViewColor(B_TRANSPARENT_32_BIT);
}

// destructor
PropertyListView::~PropertyListView()
{
	delete fMouseWheelFilter;
	delete fTabFilter;

	if (fObject)
		fObject->RemoveObserver(this);

	delete fClipboard;
	delete[] fObjects;

	delete fSavedProperties;
}

// AttachedToWindow
void
PropertyListView::AttachedToWindow()
{
	Window()->AddCommonFilter(fMouseWheelFilter);
	Window()->AddCommonFilter(fTabFilter);
	delete fUpdatePulse;
	fUpdatePulse = new BMessageRunner(BMessenger(this),
									  new BMessage(MSG_UPDATE_PULSE),
									  kUpdateDelay);
}

// DetachedFromWindow
void
PropertyListView::DetachedFromWindow()
{
	Window()->RemoveCommonFilter(fMouseWheelFilter);
	Window()->RemoveCommonFilter(fTabFilter);
	delete fUpdatePulse;
	fUpdatePulse = NULL;
}

// FrameResized
void
PropertyListView::FrameResized(float width, float height)
{
	SetVisibleSize(width, height);
//	_LayoutItems();
	Invalidate();
}

// Draw
void
PropertyListView::Draw(BRect updateRect)
{
	if (fSuspendUpdates)
		return;

	FillRect(updateRect, B_SOLID_LOW);

	// display helpful messages
	const char* message1 = NULL;
	const char* message2 = NULL;

	if (!fObject && !fObjects) {
		message1 = "Click on an object to";
		message2 = "edit it's properties here.";
	} else if (fObjects) {
		message1 = "Multiple objects selected.";
	}

	if (message1) {
		SetFont(be_bold_font);
		SetHighColor(tint_color(LowColor(), B_DARKEN_2_TINT));
		BRect b(Bounds());
		BPoint middle;
		middle.y = (b.top + b.bottom) / 2.0;
		middle.x = (b.left + b.right - StringWidth(message1)) / 2.0;
		DrawString(message1, middle);
		if (message2) {
			font_height fh;
			be_bold_font->GetHeight(&fh);
			middle.y += (fh.ascent + fh.descent) * 1.5;
			middle.x = (b.left + b.right - StringWidth(message2)) / 2.0;
			DrawString(message2, middle);
		}
	}

}

// MakeFocus
void
PropertyListView::MakeFocus(bool focus)
{
	if (focus != IsFocus()) {
		BView::MakeFocus(focus);
		if (::ScrollView* scrollView = dynamic_cast< ::ScrollView*>(Parent()))
			scrollView->ChildFocusChanged(focus);
	}
}

// MouseDown
void
PropertyListView::MouseDown(BPoint where)
{
	if (!(modifiers() & B_SHIFT_KEY)) {
		DeselectAll();
	}
	MakeFocus(true);
}

// MessageReceived
void
PropertyListView::MessageReceived(BMessage* message)
{
	switch (message->what) {
//		case MSG_PASTE_PROPERTIES:
//			if (fCanvasView && (fObject || fObjectCount > 0) && fClipboard->Lock()) {
//				if (BMessage* data = fClipboard->Data()) {
//					PropertyObject propertyObject;
//					BMessage propertyArchive;
//					for (int32 i = 0; data->FindMessage("property", i, &propertyArchive) >= B_OK; i++) {
//						if (BArchivable* archivable = instantiate_object(&propertyArchive)) {
//							// see if this is actually a stroke
//							Property* property = cast_as(archivable, Property);
//							if (!property || !propertyObject.AddProperty(property))
//								delete archivable;
//						}
//					}
//					if (propertyObject.CountProperties() > 0) {
//						PastePropertiesAction* action = NULL;
//						if (fObject) {
//							action = new PastePropertiesAction(fCanvasView,
//															   fLayer,
//															   &fObject, 1,
//															   &propertyObject);
//						} else {
//							action = new PastePropertiesAction(fCanvasView,
//															   fLayer,
//															   fObjects, fObjectCount,
//															   &propertyObject);
//						}
//						fCanvasView->PerformCommand(action);
//					}
//				}
//				fClipboard->Unlock();
//			}
//			break;
//		case MSG_COPY_PROPERTIES:
//			if (fPropertyObject) {
//				if (fClipboard->Lock()) {
//					if (BMessage* data = fClipboard->Data()) {
//						fClipboard->Clear();
//						for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
//							if (item->IsSelected()) {
//								const Property* property = item->Property();
//								if (property) {
//									BMessage propertyArchive;
//									if (property->Archive(&propertyArchive) >= B_OK) {
//										data->AddMessage("property", &propertyArchive);
//									}
//								}
//							}
//						}
//						fClipboard->Commit();
//					}
//					fClipboard->Unlock();
//				}
//				_CheckMenuStatus();
//			}
//			break;

		case MSG_ADD_KEYFRAME: {
			if (!fCurrentFrame || !fPropertyObject)
				break;
			BList commands(8);
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				if (item->IsSelected()) {
					Property* p = item->Property();
					PropertyAnimator* animator = p->Animator();
					if (!p || !p->MakeAnimatable()) {
						continue;
					}
					if (!animator && p == fLastSelectedProperty) {
						// the item needs a visual update
						item->UpdateLowColor();
						// if we had to create the animator,
						// trigger a notification, it isn't
						// currently supported by any other means
						fLastSelectedProperty = NULL;
						_NotifyPropertySelected(p);
					}
					animator = p->Animator();
					if (!animator) {
						printf("failed to create animator for property %ld!\n", i);
						continue;
					}
					int64 localFrame = fCurrentFrame->Frame();
					fPropertyObject->ConvertFrameToLocal(localFrame);

					KeyFrame* key = animator->InsertKeyFrameAt(localFrame);
					if (key) {
						key->Property()->SetValue(p);
						if (fCommandStack) {
							Command* c = new AddKeyFrameCommand(animator, key);
							if (!commands.AddItem(c))
								delete c;
						}
					}
				}
			}
			// insert command into Undo stack
			// if there was only one keyframe inserted,
			// use the command directly, otherwise form
			// a CompoundCommand
			int32 commandCount = commands.CountItems();
			if (commandCount <= 0)
				break;
			if (commandCount == 1) {
				fCommandStack->Perform((Command*)commands.ItemAt(0));
			} else {
				Command** commandPtrs = new Command*[commandCount];
				memcpy(commandPtrs, commands.Items(),
					   commandCount * sizeof(Command*));
				fCommandStack->Perform(new CompoundCommand(commandPtrs,
														   commandCount,
														   "Insert Key Frames",
														   0));
			}
			break;
		}

		// property selection
		case MSG_SELECT_ALL: {
			Property* last = NULL;
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				item->SetSelected(true);
				last = item->Property();
			}
			_NotifyPropertySelected(last);
			_CheckMenuStatus();
			break;
		}
		case MSG_SELECT_NONE: {
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				item->SetSelected(false);
			}
			_NotifyPropertySelected(NULL);
			_CheckMenuStatus();
			break;
		}
		case MSG_INVERT_SELECTION: {
			Property* last = NULL;
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				if (!item->IsSelected()) {
					item->SetSelected(true);
					last = item->Property();
				} else
					item->SetSelected(false);
			}
			_NotifyPropertySelected(last);
			_CheckMenuStatus();
			break;
		}

		case MSG_UPDATE_PULSE: {
			// if object changes were ignored because of this,
			// carry them out now
			if (fUpdateSkipped)
				ObjectChanged(fObject);
			break;
		}

		default:
			BView::MessageReceived(message);
	}
}

#ifdef __HAIKU__

BSize
PropertyListView::MinSize()
{
	// We need a stable min size: the BView implementation uses
	// GetPreferredSize(), which by default just returns the current size.
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(100, 50));
}


BSize
PropertyListView::MaxSize()
{
	return BView::MaxSize();
}


BSize
PropertyListView::PreferredSize()
{
	// We need a stable preferred size: the BView implementation uses
	// GetPreferredSize(), which by default just returns the current size.
	return BLayoutUtils::ComposeSize(ExplicitPreferredSize(), BSize(100, 120));
}

#endif // __HAIKU__

// #pragma mark -

// ObjectChanged
void
PropertyListView::ObjectChanged(const Observable* observable)
{
	if (Window() && system_time() - fLastObjectUpdate < kUpdateDelay) {
		fUpdateSkipped = true;
		return;
	}

	fLastObjectUpdate = system_time();
	fUpdateSkipped = false;

	if (!LockLooper())
		return;

	const PropertyObject* object = dynamic_cast<const PropertyObject*>(observable);
	if (object) {
		if (object == fObject) {
			// update properties
// TODO: support more powerful listener interface in PropertyObject!
// (PropertyObjectListner::PropertyChanged(const Property* p))
			_SetTo(fObject);

		}
	}

	UnlockLooper();
}

// #pragma mark -

// TabFocus
bool
PropertyListView::TabFocus(bool shift)
{
	bool result = false;
	PropertyItemView* item = NULL;
	if (IsFocus() && !shift) {
		item = _ItemAt(0);
	} else {
		int32 focussedIndex = -1;
		for (int32 i = 0; PropertyItemView* oldItem = _ItemAt(i); i++) {
			if (oldItem->IsFocused()) {
				focussedIndex = shift ? i - 1 : i + 1;
				break;
			}
		}
		item = _ItemAt(focussedIndex);
	}
	if (item) {
		item->MakeFocus(true);
		result = true;
	}
	return result;
}

// SetMenu
void
PropertyListView::SetMenu(BMenu* menu)
{
	fPropertyM = menu;
	if (fPropertyM == NULL)
		return;

	fSelectM = new BMenu("Select");
	fSelectAllMI = new BMenuItem("All", new BMessage(MSG_SELECT_ALL));
	fSelectM->AddItem(fSelectAllMI);
	fSelectNoneMI = new BMenuItem("None", new BMessage(MSG_SELECT_NONE));
	fSelectM->AddItem(fSelectNoneMI);
	fInvertSelectionMI = new BMenuItem("Invert Selection",
		new BMessage(MSG_INVERT_SELECTION));
	fSelectM->AddItem(fInvertSelectionMI);
	fSelectM->SetTargetForItems(this);

	fPropertyM->AddItem(fSelectM);

	fPropertyM->AddSeparatorItem();

	fCopyMI = new BMenuItem("Copy", new BMessage(MSG_COPY_PROPERTIES));
	fPropertyM->AddItem(fCopyMI);
	fPasteMI = new BMenuItem("Paste", new BMessage(MSG_PASTE_PROPERTIES));
	fPropertyM->AddItem(fPasteMI);

	fPropertyM->AddSeparatorItem();

	fAddKeyMI = new BMenuItem("Add Key", new BMessage(MSG_ADD_KEYFRAME), 'K');
	fPropertyM->AddItem(fAddKeyMI);

	fPropertyM->SetTargetForItems(this);
	// disable menus
	_CheckMenuStatus();
}

// SetCommandStack
void
PropertyListView::SetCommandStack(CommandStack* commandStack)
{
	fCommandStack = commandStack;
}

// SetCurrentFrame
void
PropertyListView::SetCurrentFrame(CurrentFrame* currentFrame)
{
	fCurrentFrame = currentFrame;
}

// #pragma mark -

// SetTo
void
PropertyListView::SetTo(PropertyObject* object)
{
	// set to one property object
	if (fObject != object) {
		if (fObject)
			fObject->RemoveObserver(this);

		fObject = object;

		if (fObject)
			fObject->AddObserver(this);

		_SetTo(object);
	}

	delete[] fObjects;
	fObjects = NULL;
	fObjectCount = 0;

	_CheckMenuStatus();
}

// SetTo
void
PropertyListView::SetTo(PropertyObject** objects, int32 count)
{
	// set to multiple property objects
	if (count == 0) {
		SetTo(NULL);
	} else if (count == 1) {
		SetTo(objects[0]);
	} else if (objects && count > 1) {
		SetTo(NULL);
			// disable property display (for now)
			// will also delete[] previous fObjects
		fObjects = new PropertyObject*[count];
		fObjectCount = count;
		memcpy(fObjects, objects, sizeof(PropertyObject*) * count);
		_CheckMenuStatus();
	}
}

// ScrollOffsetChanged
void
PropertyListView::ScrollOffsetChanged(BPoint oldOffset, BPoint newOffset)
{
	ScrollBy(newOffset.x - oldOffset.x,
			 newOffset.y - oldOffset.y);
}

// Select
void
PropertyListView::Select(PropertyItemView* item)
{
	Property* selectedProperty = NULL;
	if (item) {
		// in the usual case, the selected property is the
		// one of the item being clicked on
		selectedProperty = item->Property();

		if (modifiers() & B_SHIFT_KEY) {
			if (item->IsSelected()) {
				item->SetSelected(false);
				// the clicked item is deselected
				selectedProperty = NULL;
			} else {
				item->SetSelected(true);
			}
		} else if (modifiers() & B_OPTION_KEY) {
			// extending selection as block
			item->SetSelected(true);
			int32 firstSelected = _CountItems();
			int32 lastSelected = -1;
			for (int32 i = 0; PropertyItemView* otherItem = _ItemAt(i); i++) {
				if (otherItem->IsSelected()) {
					 if (i < firstSelected)
					 	firstSelected = i;
					 if (i > lastSelected)
					 	lastSelected = i;
				}
			}
			if (lastSelected > firstSelected) {
				for (int32 i = firstSelected; PropertyItemView* otherItem = _ItemAt(i); i++) {
					if (i > lastSelected)
						break;
					otherItem->SetSelected(true);
				}
			}
		} else {
			// select only the clicked item
			for (int32 i = 0; PropertyItemView* otherItem = _ItemAt(i); i++) {
				otherItem->SetSelected(otherItem == item);
			}
		}
	}
	_CheckMenuStatus();
	_NotifyPropertySelected(selectedProperty);
}

// DeselectAll
void
PropertyListView::DeselectAll()
{
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		item->SetSelected(false);
	}
	_NotifyPropertySelected(NULL);
	_CheckMenuStatus();
}

// Clicked
void
PropertyListView::Clicked(PropertyItemView* item)
{
	fLastClickedItem = item;
}

// DoubleClicked
void
PropertyListView::DoubleClicked(PropertyItemView* item)
{
	if (fLastClickedItem == item) {
		printf("implement PropertyListView::DoubleClicked()\n");
//		...->EditObject(fObject);
	}
	fLastClickedItem = NULL;
}

// #pragma mark -

// UpdateObject
void
PropertyListView::UpdateObject(Property* property)
{
	if (!fPropertyObject)
		return;

	// TODO: actually, the property has already been
	// changed at this point in time, so locking here
	// is actually already too late, but fortunately,
	// it is important that the lock is held when
	// notifications are triggered, and this is done
	// here.... so we are ok for the moment
	AutoWriteLocker locker(fLocker);
	if (fLocker && !locker.IsLocked())
		return;

	if (fCommandStack) {
		PropertyAnimator* animator = property->Animator();
		if (animator && fCurrentFrame) {
			// find local frame
			int64 localFrame = fCurrentFrame->Frame();
			fPropertyObject->ConvertFrameToLocal(localFrame);
			// find the keyframe at the frame
			// (only modify it if it is exactly at the frame)
			KeyFrame* key = animator->KeyFrameBeforeOrAt(localFrame);
			if (key && key->Frame() == localFrame) {
				// insert a ModifyKeyFrameCommand that remembers
				// the current property of the keyframe
				Command* command = new ModifyKeyFrameCommand(
											animator, key);
				// modify the property of the keyframe
				key->Property()->SetValue(property);
				// perform /after/ having changed the property
				fCommandStack->Perform(command);
				// trigger a notification, no one else will do
				animator->Notify();
			} else {
printf("PropertyListView::UpdateObject() - "
	   "animated property but no keyframe at frame %lld\n", localFrame);
	   			// TODO: insert keyframe if in "auto key" mode
	   			// ("auto key" mode doesn't exist yet)
			}
		} else {
			const Property* saved = fSavedProperties->FindProperty(
				property->Identifier());
			Command* command = new PropertyCommand(saved->Clone(false),
												   fPropertyObject);
			fCommandStack->Perform(command);
		}
	}
// TODO: (Big TODO) Propertys should know a "PropertyOwner"
// and make sure that PropertyOwner::ValueChanged() is called
	fPropertyObject->ValueChanged(property);

	_UpdateSavedProperties();
}

// #pragma mark -

// UpdateStrings
void
PropertyListView::UpdateStrings()
{
//	ObjectChanged(fObject);
//
//	if (fSelectM) {
//		LanguageManager* m = LanguageManager::Default();
//
//		fSelectM->Superitem()->SetLabel(m->GetString(PROPERTY_SELECTION, "Select"));
//		fSelectAllMI->SetLabel(m->GetString(SELECT_ALL_PROPERTIES, "All"));
//		fSelectNoneMI->SetLabel(m->GetString(SELECT_NO_PROPERTIES, "None"));
//		fInvertSelectionMI->SetLabel(m->GetString(INVERT_SELECTION, "Invert Selection"));
//
//		fPropertyM->Superitem()->SetLabel(m->GetString(PROPERTY, "Property"));
//		fCopyMI->SetLabel(m->GetString(COPY, "Copy"));
//		if (fObjectCount > 0)
//			fPasteMI->SetLabel(m->GetString(MULTI_PASTE, "Multi Paste"));
//		else
//			fPasteMI->SetLabel(m->GetString(PASTE, "Paste"));
//	}
}

// ScrollView
::ScrollView*
PropertyListView::ScrollView() const
{
	return dynamic_cast< ::ScrollView*>(ScrollSource());
}

// #pragma mark -

// AddListener
bool
PropertyListView::AddListener(SelectedPropertyListener* listener)
{
	if (listener && !fListeners.HasItem((void*)listener))
		return fListeners.AddItem((void*)listener);
	return false;
}

// RemoveListener
bool
PropertyListView::RemoveListener(SelectedPropertyListener* listener)
{
	return fListeners.RemoveItem((void*)listener);
}

// #pragma mark -

// _SetTo
void
PropertyListView::_SetTo(PropertyObject* object)
{
	Property* selectedProperty = NULL;
	// try to do without rebuilding the list
	// it should in fact be pretty unlikely that this does not
	// work, but we keep being defensive
	if (fPropertyObject && object &&
		fPropertyObject->ContainsSameProperties(*object)) {
		// iterate over view items and update their value views
		for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
			Property* property = object->PropertyAt(i);
			if (!item->AdoptProperty(property)) {
				// the reason for this can be that the property is
				// unkown to the PropertyEditorFactory and therefor
				// there is no editor view at this item
				fprintf(stderr, "PropertyListView::_SetTo() - "
								"property mismatch at %ld\n", i);
				break;
			}
			if (property)
				item->SetEnabled(property->IsEditable());
			if (item->IsSelected())
				selectedProperty = property;
		}
		fPropertyObject = object;
	} else {
		// remember scroll pos, selection and focused item
		BPoint scrollOffset = ScrollOffset();
		BList selection(20);
		int32 focused = -1;
		for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
			if (item->IsSelected())
				selection.AddItem((void*)i);
			if (item->IsFocused())
				focused = i;
		}
		if (Window())
			Window()->BeginViewTransaction();
		fSuspendUpdates = true;

		// rebuild list
		_MakeEmpty();

		fPropertyObject = object;
		if (fPropertyObject) {
			// fill with content
			for (int32 i = 0; Property* property = fPropertyObject->PropertyAt(i); i++) {
				PropertyItemView* item = new PropertyItemView(property);
				item->SetEnabled(property->IsEditable());
				_AddItem(item);
			}
			_LayoutItems();

			// restore scroll pos, selection and focus
			SetScrollOffset(scrollOffset);
			for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
				if (selection.HasItem((void*)i)) {
					item->SetSelected(true);
					selectedProperty = item->Property();
				}
				if (i == focused)
					item->MakeFocus(true);
			}
			// enable menu items etc.
		} else {
			// disable menu items etc.
		}

		if (Window())
			Window()->EndViewTransaction();
		fSuspendUpdates = false;

		SetDataRect(_ItemsRect());
	}

	_NotifyPropertyObjectSet(fPropertyObject);
	_NotifyPropertySelected(selectedProperty);

	_UpdateSavedProperties();
	_CheckMenuStatus();
	Invalidate();
}

// _UpdateSavedProperties
void
PropertyListView::_UpdateSavedProperties()
{
	fSavedProperties->DeleteProperties();

	if (!fPropertyObject)
		return;

	int32 count = fPropertyObject->CountProperties();
	for (int32 i = 0; i < count; i++) {
		const Property* p = fPropertyObject->PropertyAtFast(i);
		fSavedProperties->AddProperty(p->Clone(true));
	}
}

// _AddItem
bool
PropertyListView::_AddItem(PropertyItemView* item)
{
	if (item && BList::AddItem((void*)item)) {
//		AddChild(item);
		item->SetListView(this);
		return true;
	}
	return false;
}

// _RemoveItem
PropertyItemView*
PropertyListView::_RemoveItem(int32 index)
{
	PropertyItemView* item = (PropertyItemView*)BList::RemoveItem(index);
	if (item) {
		item->SetListView(NULL);
		if (!RemoveChild(item))
			fprintf(stderr, "failed to remove view in PropertyListView::_RemoveItem()\n");
	}
	return item;
}

// _ItemAt
PropertyItemView*
PropertyListView::_ItemAt(int32 index) const
{
	return (PropertyItemView*)BList::ItemAt(index);
}

// _CountItems
int32
PropertyListView::_CountItems() const
{
	return BList::CountItems();
}

// _MakeEmpty
void
PropertyListView::_MakeEmpty()
{
	int32 count = _CountItems();
	while (PropertyItemView* item = _RemoveItem(count - 1)) {
		delete item;
		count--;
	}
	fPropertyObject = NULL;

	SetScrollOffset(BPoint(0.0, 0.0));
}

// #pragma mark -

// _NotifyPropertyObjectSet
void
PropertyListView::_NotifyPropertyObjectSet(PropertyObject* object) const
{
	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		SelectedPropertyListener* listener
			= (SelectedPropertyListener*)listeners.ItemAtFast(i);
		listener->PropertyObjectSet(object);
	}
}

// _NotifyPropertySelected
void
PropertyListView::_NotifyPropertySelected(Property* property) const
{
	if (fLastSelectedProperty == property) {
		// avoid false notifications
		return;
	}

	BList listeners(fListeners);
	int32 count = listeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		SelectedPropertyListener* listener
			= (SelectedPropertyListener*)listeners.ItemAtFast(i);
		listener->PropertySelected(property);
	}

	fLastSelectedProperty = property;
}

// #pragma mark -

// _ItemsRect
BRect
PropertyListView::_ItemsRect() const
{
	float width = Bounds().Width();
	float height = -1.0;
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		height += item->PreferredHeight() + 1.0;
	}
	if (height < 0.0)
		height = 0.0;
	return BRect(0.0, 0.0, width, height);
}

// _LayoutItems
void
PropertyListView::_LayoutItems()
{
	// figure out maximum label width
	float labelWidth = 0.0;
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		if (item->PreferredLabelWidth() > labelWidth)
			labelWidth = item->PreferredLabelWidth();
	}
	labelWidth = ceilf(labelWidth);
	// layout items
	float top = 0.0;
	float width = Bounds().Width();
	for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
		item->MoveTo(BPoint(0.0, top));
		float height = item->PreferredHeight();
		item->SetLabelWidth(labelWidth);
		item->ResizeTo(width, height);
		item->FrameResized(item->Bounds().Width(),
						   item->Bounds().Height());
		top += height + 1.0;

		if (!item->Parent())
			AddChild(item);
	}
}

// _CheckMenuStatus
void
PropertyListView::_CheckMenuStatus()
{
	if (!fSuspendUpdates) {
		bool gotSelection = false;
		for (int32 i = 0; PropertyItemView* item = _ItemAt(i); i++) {
			if (item->IsSelected()) {
				gotSelection = true;
				break;
			}
		}
		fCopyMI->SetEnabled(gotSelection);
		fAddKeyMI->SetEnabled(gotSelection);

		bool clipboardHasData = false;
		if (fClipboard->Lock()) {
			if (BMessage* data = fClipboard->Data()) {
				clipboardHasData = data->HasMessage("property");
			}
			fClipboard->Unlock();
		}

		fPasteMI->SetEnabled(clipboardHasData);
//		LanguageManager* m = LanguageManager::Default();
		if (fObjectCount > 0)
//			fPasteMI->SetLabel(m->GetString(MULTI_PASTE, "Multi Paste"));
			fPasteMI->SetLabel("Multi Paste");
		else
//			fPasteMI->SetLabel(m->GetString(PASTE, "Paste"));
			fPasteMI->SetLabel("Paste");

		bool enableMenu = fObject || fObjectCount > 0;
		if (fPropertyM->IsEnabled() != enableMenu)
			fPropertyM->SetEnabled(enableMenu);

		bool gotItems = _CountItems() > 0;
		fSelectM->SetEnabled(gotItems);
	}
}


