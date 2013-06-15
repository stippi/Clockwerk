/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef SERVER_OBJECT_H
#define SERVER_OBJECT_H

#include "PropertyObject.h"
#include "Referencable.h"

class OptionProperty;
class ServerObjectManager;
class StringProperty;

class ServerObject : public PropertyObject,
					 public Referencable {
 public:
								ServerObject(const char* type);
								ServerObject(const ServerObject& other,
											 bool deep);
	virtual						~ServerObject();

	virtual	status_t			SetTo(const ServerObject* other);

	// PropertyObject interface
	virtual	void				ValueChanged(Property* property);

	// ServerObject
			status_t			InitCheck() const
									{ return fInitStatus; }
			bool				IsValid() const;

			void				SetDependenciesResolved(bool resolved);

			void				SetName(const BString& name);
			BString				Name() const;

			void				SetID(const BString& id);
			BString				ID() const;

			void				SetVersion(int32 version);
			int32				Version() const;

			BString				Type() const;

	virtual	void				SetPublished(int32 version);

	virtual	bool				IsMetaDataOnly() const;
	virtual	bool				IsExternalData() const;

			void				SetStatus(int32 status);
			int32				Status() const;
			bool				HasRemovedStatus() const;

	virtual	status_t			ResolveDependencies(
									const ServerObjectManager* library);

			void				SetMetaDataSaved(bool saved);
			bool				IsMetaDataSaved() const
									{ return fMetaDataSaved; }

			void				SetDataSaved(bool saved);
			bool				IsDataSaved() const
									{ return fDataSaved; }

	// for ServerObjectManager only
			void				AttachedToManager(
									ServerObjectManager* manager);
			void				DetachedFromManager();

 private:
			status_t			fInitStatus;
			bool				fDependenciesResolved : 1;
			bool				fMetaDataSaved : 1;
			bool				fDataSaved : 1;

	mutable	StringProperty*		fCachedIDProperty;
	mutable	OptionProperty*		fCachedStatusProperty;

			ServerObjectManager* fObjectManager;
};

#endif // SERVER_OBJECT_H
