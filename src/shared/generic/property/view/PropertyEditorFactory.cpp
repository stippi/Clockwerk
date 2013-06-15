/*
 * Copyright 2006, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "PropertyEditorFactory.h"

#include "ColorProperty.h"
#include "DurationProperty.h"
#include "FontProperty.h"
#include "Int64Property.h"
#include "OptionProperty.h"
#include "TimeProperty.h"

#include "BoolValueView.h"
#include "ColorValueView.h"
#include "DurationValueView.h"
#include "FloatValueView.h"
#include "FontValueView.h"
#include "IntValueView.h"
#include "Int64ValueView.h"
#include "OptionValueView.h"
#include "StringValueView.h"
#include "TimeValueView.h"

PropertyEditorView*
EditorFor(Property* p)
{
	if (!p)
		return NULL;

	if (IntProperty* i = dynamic_cast<IntProperty*>(p))
		return new IntValueView(i);

	if (FloatProperty* f = dynamic_cast<FloatProperty*>(p))
		return new FloatValueView(f);

	if (FontProperty* f = dynamic_cast<FontProperty*>(p))
		return new FontValueView(f);

	if (BoolProperty* b = dynamic_cast<BoolProperty*>(p))
		return new BoolValueView(b);

	if (StringProperty* s = dynamic_cast<StringProperty*>(p))
		return new StringValueView(s);

	if (Int64Property* i = dynamic_cast<Int64Property*>(p))
		return new Int64ValueView(i);

	if (OptionProperty* o = dynamic_cast<OptionProperty*>(p))
		return new OptionValueView(o);

	if (ColorProperty* c = dynamic_cast<ColorProperty*>(p))
		return new ColorValueView(c);

	if (TimeProperty* t = dynamic_cast<TimeProperty*>(p))
		return new TimeValueView(t);

	if (DurationProperty* d = dynamic_cast<DurationProperty*>(p))
		return new DurationValueView(d);

	return NULL;
}

