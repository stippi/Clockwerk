/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <fstream>
#include <stdio.h>

#include <DataIO.h>

#include <DOM_Element.hpp>
#include <DOM_NamedNodeMap.hpp>
#include <dom/DOM_DOMException.hpp>
#include <parsers/DOMParser.hpp>
#include <sax/ErrorHandler.hpp>
#include <sax/SAXException.hpp>
#include <sax/SAXParseException.hpp>

#include "XercesDOMHelper.h"
#include "DataIOInputSource.h"
#include "DOMNodeWriter.h"
#include "XercesSupport.h"
#include "XMLStorable.h"


// ---------------------------------------------------------------------------
//  Simple error handler deriviative to install on parser
// ---------------------------------------------------------------------------
class DOMCountErrorHandler : public ErrorHandler {
public:
	// -----------------------------------------------------------------------
	//  Constructors and Destructor
	// -----------------------------------------------------------------------
	DOMCountErrorHandler();
	~DOMCountErrorHandler();


	// -----------------------------------------------------------------------
	//  Getter methods
	// -----------------------------------------------------------------------
	bool getSawErrors() const;


	// -----------------------------------------------------------------------
	//  Implementation of the SAX ErrorHandler interface
	// -----------------------------------------------------------------------
	void warning(const SAXParseException& e);
	void error(const SAXParseException& e);
	void fatalError(const SAXParseException& e);
	void resetErrors();


private :
	// -----------------------------------------------------------------------
	//  Unimplemented constructors and operators
	// -----------------------------------------------------------------------
	DOMCountErrorHandler(const DOMCountErrorHandler&);
	void operator=(const DOMCountErrorHandler&);


	// -----------------------------------------------------------------------
	//  Private data members
	//
	//  fSawErrors
	//	  This is set if we get any errors, and is queryable via a getter
	//	  method. Its used by the main code to suppress output if there are
	//	  errors.
	// -----------------------------------------------------------------------
	bool	fSawErrors;
};


DOMCountErrorHandler::DOMCountErrorHandler() :

	fSawErrors(false)
{
}


DOMCountErrorHandler::~DOMCountErrorHandler()
{
}


inline bool
DOMCountErrorHandler::getSawErrors() const
{
	return fSawErrors;
}


// ---------------------------------------------------------------------------
//  DOMCountHandlers: Overrides of the SAX ErrorHandler interface
// ---------------------------------------------------------------------------
void
DOMCountErrorHandler::error(const SAXParseException& e)
{
	fSawErrors = true;
	std::cerr << "\nError at file " << StrX(e.getSystemId())
		<< ", line " << e.getLineNumber()
		<< ", char " << e.getColumnNumber()
		<< "\n  Message: " << StrX(e.getMessage()) << std::endl;
}


void
DOMCountErrorHandler::fatalError(const SAXParseException& e)
{
	fSawErrors = true;
	std::cerr << "\nFatal Error at file " << StrX(e.getSystemId())
		<< ", line " << e.getLineNumber()
		<< ", char " << e.getColumnNumber()
		<< "\n  Message: " << StrX(e.getMessage()) << std::endl;
}


void
DOMCountErrorHandler::warning(const SAXParseException& e)
{
	std::cerr << "\nWarning at file " << StrX(e.getSystemId())
		 << ", line " << e.getLineNumber()
		 << ", char " << e.getColumnNumber()
		 << "\n  Message: " << StrX(e.getMessage()) << std::endl;
}


void
DOMCountErrorHandler::resetErrors()
{
}




// constructor
XercesDOMHelper::XercesDOMHelper()
	: XMLHelper(),
	  fDocument(),
	  fElementStack(),
	  fCurrentElement()
{
	Init();
}

// destructor
XercesDOMHelper::~XercesDOMHelper()
{
}

// Init
void
XercesDOMHelper::Init()
{
	Init(DOM_Document());
}

