/*
 * Copyright 2006-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PROPERTY_LIST_VIEW_H
#define PROPERTY_LIST_VIEW_H

#include <List.h>
#include <View.h>

#include "MouseWheelFilter.h"
#include "Observer.h"
#include "Scrollable.h"

class BClipboard;
class BMenu;
class BMenuItem;
class BMessageRunner;
class CommandStack;
class CurrentFrame;
class MouseWheelFilter;
class Property;
class PropertyItemView;
class PropertyObject;
class RWLocker;
class ScrollView;
class TabFilter;

class SelectedPropertyListener {
 public:
								SelectedPropertyListener();
	virtual						~SelectedPropertyListener();

	virtual	void				PropertyObjectSet(PropertyObject* object) = 0;
	virtual	void				PropertySelected(Property* property) = 0;
};

class PropertyListView : public BView,
						 public Scrollable,
						 private BList,
						 public Observer,
						 public MouseWheelTarget {
 public:
								PropertyListView(RWLocker* locker);
	virtual						~PropertyListView();

								// BView
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				FrameResized(float width, float height);
	virtual	void				Draw(BRect updateRect);
	virtual	void				MakeFocus(bool focus);

	virtual	void				MouseDown(BPoint where);

	virtual	void				MessageReceived(BMessage* message);

#ifdef __HAIKU__
	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();
	virtual	BSize				PreferredSize();
#endif

								// ObjectObserver
	virtual	void				ObjectChanged(const Observable* object);

								// Scrollable
	virtual	void				ScrollOffsetChanged(BPoint oldOffset,
													BPoint newOffset);

								// MouseWheelTarget
	virtual	bool				MouseWheelChanged(float x, float y) { return false; }

								// PropertyListView
			bool				TabFocus(bool shift);

			void				SetMenu(BMenu* menu);
			void				SetCommandStack(CommandStack* commandStack);
			void				SetCurrentFrame(CurrentFrame* currentFrame);

			void				SetTo(PropertyObject* object);
			void				SetTo(PropertyObject** objects, int32 count);

			void				Select(PropertyItemView* item);
			void				DeselectAll();

			void				Clicked(PropertyItemView* item);
			void				DoubleClicked(PropertyItemView* item);

			void				UpdateObject(Property* property);

			void				UpdateStrings();

			::ScrollView*		ScrollView() const;

			bool				AddListener(
									SelectedPropertyListener* listener);
			bool				RemoveListener(
									SelectedPropertyListener* listener);

 private:
			void				_SetTo(PropertyObject* object);
			void				_UpdateSavedProperties();

			bool				_AddItem(PropertyItemView* item);
			PropertyItemView*	_RemoveItem(int32 index);
			PropertyItemView*	_ItemAt(int32 index) const;
			int32				_CountItems() const;

			void				_NotifyPropertyObjectSet(
									PropertyObject* object) const;
			void				_NotifyPropertySelected(Property* p) const;

			void				_MakeEmpty();

			BRect				_ItemsRect() const;
			void				_LayoutItems();

			void				_CheckMenuStatus();

			BClipboard*			fClipboard;
			CommandStack*		fCommandStack;
			RWLocker*			fLocker;
			CurrentFrame*		fCurrentFrame;

			BMenu*				fSelectM;
			BMenuItem*			fSelectAllMI;
			BMenuItem*			fSelectNoneMI;
			BMenuItem*			fInvertSelectionMI;

			MouseWheelFilter*	fMouseWheelFilter;
			TabFilter*			fTabFilter;

			BMenu*				fPropertyM;
			BMenuItem*			fCopyMI;
			BMenuItem*			fPasteMI;

			BMenuItem*			fAddKeyMI;

			PropertyObject*		fPropertyObject;
			PropertyObject*		fSavedProperties;

			PropertyObject*		fObject;
				// editing one object
			PropertyObject**	fObjects;
			int32				fObjectCount;
				// editing multiple objects

			BList				fListeners;

			PropertyItemView*	fLastClickedItem;
	mutable	Property*			fLastSelectedProperty;

			bool				fSuspendUpdates;

			bigtime_t			fLastObjectUpdate;
			bool				fUpdateSkipped;
			BMessageRunner*		fUpdatePulse;
};

#endif // PROPERTY_LIST_VIEW_H
