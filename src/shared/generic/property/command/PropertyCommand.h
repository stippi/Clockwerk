/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PROPERTY_COMMAND_H
#define PROPERTY_COMMAND_H

#include "Command.h"

#include "CommandStack.h"
#include "Property.h"
#include "PropertyObject.h"

class PropertyCommand : public Command {
 public:
								PropertyCommand(PropertyObject* savedProperties,
												PropertyObject* object);
								PropertyCommand(Property* savedProperty,
												PropertyObject* object);
	virtual						~PropertyCommand();
	
	virtual	status_t			InitCheck();

	virtual	status_t			Perform();
	virtual status_t			Undo();
	virtual status_t			Redo();

	virtual void				GetName(BString& name);

	virtual	bool				CombineWithNext(const Command* next);

	template<class PropertyType, class ValueType>
	static	status_t			ChangeProperty(CommandStack* commandStack,
									PropertyObject* object,
									PropertyType* property, ValueType value);

 private:
			PropertyObject*		fSavedProperties;
			PropertyObject*		fObject;
};

// ChangeProperty
template<class PropertyType, class ValueType>
/*static*/ status_t
PropertyCommand::ChangeProperty(CommandStack* commandStack,
	PropertyObject* object, PropertyType* property, ValueType value)
{
	if (!commandStack || !object || !property)
		return B_BAD_VALUE;

	Property* clonedProperty = property->Clone(false);
	if (!clonedProperty)
		return B_NO_MEMORY;

	PropertyCommand* command
		= new (std::nothrow) PropertyCommand(clonedProperty, object);

	if (property->SetValue(value))
		object->ValueChanged(property);

	return commandStack->Perform(command);	
}

#endif // PROPERTY_COMMAND_H