// Init
void
XercesDOMHelper::Init(const char* rootTagname)
{
	if (rootTagname) {
		DOM_Document document =  DOM_Document::createDocument();
		document.appendChild(document.createElement(rootTagname));
		Init(document);
	} else
		Init();
}

// Init
void
XercesDOMHelper::Init(const DOM_Document& document)
{
	fDocument = document;
	fElementStack = NodeStack();
	fCurrentElement = 0;
	// If the document is null, create a new one.
	if (fDocument == 0)
		fDocument = DOM_Document::createDocument();
	// If the document has no root element, create one.
	if (fDocument.getDocumentElement() == 0)
		fDocument.appendChild(fDocument.createElement("Document"));
	// push the root document
	_PushElement(fDocument.getDocumentElement());
}

// Load
status_t
XercesDOMHelper::Load(const char* filename)
{
	return XMLHelper::Load(filename);
}

// Load
status_t
XercesDOMHelper::Load(BDataIO& input)
{
	status_t error = B_ERROR;
	// Instantiate the DOM parser.
	DOMParser parser;
	DOMCountErrorHandler errorHandler;
	parser.setErrorHandler(&errorHandler);
	try {
		DataIOInputSource inputSource(&input);
		parser.parse(inputSource);
		//
		if (errorHandler.getSawErrors()) {
			std::cout << "\nErrors occured, no output available\n"
				<< std::endl;
		} else {
			Init(parser.getDocument());
			error = B_OK;
		}
	} catch (const XMLException& toCatch) {
		std::cerr << "\nError during parsing: \n"
		<< "Exception message is:  \n"
		<< StrX(toCatch.getMessage()) << "\n" << std::endl;
	} catch (const DOM_DOMException& toCatch) {
		std::cerr << "\nDOM Error during parsing: \n"
		<< "DOMException code is:  \n"
		<< (int32)toCatch.code << "\n" << std::endl;
	} catch (...) {
		std::cerr << "\nUnexpected exception during parsing: \n";
	}
	return error;
}

// Save
status_t
XercesDOMHelper::Save(const char* filename)
{
	return XMLHelper::Save(filename);
}

// Save
status_t
XercesDOMHelper::Save(BDataIO& output)
{
	DOMNodeWriter writer(output);
	return writer.WriteNode(fDocument.getDocumentElement());
}

// CreateTag
status_t
XercesDOMHelper::CreateTag(const char* tagname)
{
	status_t error = B_OK;
	if (tagname) {
		DOM_Node& topElement = _TopElement();
		if (topElement != 0) {
			DOM_Node element = fDocument.createElement(tagname);
//			topElement.appendChild(element);
			if (fCurrentElement != 0
				&& fCurrentElement.getParentNode() == topElement) {
				topElement.insertBefore(element,
										fCurrentElement.getNextSibling());
			} else
				topElement.insertBefore(element, topElement.getFirstChild());
			_PushElement(element);
		} else
			error = B_ERROR;
	} else
		error = B_BAD_VALUE;
	return error;
}

// OpenTag
status_t
XercesDOMHelper::OpenTag()
{
	status_t error = B_OK;
	DOM_Node& element = _NextElement();
	if (element != 0)
		_PushElement(element);
	else
		error = B_ERROR;
	return error;
}

// OpenTag
status_t
XercesDOMHelper::OpenTag(const char* tagname)
{
	status_t error = B_OK;
	if (tagname) {
		DOM_Node& element = _NextElement(tagname);
		if (element != 0)
			_PushElement(element);
		else
			error = B_ERROR;
	} else
		error = B_BAD_VALUE;
	return error;
}

// CloseTag
status_t
XercesDOMHelper::CloseTag()
{
	status_t error = B_OK;
	if (_PopElement() == 0)
		error = B_ERROR;
	return error;
}

// RewindTag
status_t
XercesDOMHelper::RewindTag()
{
	status_t error = B_OK;
	fCurrentElement = 0;
	return error;
}

