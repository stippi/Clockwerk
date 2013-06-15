/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef REQUEST_XML_CONVERTER_H
#define REQUEST_XML_CONVERTER_H

#include <SupportDefs.h>

class BDataIO;
class BMessage;
class XMLHelper;

class RequestXMLConverter {
public:
								RequestXMLConverter();
								~RequestXMLConverter();

			status_t			ConvertToXML(BMessage* request,
									XMLHelper* xmlHelper);
			status_t			ConvertToXML(BMessage* request,
									BDataIO& stream);

			status_t			ConvertFromXML(XMLHelper* xmlHelper,
									BMessage* request);
			status_t			ConvertFromXML(BDataIO& stream,
									BMessage* request);
private:
			struct RequestPropertyInfo;
			struct RequestInfo;

			status_t			_ConvertFieldValueToString(BMessage* request,
									const char* name, type_code type,
									int32 index, const char** data);
			status_t			_ConvertFieldValueFromString(BMessage* request,
									const char* name, type_code type,
									const char* data);

			const RequestInfo*	_FindRequestInfo(uint32 code);
			const RequestInfo*	_FindRequestInfo(const char* name);
			const RequestPropertyInfo* _FindPropertyInfo(
									const RequestInfo* requestInfo,
									const char* name);
			status_t			_GetTypeForTypeName(const char* name,
									type_code& type);
			const char*			_TypeNameForType(type_code type);

private:
			char*				fBuffer;

private:
	static	const RequestInfo	sRequestInfos[];
};

#endif	// REQUEST_XML_CONVERTER_H
