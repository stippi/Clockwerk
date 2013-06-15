/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_HELPER_H
#define XML_HELPER_H

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <GraphicsDefs.h>
#include <Locker.h>
#include <String.h>

#include "AttributeAccessor.h"
#include "Font.h"
#include "IDManager.h"
#include "ParameterManager.h"


class BDataIO;
class XMLStorable;


class XMLHelper : public IDManager, public ParameterManager,
	public AttributeAccessor {
public:
								XMLHelper();
	virtual						~XMLHelper();

	virtual	void				Init()									= 0;
	virtual	void				Init(const char* rootTagname)			= 0;
	virtual	status_t			Load(const char* filename);
	virtual	status_t			Load(BDataIO& input)					= 0;
	virtual	status_t			Save(const char* filename);
	virtual	status_t			Save(BDataIO& output)					= 0;

								// (global!) locking
								// Since libxerces appears not to be reentrant,
								// anyone using an XMLHelper object needs to lock
								// it once before using it, and unlock it when finished.
			bool				Lock();
			status_t			LockWithTimeout(bigtime_t timeout);
			void				Unlock();
			bool				IsLocked() const;

								// to retrieve the path to the
								// xml file that the helper was
								// loaded from
			const char*			GetCurrentPath() const;

	virtual	status_t			CreateTag(const char* name)				= 0;
	virtual	status_t			OpenTag()								= 0;
	virtual	status_t			OpenTag(const char* name)				= 0;
	virtual	status_t			CloseTag()								= 0;
	virtual	status_t			RewindTag()								= 0;
	virtual	status_t			GetTagName(BString& name) const			= 0;

			status_t			StoreObject(const XMLStorable& object);
			status_t			StoreObject(const XMLStorable* object);
			status_t			StoreObject(const char* tagname,
											const XMLStorable& object);
			status_t			StoreObject(const char* tagname,
											const XMLStorable* object);
			status_t			StoreIDObject(const XMLStorable& object);
			status_t			StoreIDObject(const XMLStorable* object);
			status_t			StoreIDObject(const char* tagname,
											  const XMLStorable& object);
			status_t			StoreIDObject(const char* tagname,
											  const XMLStorable* object);
			status_t			RestoreObject(XMLStorable& object);
			status_t			RestoreObject(XMLStorable* object);
			status_t			RestoreObject(const char* tagname,
											  XMLStorable& object);
			status_t			RestoreObject(const char* tagname,
											  XMLStorable* object);
			status_t			RestoreIDObject(XMLStorable& object);
			status_t			RestoreIDObject(XMLStorable* object);
			status_t			RestoreIDObject(const char* tagname,
												XMLStorable& object);
			status_t			RestoreIDObject(const char* tagname,
												XMLStorable* object);

			void				AssociateStorableWithID(XMLStorable* object,
														const char* id);
			const char*			GetIDForStorable(XMLStorable* object);
			XMLStorable*		GetStorableForID(const char* id);
			bool				RemoveStorable(XMLStorable* object);

 private:
			BString				fCurrentPath;
	static	BLocker				fGlobalLock;
};


#endif	// XML_HELPER_H
