/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "xml_export.h"

#include <ByteOrder.h>
#include <DataIO.h>
#include <Path.h>

#include "CommonPropertyIDs.h"
#include "KeyFrame.h"
#include "Property.h"
#include "PropertyAnimator.h"
#include "ServerObject.h"
#include "ServerObjectManager.h"
#include "XMLHelper.h"
#include "XMLSupport.h"

// export_objects
status_t
export_objects(ServerObjectManager* manager, BPositionIO* stream)
{
	XMLHelper* xmlHelper = create_xml_helper();

	XMLHelper& xml = *xmlHelper;
	if (!xml.Lock()) {
		delete xmlHelper;
		return B_ERROR;
	}

	xml.Init("OBJECT_LIBRARY");

	// write the clip library
	status_t ret = store_objects(xml, manager);

	// save the file
	if (ret == B_OK)
		ret = xml.Save(*stream);

	xml.Unlock();
	delete xmlHelper;

	return ret;
}

// store_objects
status_t
store_objects(XMLHelper& xml, ServerObjectManager* manager)
{
	status_t ret = B_OK;

	int32 count = manager->CountObjects();
	for (int32 i = 0; i < count; i++) {
		ServerObject* object = manager->ObjectAtFast(i);

		ret = xml.CreateTag("OBJECT");

		if (ret == B_OK)
			ret = xml.SetAttribute("type", object->Type());

		if (ret == B_OK)
			ret = xml.SetAttribute("soid", object->ID());

		if (ret == B_OK)
			ret = store_properties(xml, object);
	
		if (ret == B_OK)
			ret = xml.CloseTag();

		if (ret < B_OK)
			break;
	}

	return ret;
}

// store_properties
status_t
store_properties(XMLHelper& xml, PropertyObject* object)
{
	status_t ret = B_OK;

	// store the properties
	int32 count = object->CountProperties();
	for (int32 i = 0; i < count; i++) {
		Property* property = object->PropertyAtFast(i);

		PropertyAnimator* animator = property->Animator();
		if (!animator && is_default_value(property)) {
			// skip properties with default values
			// and no animator
			continue;
		}

		// store property as attribute
		ret = xml.CreateTag("PROPERTY");

		if (ret == B_OK && animator) {
			// store keyframes
			int32 keyCount = animator->CountKeyFrames();
			for (int32 i = 0; i < keyCount; i++) {
				KeyFrame* key = animator->KeyFrameAtFast(i);
		
				ret = xml.CreateTag("KEY");
				if (ret != B_OK)
					break;
		
				if (key->Frame() != 0) {
					ret = xml.SetAttribute("frame", key->Frame());
					if (ret != B_OK)
						break;
				}
				if (key->IsLocked()) {
					ret = xml.SetAttribute("locked", key->IsLocked());
					if (ret != B_OK)
						break;
				}
		
				BString valueAsString;
				key->Property()->GetValue(valueAsString);
				ret = xml.SetAttribute("value", valueAsString.String());
				if (ret != B_OK)
					break;
		
				ret = xml.CloseTag();
				if (ret != B_OK)
					break;
			}
		}

		if (ret == B_OK) {
			// id
			unsigned id = B_HOST_TO_BENDIAN_INT32(property->Identifier());
			char idString[5];
			sprintf(idString, "%.4s", (const char*)&id);
			idString[4] = 0;
			ret = xml.SetAttribute("id", idString);
		}
		if (ret == B_OK) {
			// value
			BString value;
			property->GetValue(value);
			ret = xml.SetAttribute("value", value.String());
		}
//		if (ret == B_OK) {
//			// type
//			// NOTE: type is currently ignored during import,
//			// it could be used for additional robustness
//			unsigned type = B_HOST_TO_BENDIAN_INT32(property->Type());
//			char typeString[5];
//			sprintf(typeString, "%.4s", (const char*)&type);
//			typeString[4] = 0;
//			ret = xml.SetAttribute("type", typeString);
//		}
		if (ret == B_OK)
			ret = xml.CloseTag();
		if (ret < B_OK)
			break;
	}

	return ret;
}