// GetTagName
status_t
XercesDOMHelper::GetTagName(BString& name) const
{
	status_t error = B_OK;
	DOM_Node element = _TopElement();
	if (element != 0) {
		if (char* nodeName = element.getNodeName().transcode()) {
			name = nodeName;
			delete[] nodeName;
		} else
			error = B_ERROR;
	} else
		error = B_ERROR;
	return error;
}

// SetAttribute
status_t
XercesDOMHelper::SetAttribute(const char* name, const char* value)
{
	status_t error = B_ERROR;
	if (name && value) {
		DOM_Node& element = _TopElement();
		if (element != 0) {
			DOM_NamedNodeMap attributes = element.getAttributes();
			if (attributes != 0) {
				DOM_Attr attribute = fDocument.createAttribute(name);
				attribute.setNodeValue(value);
				attributes.setNamedItem(attribute);
				error = B_OK;
			}
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// GetAttribute
status_t
XercesDOMHelper::GetAttribute(const char* name, BString& value)
{
	status_t error = B_ERROR;
	if (name) {
		DOM_Node& element = _TopElement();
		if (element != 0) {
			DOM_NamedNodeMap attributes = element.getAttributes();
			if (attributes != 0) {
				DOM_Node attribute = attributes.getNamedItem(name);
				if (attribute != 0) {
					char *attrValue = attribute.getNodeValue().transcode();
					value.SetTo(attrValue);
					delete[] attrValue;
					error = B_OK;
				}
			}
		}
	} else
		error = B_BAD_VALUE;
	return error;
}

// HasAttribute
bool
XercesDOMHelper::HasAttribute(const char* name)
{
	bool result = false;
	if (name) {
		DOM_Node& element = _TopElement();
		if (element != 0) {
			DOM_NamedNodeMap attributes = element.getAttributes();
			if (attributes != 0)
				result = (attributes.getNamedItem(name) != 0);
		}
	}
	return result;
}

// _PushElement
void
XercesDOMHelper::_PushElement(const DOM_Node& element)
{
	if (element != 0) {
		fElementStack.push(element);
		fCurrentElement = 0;
	}
}

// _PopElement
DOM_Node&
XercesDOMHelper::_PopElement()
{
	fCurrentElement = fElementStack.top();
	fElementStack.pop();
	return fCurrentElement;
}

// _TopElement
DOM_Node&
XercesDOMHelper::_TopElement() const
{
	return fElementStack.top();
}

// _NextElement
//
// Set the current element to and returns the next element within the
// top element. If there is no next element, the current element is set
// to null.
DOM_Node&
XercesDOMHelper::_NextElement()
{
	// get the next available node
	if (fCurrentElement == 0) {
		DOM_Node& parent = _TopElement();
		if (parent != 0)
			fCurrentElement = parent.getFirstChild();
	} else
		fCurrentElement = fCurrentElement.getNextSibling();
	// get the first node that is an element
	while (fCurrentElement != 0
		   && fCurrentElement.getNodeType() != DOM_Node::ELEMENT_NODE) {
		fCurrentElement = fCurrentElement.getNextSibling();
	}
	return fCurrentElement;
}

// _NextElement
//
// Set the current element to and returns the next element within the
// top element that has the supplied name (ignoring case). If there is no
// next element, the current element is set to null.
DOM_Node&
XercesDOMHelper::_NextElement(const char* tagname)
{
	DOMString name(tagname);
	unsigned int nameLen = name.length();
	bool go = true;
	do {
		_NextElement();
		go = (fCurrentElement != 0);
		if (go) {
			DOMString nodeName = fCurrentElement.getNodeName();
			unsigned int nodeNameLen = nodeName.length();
			if (nodeNameLen == nameLen
				&& XMLString::compareNIString(name.rawBuffer(),
											  nodeName.rawBuffer(),
											  nodeNameLen) == 0) {
				go = false;
			}
		}
	} while (go);
	return fCurrentElement;
}

