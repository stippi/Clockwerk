/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "xml_import.h"

#include <new>

#include <ByteOrder.h>
#include <DataIO.h>
#include <Path.h>

#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "ServerObject.h"
#include "ServerObjectFactory.h"
#include "ServerObjectManager.h"
#include "XMLHelper.h"
#include "XMLSupport.h"

using std::nothrow;

// import_objects
status_t
import_objects(ServerObjectManager* manager,
			   ServerObjectFactory* factory,
			   BPositionIO* stream)
{
	XMLHelper* xmlHelper = create_xml_helper();

	XMLHelper& xml = *xmlHelper;
	if (!xml.Lock()) {
		delete xmlHelper;
		return B_ERROR;
	}

	status_t ret = xml.Load(*stream);

	// instantiate all objects
	if (ret == B_OK)
		ret = restore_objects(xml, manager, factory);

	// resolve dependencies
	if (ret == B_OK)
		ret = manager->ResolveDependencies();

	xml.Unlock();
	delete xmlHelper;

	return ret;
}

// instanitate_object
static status_t
instanitate_object(XMLHelper& xml, BString& type,
				   ServerObjectManager* manager, ServerObjectFactory* factory)
{
	// retrieve id
	BString id = xml.GetAttribute("soid", "");
	if (id.CountChars() == 0)
		return B_BAD_VALUE;

	status_t ret = B_OK;

	// instantiate object from factory
	ServerObject* object = factory->Instantiate(type, id, manager);
	if (!object)
		ret = B_NO_MEMORY;

	// read object properties
	if (ret == B_OK)
		ret = restore_properties(xml, object);

	// add to object manager
	if (ret == B_OK && !manager->AddObject(object))
		ret = B_NO_MEMORY;

	// clean up in case of error
	if (ret != B_OK)
		delete object;

	return ret;
}

// restore_objects
status_t
restore_objects(XMLHelper& xml, ServerObjectManager* manager,
				ServerObjectFactory* factory)
{
	status_t ret = B_OK;

	while (xml.OpenTag("OBJECT") == B_OK) {
		// read "type" attribute to get a preconfigured PropertyObject
		BString type = xml.GetAttribute("type", "");
		if (type.CountChars() == 0) {
			ret = xml.CloseTag();
			if (ret == B_OK)
				continue;
			else
				break;
		}

		// instantiate and add object
		ret = instanitate_object(xml, type, manager, factory);
		// ignore invalid object data (no object was added to the library)
		if (ret != B_NO_MEMORY)
			ret = B_OK;

		if (ret == B_OK)
			ret = xml.CloseTag();

		if (ret != B_OK)
			break;
	}

	return ret;
}

// restore_properties
status_t
restore_properties(XMLHelper& xml, PropertyObject* object)
{
	status_t ret = B_OK;

	AutoNotificationSuspender _(object);

	// read properties
	while (xml.OpenTag("PROPERTY") == B_OK) {
		// read ID
		int32 id = 0;
		BString idString = xml.GetAttribute("id", "");
		if (idString.CountChars() == 4) {
			id = B_BENDIAN_TO_HOST_INT32(*(uint32*)idString.String());
		} else {
			xml.CloseTag();
			continue;
		}
		// find property that matches the ID
		Property* property = object->FindProperty(id);
		if (!property) {
			xml.CloseTag();
			continue;
		}
		// read value
		BString value = xml.GetAttribute("value", "");
		if (property->SetValue(value.String()))
			object->ValueChanged(property);

		bool firstRun = true;

		// see if this property is supposed to be animated
		while (xml.OpenTag("KEY") == B_OK) {
			// if this item is not animated yet, create animator
			PropertyAnimator* animator = property->Animator();
			if (!animator) {
				property->MakeAnimatable();
				animator = property->Animator();
				firstRun = false;
			} else if (firstRun) {
				// clean out any existing keyframes
				// bad luck if the file is corrupt...
				animator->MakeEmpty();
				firstRun = false;
			}
			if (!animator) {
				// non-animatable property
				ret = xml.CloseTag();
				break;
			}

			// read attributes of keyframe
			int64 frame = xml.GetAttribute("frame", (int64)0);
			bool locked = xml.GetAttribute("locked", false);
			value = xml.GetAttribute("value", "");
	
			// insert new keyframe with these attributes
			Property* keyProperty = property->Clone(false);
			if (!keyProperty) {
				ret = B_NO_MEMORY;
				break;
			}
			keyProperty->SetValue(value.String());
			KeyFrame* key = new (nothrow) KeyFrame(keyProperty, frame, locked);
			if (!key) {
				delete keyProperty;
				ret = B_NO_MEMORY;
				break;
			}
			if (!animator->AddKeyFrame(key)) {
				delete key;
				ret = B_NO_MEMORY;
				break;
			}
	
			ret = xml.CloseTag();
	
			if (ret != B_OK)
				break;
		}


		ret = xml.CloseTag();

		if (ret != B_OK)
			break;
	}

	return ret;
}
