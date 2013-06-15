/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include <DOM_Document.hpp>
#include <DOM_Node.hpp>
#include <DOM_NamedNodeMap.hpp>
#include <XMLFormatter.hpp>
#include <XMLString.hpp>
#include <XMLUni.hpp>
#include <XMLUniDefs.hpp>

#include "DOMNodeWriter.h"
#include "XercesSupport.h"

const int32 kDefaultMaximalIndentLevel   = 10;
const int32 kDefaultSpacesPerIndentLevel = 4;


// DOMNodeWriterIOException

struct DOMNodeWriterIOException
{
};



// DataIOFormatTarget

// constructor
DOMNodeWriter::DataIOFormatTarget::DataIOFormatTarget(BDataIO& target)
	: fTarget(target)
{
}

// destructor
DOMNodeWriter::DataIOFormatTarget::~DataIOFormatTarget()
{
}

// writeChars
void
DOMNodeWriter::DataIOFormatTarget::writeChars(const XMLByte* toWrite,
											  const unsigned int count,
											  XMLFormatter* const formatter)
{
	ssize_t error = fTarget.Write(toWrite, count);
	if (error != (ssize_t)count)
		throw DOMNodeWriterIOException();
}



// DOMNodeWriter

// constructor
DOMNodeWriter::DOMNodeWriter(BDataIO& target)
	: fFormatTarget(target),
	  fEncodingName(XMLUni::fgUTF8EncodingString),
	  fFormatter(fEncodingName, &fFormatTarget, XMLFormatter::NoEscapes,
				 XMLFormatter::UnRep_CharRef),
	  fIndentLevel(0),
	  fMaximalIndentLevel(kDefaultMaximalIndentLevel),
	  fSpacesPerIndentLevel(kDefaultSpacesPerIndentLevel),
	  fIndentSpaces(NULL)
{
	_RecreateIndentSpaces();
}

// destructor
DOMNodeWriter::~DOMNodeWriter()
{
	delete[] fIndentSpaces;
}

// SetMaximalIndentLevel
void
DOMNodeWriter::SetMaximalIndentLevel(uint32 level)
{
	if (level > 0) {
		fMaximalIndentLevel = level;
		_RecreateIndentSpaces();
	}
}

// GetMaximalIndentLevel
uint32
DOMNodeWriter::GetMaximalIndentLevel() const
{
	return fMaximalIndentLevel;
}

// SetSpacesPerIndentLevel
void
DOMNodeWriter::SetSpacesPerIndentLevel(uint32 spaces)
{
	fSpacesPerIndentLevel = spaces;
	_RecreateIndentSpaces();
}

// GetSpacesPerIndentLevel
uint32
DOMNodeWriter::GetSpacesPerIndentLevel() const
{
	return fSpacesPerIndentLevel;
}

// WriteNode
status_t
DOMNodeWriter::WriteNode(const DOM_Node& node)
{
	status_t error = B_OK;
	try {
fIndentLevel = 0;
		_WriteNode(node);
	} catch (...) {
		error = B_ERROR;
	}
	return error;
}


// ---------------------------------------------------------------------------
//  Local const data
//
//  Note: This is the 'safe' way to do these strings. If you compiler supports
//        L"" style strings, and portability is not a concern, you can use
//        those types constants directly.
// ---------------------------------------------------------------------------
static const XMLCh  gEndElement[] = { chOpenAngle, chForwardSlash, chNull };
static const XMLCh  gEndPI[] = { chQuestion, chCloseAngle, chNull};
static const XMLCh  gStartPI[] = { chOpenAngle, chQuestion, chNull };
static const XMLCh  gXMLDecl1[] =
{
        chOpenAngle, chQuestion, chLatin_x, chLatin_m, chLatin_l
    ,   chSpace, chLatin_v, chLatin_e, chLatin_r, chLatin_s, chLatin_i
    ,   chLatin_o, chLatin_n, chEqual, chDoubleQuote, chNull
};
static const XMLCh  gXMLDecl2[] =
{
        chDoubleQuote, chSpace, chLatin_e, chLatin_n, chLatin_c
    ,   chLatin_o, chLatin_d, chLatin_i, chLatin_n, chLatin_g, chEqual
    ,   chDoubleQuote, chNull
};
static const XMLCh  gXMLDecl3[] =
{
        chDoubleQuote, chSpace, chLatin_s, chLatin_t, chLatin_a
    ,   chLatin_n, chLatin_d, chLatin_a, chLatin_l, chLatin_o
    ,   chLatin_n, chLatin_e, chEqual, chDoubleQuote, chNull
};
static const XMLCh  gXMLDecl4[] =
{
        chDoubleQuote, chQuestion, chCloseAngle
    ,   chLF, chNull
};

