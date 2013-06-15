/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PROPERTY_EDITOR_FACTORY
#define PROPERTY_EDITOR_FACTORY

#include <SupportDefs.h>

class Property;
class PropertyEditorView;

PropertyEditorView*	EditorFor(Property* property);

#endif // PROPERTY_EDITOR_FACTORY
