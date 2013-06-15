/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef PARAMETER_MANAGER_H
#define PARAMETER_MANAGER_H

#include <SupportDefs.h>

class BMessage;
class BString;

class ParameterManager {
 public:
								ParameterManager();
	virtual						~ParameterManager();

			void				SetParameter(const char* name,
											 const char* value);
			void				SetParameter(const char* name,
											 bool value);
			void				SetParameter(const char* name,
											 uint32 value);
			void				SetParameter(const char* name,
											 int32 value);
			void				SetParameter(const char* name,
											 uint64 value);
			void				SetParameter(const char* name,
											 int64 value);
			void				SetParameter(const char* name,
											 float value);
			void				SetParameter(const char* name,
											 double value);
			void				SetParameter(const char* name,
											 const void* value);

			bool				RemoveParameter(const char* name);

			bool				GetParameter(const char* name,
											 BString& value) const;
			BString				GetParameter(const char* name,
											 const char* defaultValue) const;
			bool				GetParameter(const char* name,
											 bool defaultValue) const;
			uint32				GetParameter(const char* name,
											 uint32 defaultValue) const;
			int32				GetParameter(const char* name,
											 int32 defaultValue) const;
			uint64				GetParameter(const char* name,
											 uint64 defaultValue) const;
			int64				GetParameter(const char* name,
											 int64 defaultValue) const;
			float				GetParameter(const char* name,
											 float defaultValue) const;
			double				GetParameter(const char* name,
											 double defaultValue) const;
			void*				GetParameter(const char* name,
											 const void* value) const;


 private:
			BMessage*			fParameters;
};

#endif	// PARAMETER_MANAGER_H