static const XMLCh  gStartCDATA[] =
{
        chOpenAngle, chBang, chOpenSquare, chLatin_C, chLatin_D,
        chLatin_A, chLatin_T, chLatin_A, chOpenSquare, chNull
};

static const XMLCh  gEndCDATA[] =
{
    chCloseSquare, chCloseSquare, chCloseAngle, chNull
};
static const XMLCh  gStartComment[] =
{
    chOpenAngle, chBang, chDash, chDash, chNull
};

static const XMLCh  gEndComment[] =
{
    chDash, chDash, chCloseAngle, chNull
};

static const XMLCh  gStartDoctype[] =
{
    chOpenAngle, chBang, chLatin_D, chLatin_O, chLatin_C, chLatin_T,
    chLatin_Y, chLatin_P, chLatin_E, chSpace, chNull
};
static const XMLCh  gPublic[] =
{
    chLatin_P, chLatin_U, chLatin_B, chLatin_L, chLatin_I,
    chLatin_C, chSpace, chDoubleQuote, chNull
};
static const XMLCh  gSystem[] =
{
    chLatin_S, chLatin_Y, chLatin_S, chLatin_T, chLatin_E,
    chLatin_M, chSpace, chDoubleQuote, chNull
};
static const XMLCh  gStartEntity[] =
{
    chOpenAngle, chBang, chLatin_E, chLatin_N, chLatin_T, chLatin_I,
    chLatin_T, chLatin_Y, chSpace, chNull
};
static const XMLCh  gNotation[] =
{
    chLatin_N, chLatin_D, chLatin_A, chLatin_T, chLatin_A,
    chSpace, chDoubleQuote, chNull
};


