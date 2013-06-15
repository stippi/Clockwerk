/*
 * Copyright 2004-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef PROPERTY_OBJECT_H
#define PROPERTY_OBJECT_H

#include "Observable.h"

class BMessage;
class BString;
class FloatProperty;
class Property;

class PropertyObject : public Observable {
 public:
								PropertyObject();
								PropertyObject(const PropertyObject& other,
											   bool deep);
	virtual						~PropertyObject();

			status_t			Archive(BMessage* into) const;
			status_t			Unarchive(const BMessage* archive);

			bool				AddProperty(Property* property);

			Property*			PropertyAt(int32 index) const;
			Property*			PropertyAtFast(int32 index) const;
			int32				CountProperties() const;

			Property*			FindProperty(uint32 propertyID) const;
			FloatProperty*		FindFloatProperty(uint32 propertyID) const;

			bool				ContainsSameProperties(
									const PropertyObject& other) const;
 private:
			status_t			Assign(
									const PropertyObject& other, bool deep);

 public:
			void				DeleteProperties();
			bool				DeleteProperty(uint32 propertyID);

	virtual	void				ValueChanged(Property* property);

	// common interface for any property
			bool				SetValue(uint32 propertyID,
										 const char* value);
			bool				GetValue(uint32 propertyID,
										 BString& value) const;


	// access to more specific property types
			bool				SetValue(uint32 propertyID,
										 int32 value);
			int32				Value(uint32 propertyID,
									  int32 defaultValue) const;

			bool				SetValue(uint32 propertyID,
										 int64 value);
			int64				Value(uint32 propertyID,
									  int64 defaultValue) const;

			bool				SetValue(uint32 propertyID,
										 float value);
			float				Value(uint32 propertyID,
									  float defaultValue) const;

			bool				SetValue(uint32 propertyID,
										 bool value);
			bool				Value(uint32 propertyID,
									  bool defaultValue) const;

			const char*			Value(uint32 propertyID,
									  const char* defaultValue) const;

	// maybe the PropertyObject should know more stuff...
	virtual	void				ConvertFrameToLocal(int64& frame) const;

 private:
			BList				fProperties;
};

#endif // PROPERTY_OBJECT_H
