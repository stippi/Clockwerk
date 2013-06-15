/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XERCES_DOM_HELPER_H
#define XERCES_DOM_HELPER_H

#include <stack>
#include <String.h>
#include <DOM_Document.hpp>
#include <DOM_Node.hpp>
#include "XMLHelper.h"

class BDataIO;
class XMLStorable;

class XercesDOMHelper : public XMLHelper
{
 public:
								XercesDOMHelper();
	virtual						~XercesDOMHelper();

	virtual	void				Init();
	virtual	void				Init(const char* rootTagname);
			void				Init(const DOM_Document& document);
	virtual	status_t			Load(const char* filename);
	virtual	status_t			Load(BDataIO& input);
	virtual	status_t			Save(const char* filename);
	virtual	status_t			Save(BDataIO& output);

	virtual	status_t			CreateTag(const char* name);
	virtual	status_t			OpenTag();
	virtual	status_t			OpenTag(const char* name);
	virtual	status_t			CloseTag();
	virtual	status_t			RewindTag();
	virtual	status_t			GetTagName(BString& name) const;

	virtual	status_t			SetAttribute(const char* name,
											 const char* value);
	virtual	status_t			GetAttribute(const char* name,
											 BString& value);
	virtual	bool				HasAttribute(const char* name);

 private:
			void				_PushElement(const DOM_Node& element);
			DOM_Node&			_PopElement();
			DOM_Node&			_TopElement() const;
			DOM_Node&			_NextElement();
			DOM_Node&			_NextElement(const char* tagname);

	typedef	std::stack<DOM_Node> NodeStack;

			DOM_Document		fDocument;
	mutable	NodeStack			fElementStack;
			DOM_Node			fCurrentElement;
};

#endif	// XERCES_DOM_HELPER_H