// _WriteNode
void
DOMNodeWriter::_WriteNode(const DOM_Node& toWrite)
{
	// Get the name and value out for convenience
	DOMString   nodeName = toWrite.getNodeName();
	DOMString   nodeValue = toWrite.getNodeValue();
	unsigned long lent = nodeValue.length();
	
	switch (toWrite.getNodeType())
	{
		case DOM_Node::TEXT_NODE:
		{
			fFormatter.formatBuf(nodeValue.rawBuffer(),
				lent, XMLFormatter::CharEscapes);
			break;
		}
		case DOM_Node::PROCESSING_INSTRUCTION_NODE :
		{
			fFormatter << XMLFormatter::NoEscapes << gStartPI  << nodeName;
			if (lent > 0)
				fFormatter << chSpace << nodeValue;
			fFormatter << XMLFormatter::NoEscapes << gEndPI;
			break;
		}
		case DOM_Node::DOCUMENT_NODE :
		{
			DOM_Node child = toWrite.getFirstChild();
			while (child != 0)
			{
				_WriteNode(child);
				child = child.getNextSibling();
			}
			break;
		}
		case DOM_Node::ELEMENT_NODE :
		{
fIndentLevel++;
_IndentElementTag();
			// The name has to be representable without any escapes
			fFormatter  << XMLFormatter::NoEscapes
				<< chOpenAngle << nodeName;

			// Output the element start tag.

			// Output any attributes on this element
			DOM_NamedNodeMap attributes = toWrite.getAttributes();
			int attrCount = attributes.getLength();
			for (int i = 0; i < attrCount; i++)
			{
				DOM_Node  attribute = attributes.item(i);
				//
				//  Again the name has to be completely representable. But the
				//  attribute can have refs and requires the attribute style
				//  escaping.
				//
				fFormatter  << XMLFormatter::NoEscapes
					<< chSpace << attribute.getNodeName()
					<< chEqual << chDoubleQuote
					<< XMLFormatter::AttrEscapes
					<< attribute.getNodeValue()
					<< XMLFormatter::NoEscapes
					<< chDoubleQuote;
			}
			//
			//  Test for the presence of children, which includes both
			//  text content and nested elements.
			//
			DOM_Node child = toWrite.getFirstChild();
			if (child != 0)
			{
				// There are children. Close start-tag, and output children.
				// No escapes are legal here
				fFormatter << XMLFormatter::NoEscapes << chCloseAngle;
fFormatter << XMLFormatter::NoEscapes << chLF;
				while( child != 0)
				{
					_WriteNode(child);
					child = child.getNextSibling();
				}
				//
				// Done with children.  Output the end tag.
				//
_IndentElementTag();
				fFormatter << XMLFormatter::NoEscapes << gEndElement
					<< nodeName << chCloseAngle;
			}
			else
			{
				//
				//  There were no children. Output the short form close of
				//  the element start tag, making it an empty-element tag.
				//
				fFormatter << XMLFormatter::NoEscapes << chForwardSlash << chCloseAngle;
			}
fFormatter << XMLFormatter::NoEscapes << chLF;
fIndentLevel--;
			break;
		}
		case DOM_Node::ENTITY_REFERENCE_NODE:
		{
			DOM_Node child;
#if 0
			for (child = toWrite.getFirstChild();
				child != 0;
				child = child.getNextSibling())
			{
				_WriteNode(child);
			}
#else
			//
			// Instead of printing the refernece tree
			// we'd output the actual text as it appeared in the xml file.
			// This would be the case when -e option was chosen
			//
			fFormatter << XMLFormatter::NoEscapes << chAmpersand
			<< nodeName << chSemiColon;
#endif
			break;
		}
		case DOM_Node::CDATA_SECTION_NODE:
		{
			fFormatter << XMLFormatter::NoEscapes << gStartCDATA
				<< nodeValue << gEndCDATA;
			break;
		}
		case DOM_Node::COMMENT_NODE:
		{
			fFormatter << XMLFormatter::NoEscapes << gStartComment
				<< nodeValue << gEndComment;
			break;
		}
		case DOM_Node::DOCUMENT_TYPE_NODE:
		{
			DOM_DocumentType doctype = (DOM_DocumentType &)toWrite;;

			fFormatter << XMLFormatter::NoEscapes  << gStartDoctype
				<< nodeName;

			DOMString id = doctype.getPublicId();
			if (id != 0)
			{
				fFormatter << XMLFormatter::NoEscapes << chSpace << gPublic
					<< id << chDoubleQuote;
				id = doctype.getSystemId();
				if (id != 0)
				{
					fFormatter << XMLFormatter::NoEscapes << chSpace
						<< chDoubleQuote << id << chDoubleQuote;
				}
			}
			else
			{
				id = doctype.getSystemId();
				if (id != 0)
				{
					fFormatter << XMLFormatter::NoEscapes << chSpace << gSystem
						<< id << chDoubleQuote;
				}
			}

			id = doctype.getInternalSubset();
			if (id != 0)
				fFormatter << XMLFormatter::NoEscapes << chOpenSquare
					<< id << chCloseSquare;

			fFormatter << XMLFormatter::NoEscapes << chCloseAngle;
			break;
		}
		case DOM_Node::ENTITY_NODE:
		{
			fFormatter << XMLFormatter::NoEscapes << gStartEntity
				<< nodeName;

			DOMString id = ((DOM_Entity &)toWrite).getPublicId();
			if (id != 0)
				fFormatter << XMLFormatter::NoEscapes << gPublic
					<< id << chDoubleQuote;

			id = ((DOM_Entity &)toWrite).getSystemId();
			if (id != 0)
				fFormatter << XMLFormatter::NoEscapes << gSystem
					<< id << chDoubleQuote;

			id = ((DOM_Entity &)toWrite).getNotationName();
			if (id != 0)
				fFormatter << XMLFormatter::NoEscapes << gNotation
					<< id << chDoubleQuote;

			fFormatter << XMLFormatter::NoEscapes << chCloseAngle << chLF;

			break;
		}
		case DOM_Node::XML_DECL_NODE:
		{
			fFormatter << gXMLDecl1 << ((DOM_XMLDecl &)toWrite).getVersion();
			fFormatter << gXMLDecl2 << fEncodingName;
			DOMString str = ((DOM_XMLDecl &)toWrite).getStandalone();
			if (str != 0)
				fFormatter << gXMLDecl3 << str;
			fFormatter << gXMLDecl4;
			break;
		}
		default:
			std::cerr << "Unrecognized node type = "
				<< (long)toWrite.getNodeType() << std::endl;
	}
}

// _IndentElementTag
void
DOMNodeWriter::_IndentElementTag()
{
	uint32 level = fIndentLevel - 1;
	if (level > fMaximalIndentLevel)
		level = fMaximalIndentLevel;
	uint32 spaces = level * fSpacesPerIndentLevel;
	fFormatter.formatBuf(fIndentSpaces, spaces, XMLFormatter::NoEscapes);
}

// _RecreateIndentSpaces
void
DOMNodeWriter::_RecreateIndentSpaces()
{
	delete[] fIndentSpaces;
	uint32 spacesNeeded = fMaximalIndentLevel * fSpacesPerIndentLevel;
	fIndentSpaces = new XMLCh[spacesNeeded + 1];
	for (uint32 i = 0; i < spacesNeeded;  i++)
		fIndentSpaces[i] = chSpace;
	fIndentSpaces[spacesNeeded] = chNull;
}
