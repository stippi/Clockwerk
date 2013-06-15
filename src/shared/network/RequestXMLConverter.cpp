/*
 * Copyright 2007-2008, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <Message.h>
#include <String.h>

#include "AutoDeleter.h"
#include "AutoLocker.h"
#include "Debug.h"
#include "Logger.h"
#include "RequestMessageCodes.h"
#include "RequestXMLConverter.h"
#include "XMLHelper.h"
#include "XMLSupport.h"


static Logger sLog("network.RequestXMLConverter");

struct RequestXMLConverter::RequestPropertyInfo {
	const char*	name;
	type_code	type;
};

struct RequestXMLConverter::RequestInfo {
	const char*				name;
	uint32					code;
	RequestPropertyInfo		properties[16];
};

// NOTE: B_STRING_TYPES are actually superfluous, see ConvertFromXML,
// values are added to the message as string, if "type" attribute is
// missing

const RequestXMLConverter::RequestInfo
RequestXMLConverter::sRequestInfos[] = {
	{
		"error reply", REQUEST_ERROR_REPLY,
		{
			{ "error",		B_STRING_TYPE },
			{}
		}
	},
	{
		"upload", REQUEST_UPLOAD,
		{
			{ "name",		B_STRING_TYPE },
			{ "size",		B_INT64_TYPE },
			{ "type",		B_STRING_TYPE },
			{ "vrsn",		B_STRING_TYPE },
			{ "soid",		B_STRING_TYPE },

			{}
		}
	},
	{
		"upload ok", REQUEST_UPLOAD_OK_REPLY,
		{
			{ "soid",		B_STRING_TYPE },
			{ "vrsn",		B_INT32_TYPE },
			{}
		}
	},
	{
		"download", REQUEST_DOWNLOAD,
		{
			{ "soid",		B_STRING_TYPE },
			{ "vrsn",		B_INT32_TYPE },
			{ "send data",	B_BOOL_TYPE },
			{ "skip data",	B_INT64_TYPE },

			{}
		}
	},
	{
		"remove", REQUEST_REMOVE,
		{
			{ "soid",		B_STRING_TYPE },
			{}
		}
	},
	{
		"list files", REQUEST_LIST_FILES,
		{
			{ "scop",					B_STRING_TYPE },
			{ "results start index",	B_INT32_TYPE },
			{ "results max count",		B_INT32_TYPE },
			{ "results cookie",			B_STRING_TYPE },
			{}
		}
	},
	{
		"list files reply", REQUEST_LIST_FILES_REPLY,
		{
			{ "name",			B_STRING_TYPE },
			{ "size",			B_INT64_TYPE },
			{ "type",			B_STRING_TYPE },
			{ "soid",			B_STRING_TYPE },
			{ "vrsn",			B_INT32_TYPE },
			{ "results cookie",	B_STRING_TYPE },
			{}
		}
	},
	{
		"list scopes", REQUEST_LIST_SCOPES,
		{
			{}
		}
	},
	{
		"list scopes reply", REQUEST_LIST_SCOPES_REPLY,
		{
			{ "scop",		B_STRING_TYPE },
			{}
		}
	},
	{
		"get revision", REQUEST_GET_REVISION,
		{
			{ "revision number", B_INT64_TYPE },
			{}
		}
	},
	{
		"get revision reply", REQUEST_GET_REVISION_REPLY,
		{
			{ "latest revision number", B_INT64_TYPE },
			{ "local revision number", B_INT64_TYPE },
			{ "change set id", B_STRING_TYPE },
			{ "revision number", B_INT64_TYPE },
			{ "timestamp", B_INT64_TYPE },
			{}
		}
	},
	{
		"get unrevisioned change set", REQUEST_GET_UNREVISIONED_CHANGE_SETS,
		{
			{}
		}
	},
	{
		"get unrevisioned change set reply", REQUEST_GET_UNREVISIONED_CHANGE_SETS_REPLY,
		{
			{ "local revision number", B_INT64_TYPE },
			{ "change set id", B_STRING_TYPE },
			{}
		}
	},
	{
		"get change set", REQUEST_GET_CHANGE_SET,
		{
			{ "change set id", B_STRING_TYPE },
			{}
		}
	},
	{
		"get change set reply", REQUEST_GET_CHANGE_SET_REPLY,
		{
			{ "local revision number", B_INT64_TYPE },
			{ "client", B_STRING_TYPE },
			{ "user", B_STRING_TYPE },
			{ "soid", B_STRING_TYPE },
			{ "vrsn", B_INT32_TYPE },
			{}
		}
	},
	{
		"start transaction", REQUEST_START_TRANSACTION,
		{
			{ "change set id", B_STRING_TYPE },
			{ "client", B_STRING_TYPE },
			{ "user", B_STRING_TYPE },
			{}
		}
	},
	{
		"finish transaction", REQUEST_FINISH_TRANSACTION,
		{
			{}
		}
	},
	{
		"finish transaction reply", REQUEST_FINISH_TRANSACTION_REPLY,
		{
			{ "revision number", B_INT64_TYPE },
			{ "local revision number", B_INT64_TYPE },
			{ "is master", B_STRING_TYPE },
			{}
		}
	},
	{
		"cancel transaction", REQUEST_CANCEL_TRANSACTION,
		{
			{}
		}
	},
	{
		"send data", REQUEST_SEND_DATA,
		{
			{ "size",		B_INT32_TYPE },
			{}
		}
	},
	{
		"clear stores", REQUEST_CLEAR_STORES,
		{
			{}
		}
	},
	{
		"sync with superior", REQUEST_SYNC_WITH_SUPERIOR,
		{
			{}
		}
	},
	// for software update:
	{
		"clockwerk revision", REQUEST_CLOCKWERK_REVISION,
		{
			{ "soid",		B_STRING_TYPE },
				// id of the server object that represents the individual file
				// multiple fields
			{ "vrsn",		B_INT32_TYPE },
				// version of the respective server object at the same
				// field index
			{}
		}
	},
	{}
};

// constructor
RequestXMLConverter::RequestXMLConverter()
{
}

// destructor
RequestXMLConverter::~RequestXMLConverter()
{
}

// ConvertToXML
status_t
RequestXMLConverter::ConvertToXML(BMessage* request, XMLHelper* xmlHelper)
{
	if (!request || !xmlHelper)
		return B_BAD_VALUE;

	char _buffer[1024];
	fBuffer = _buffer;

	AutoLocker<XMLHelper> _(xmlHelper);

	xmlHelper->Init();

	// find request info
	const RequestInfo* requestInfo = _FindRequestInfo(request->what);
	if (!requestInfo)
		RETURN_ERROR(B_ERROR);

	// set request name
	status_t error = xmlHelper->SetAttribute("request", requestInfo->name);
	if (error != B_OK)
		return error;

#if B_BEOS_VERSION_DANO
	const
#endif
	char *name;
	type_code type;
	int32 count;
	for (int32 i = 0;
		 request->GetInfo(B_ANY_TYPE, i, &name, &type, &count) == B_OK;
		 i++) {
		for (int32 index = 0; index < count; index++) {
			// open Property element
			error = xmlHelper->CreateTag("Property");
			if (error != B_OK)
				return error;

			// write property name
			error = xmlHelper->SetAttribute("name", name);
			if (error != B_OK)
				return error;

			// check if this is a known type on the other side
			if (_FindPropertyInfo(requestInfo, name) == NULL) {
				// it's not, so let's add specific type information
				error = xmlHelper->SetAttribute("type", _TypeNameForType(type));
				if (error != B_OK)
					return error;
			}

			// convert the value to string
			const char* data = fBuffer;
			error = _ConvertFieldValueToString(request, name, type, index,
				&data);
			if (error != B_OK)
				return error;

			// add the value
			error = xmlHelper->SetAttribute("value", data);
			if (error != B_OK)
				return error;

			// close Property element
			error = xmlHelper->CloseTag();
			if (error != B_OK)
				return error;
		}
	}

	return B_OK;
}



// ConvertToXML
status_t
RequestXMLConverter::ConvertToXML(BMessage* request, BDataIO& stream)
{
	if (!request)
		return B_BAD_VALUE;

	XMLHelper* xmlHelper = create_xml_helper();
	if (!xmlHelper)
		return B_NO_MEMORY;

	ObjectDeleter<XMLHelper> deleter(xmlHelper);
	AutoLocker<XMLHelper> lock(xmlHelper);

	if (!lock.IsLocked())
		return B_ERROR;

	status_t ret = ConvertToXML(request, xmlHelper);
	if (ret < B_OK)
		return ret;

	return xmlHelper->Save(stream);
}


// ConvertFromXML
status_t
RequestXMLConverter::ConvertFromXML(XMLHelper* xmlHelper, BMessage* request)
{
	if (!request || !xmlHelper)
		return B_BAD_VALUE;

	AutoLocker<XMLHelper> _(xmlHelper);

	request->MakeEmpty();

	// get request name
	BString requestName;
	status_t error = xmlHelper->GetAttribute("request", requestName);
	if (error != B_OK)
		RETURN_ERROR(error);

	// find request info
	const RequestInfo* requestInfo = _FindRequestInfo(requestName.String());
	if (!requestInfo)
		RETURN_ERROR(B_ERROR);

	request->what = requestInfo->code;

	xmlHelper->RewindTag();

	while (xmlHelper->OpenTag("Property") == B_OK) {
		// get property name
		BString name;
		status_t error = xmlHelper->GetAttribute("name", name);
		if (error != B_OK)
			RETURN_ERROR(error);

		// get property value string
		BString value;
		error = xmlHelper->GetAttribute("value", value);
		if (error != B_OK)
			RETURN_ERROR(error);

		// find property info
		const RequestPropertyInfo* propertyInfo =
			_FindPropertyInfo(requestInfo, name.String());
		if (propertyInfo) {
			// convert the value
			error = _ConvertFieldValueFromString(request, name.String(),
				propertyInfo->type, value.String());
			if (error != B_OK)
				RETURN_ERROR(error);
		} else {
			// see if there is a type attribute
			type_code type = B_STRING_TYPE;
				// if there is no type specified, treat it as string

			BString typeName;
			if (xmlHelper->GetAttribute("type", typeName) == B_OK)
				error = _GetTypeForTypeName(typeName.String(), type);

			if (error == B_OK) {
				error = _ConvertFieldValueFromString(request, name.String(),
					type, value.String());
			}
			if (error != B_OK)
				RETURN_ERROR(error);
		}


		xmlHelper->CloseTag();
	}

	return B_OK;
}

// ConvertFromXML
status_t
RequestXMLConverter::ConvertFromXML(BDataIO& stream, BMessage* request)
{
	if (!request)
		return B_BAD_VALUE;

	XMLHelper* xmlHelper = create_xml_helper();
	if (!xmlHelper)
		return B_NO_MEMORY;

	ObjectDeleter<XMLHelper> deleter(xmlHelper);
	AutoLocker<XMLHelper> lock(xmlHelper);

	if (!lock.IsLocked())
		return B_ERROR;

	status_t ret = xmlHelper->Load(stream);
	if (ret < B_OK)
		return ret;

	return ConvertFromXML(xmlHelper, request);
}


// _ConvertFieldValueToString
status_t
RequestXMLConverter::_ConvertFieldValueToString(BMessage* request,
	const char* name, type_code type, int32 index, const char** data)
{
	switch (type) {
		case B_STRING_TYPE:
			return request->FindString(name, index, data);
		case B_BOOL_TYPE:
		{
			bool value;
			status_t error = request->FindBool(name, index, &value);
			if (error != B_OK)
				return error;
			*data = (value ? "true" : "false");
			return B_OK;
		}
		case B_INT32_TYPE:
		{
			int32 value;
			status_t error = request->FindInt32(name, index, &value);
			if (error != B_OK)
				return error;
			sprintf(fBuffer, "%ld", value);
			return B_OK;
		}
		case B_INT64_TYPE:
		{
			int64 value;
			status_t error = request->FindInt64(name, index, &value);
			if (error != B_OK)
				return error;
			sprintf(fBuffer, "%lld", value);
			return B_OK;
		}

		default:
			// unsupported type
			RETURN_ERROR(B_ERROR);
	}
}

// _ConvertFieldValueFromString
status_t
RequestXMLConverter::_ConvertFieldValueFromString(BMessage* request,
	const char* name, type_code type, const char* data)
{
	switch (type) {
		case B_STRING_TYPE:
			LOG_TRACE("_ConvertFieldValueFromString(%s, %s) string",
				name, data);
			return request->AddString(name, data);
		case B_BOOL_TYPE:
		{
			bool value = (strcasecmp(data, "true") == 0);
			LOG_TRACE("_ConvertFieldValueFromString(%s, %s) bool: %d",
				name, data, value);
			return request->AddBool(name, value);
		}
		case B_INT32_TYPE:
		{
			int32 value;
			if (sscanf(data, "%ld", &value) != 1)
				RETURN_ERROR(B_BAD_VALUE);
			LOG_TRACE("_ConvertFieldValueFromString(%s, %s) int32: %ld",
				name, data, value);
			return request->AddInt32(name, value);
		}
		case B_INT64_TYPE:
		{
			int64 value;
			if (sscanf(data, "%lld", &value) != 1)
				RETURN_ERROR(B_BAD_VALUE);
			LOG_TRACE("_ConvertFieldValueFromString(%s, %s) int64: %lld",
				name, data, value);
			return request->AddInt64(name, value);
		}
			
		default:
			// unsupported type
			RETURN_ERROR(B_ERROR);
	}
}

// _FindRequestInfo
const RequestXMLConverter::RequestInfo*
RequestXMLConverter::_FindRequestInfo(uint32 code)
{
	for (int32 i = 0; sRequestInfos[i].name; i++) {
		if (sRequestInfos[i].code == code)
			return sRequestInfos + i;
	}

	return NULL;
}

// _FindRequestInfo
const RequestXMLConverter::RequestInfo*
RequestXMLConverter::_FindRequestInfo(const char* name)
{
	for (int32 i = 0; sRequestInfos[i].name; i++) {
		if (strcmp(sRequestInfos[i].name, name) == 0)
			return sRequestInfos + i;
	}

	return NULL;
}

// _FindPropertyInfo
const RequestXMLConverter::RequestPropertyInfo*
RequestXMLConverter::_FindPropertyInfo(const RequestInfo* requestInfo,
	const char* name)
{
	for (int32 i = 0; requestInfo->properties[i].name; i++) {
		if (strcmp(requestInfo->properties[i].name, name) == 0)
			return requestInfo->properties + i;
	}

	return NULL;
}

// _GetTypeForTypeName
status_t
RequestXMLConverter::_GetTypeForTypeName(const char* name, type_code& type)
{
	if (!strcmp(name, "long"))
		type = B_INT64_TYPE;
	else if (!strcmp(name, "int"))
		type = B_INT32_TYPE;
	else if (!strcmp(name, "bool"))
		type = B_BOOL_TYPE;
	else if (!strcmp(name, "string"))
		type = B_STRING_TYPE;
	else
		return B_BAD_VALUE;

	return B_OK;
}

// _TypeNameForType
const char*
RequestXMLConverter::_TypeNameForType(type_code type)
{
	switch (type) {
		case B_INT64_TYPE:
			return "long";
		case B_INT32_TYPE:
			return "int";
		case B_BOOL_TYPE:
			return "bool";
		case B_STRING_TYPE:
			return "string";

		default:
			return NULL;
	}
}
