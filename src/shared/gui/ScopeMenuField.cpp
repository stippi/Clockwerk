/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "ScopeMenuField.h"

#include <MenuItem.h>
#include <Message.h>
#include <String.h>


// constructor
ScopeMenuField::ScopeMenuField(BRect frame, const char* name,
		const char* label, BMenu* menu, bool fixedSize, uint32 resizingMode)
	: BMenuField(frame, name, label, menu, fixedSize, resizingMode)
{
}

// destructor
ScopeMenuField::~ScopeMenuField()
{
}

// SetScopes
void
ScopeMenuField::SetScopes(const BMessage* _scopes)
{
	BMenu* menu = Menu();
	// clean out menu
	while (BMenuItem* item = menu->RemoveItem((int32)0))
		delete item;

	BMessage scopes(*_scopes);
	if (!scopes.HasString("scop"))
		scopes.AddString("scop", "all");

	BString scope;
	for (int32 i = 0; scopes.FindString("scop", i, &scope) == B_OK; i++) {
		BMessage* message = new BMessage(MSG_SET_SCOPE);
		message->AddString("scop", scope.String());

		BMenuItem* item = new BMenuItem(scope.String(), message);
		menu->AddItem(item);
		item->SetTarget(this);
	}
}

// MarkScope
void
ScopeMenuField::MarkScope(const char* _scope)
{
	BString scope(_scope);

	for (int32 i = 0; BMenuItem* item = Menu()->ItemAt(i); i++) {
		BMessage* message = item->Message();
		if (!message)
			continue;

		const char* itemScope;
		if (message->FindString("scop", &itemScope) < B_OK)
			continue;

		if (scope == itemScope) {
			item->SetMarked(true);
			break;
		}
	}
}


