/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef DOM_NODE_WRITER_H
#define DOM_NODE_WRITER_H

#include <DataIO.h>

#include <XMLFormatter.hpp>

class DOM_Node;

class DOMNodeWriter {
 public:
								DOMNodeWriter(BDataIO& target);
	virtual						~DOMNodeWriter();

			void				SetMaximalIndentLevel(uint32 level);
			uint32				GetMaximalIndentLevel() const;
			void				SetSpacesPerIndentLevel(uint32 spaces);
			uint32				GetSpacesPerIndentLevel() const;

			status_t			WriteNode(const DOM_Node& node);

 private:
			void				_WriteNode(const DOM_Node& node);
			void				_IndentElementTag();
			void				_RecreateIndentSpaces();

 private:
	class DataIOFormatTarget : public XMLFormatTarget
	{
	 public:
								DataIOFormatTarget(BDataIO& target);
								~DataIOFormatTarget();
			void				writeChars(const XMLByte* toWrite,
										   const unsigned int count,
										   XMLFormatter* const formatter);
	 private:
			BDataIO&			fTarget;
	};

 private:
			DataIOFormatTarget	fFormatTarget;
			const XMLCh*		fEncodingName;
			XMLFormatter		fFormatter;
			uint32				fIndentLevel;
			uint32				fMaximalIndentLevel;
			uint32				fSpacesPerIndentLevel;
			XMLCh*				fIndentSpaces;
};

#endif	// DOM_NODE_WRITER_H
