/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PropertyCommand.h"

#include <new>
#include <stdio.h>

#include "CommonPropertyIDs.h"

using std::nothrow;

// constructor
PropertyCommand::PropertyCommand(PropertyObject* savedProperties,
								 PropertyObject* object)
	: Command(),
	  fSavedProperties(savedProperties),
	  fObject(object)
{
}

// constructor
PropertyCommand::PropertyCommand(Property* savedProperty,
								 PropertyObject* object)
	: Command(),
	  fSavedProperties(new (nothrow) PropertyObject()),
	  fObject(object)
{
	if (!fSavedProperties || !fSavedProperties->AddProperty(savedProperty))
		delete savedProperty;
}

// destructor
PropertyCommand::~PropertyCommand()
{
	delete fSavedProperties;
}

// InitCheck
status_t
PropertyCommand::InitCheck()
{
	status_t status = fObject
					  && fSavedProperties
					  && fSavedProperties->CountProperties() > 0 ?
					  B_OK : B_BAD_VALUE;
	return status;
}

// Perform
status_t
PropertyCommand::Perform()
{
	// properties already changed
	return B_OK;
}

// Undo
status_t
PropertyCommand::Undo()
{
	int32 count = fSavedProperties->CountProperties();
	status_t ret = B_OK;
	fObject->SuspendNotifications(true);
	for (int32 i = 0; i < count; i++) {
		Property* saved = fSavedProperties->PropertyAtFast(i);
		// search the same property in our object
		int32 id = saved->Identifier();
		Property* p = fObject->FindProperty(id);
		if (!p) {
			ret = B_MISMATCHED_VALUES;
			break;
		}
		// store the current value of the property
		Property* temp = p->Clone(false);
		if (!temp) {
			ret = B_NO_MEMORY;
			break;
		}
		// toggle the value of the property with our backup
		if (p->SetValue(saved))
			fObject->ValueChanged(p);
		saved->SetValue(temp);
		delete temp;
	}
	fObject->SuspendNotifications(false);
	return ret;
}

// Redo
status_t
PropertyCommand::Redo()
{
	return Undo();
}

// GetName
void
PropertyCommand::GetName(BString& name)
{
	if (fSavedProperties->CountProperties() > 1)
		name << "Change Object Properties";
	else {
		name << "Change ";
		Property* p = fSavedProperties->PropertyAt(0);
		if (p)
			name << name_for_id(p->Identifier());
		else
			name << "Nothing";
	}
}

// CombineWithNext
bool
PropertyCommand::CombineWithNext(const Command* _next)
{
	const PropertyCommand* next = dynamic_cast<const PropertyCommand*>(_next);
	if (!next)
		return false;
	if (next->fTimeStamp - fTimeStamp > 500000)
		return false;
	if (next->fObject != fObject)
		return false;

	int32 count = fSavedProperties->CountProperties();
	if (next->fSavedProperties->CountProperties() != count)
		return false;

	// see if we control the same properties
	for (int32 i = 0; i < count; i++) {
		Property* our = fSavedProperties->PropertyAt(i);
		Property* other = next->fSavedProperties->PropertyAt(i);
		if (our->Identifier() != other->Identifier())
			return false;
	}

	fTimeStamp = next->fTimeStamp;

	return true;
}

