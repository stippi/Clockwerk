/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SCOPE_MENU_FIELD_H
#define SCOPE_MENU_FIELD_H


#include <MenuField.h>


enum {
	MSG_SET_SCOPE		= 'stsp',
};


class ScopeMenuField : public BMenuField {
 public:
								ScopeMenuField(BRect frame, const char* name,
									const char* label, BMenu* menu,
									bool fixedSize, uint32 resizingMode
										= B_FOLLOW_LEFT | B_FOLLOW_TOP);
	virtual						~ScopeMenuField();

			void				SetScopes(const BMessage* scopes);
			void				MarkScope(const char* scope);
};

#endif // SCOPE_MENU_FIELD_H
