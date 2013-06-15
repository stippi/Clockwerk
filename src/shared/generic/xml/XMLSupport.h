/*
 * Copyright 2002-2004, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_SUPPORT_H
#define XML_SUPPORT_H

#include <SupportDefs.h>

class XMLHelper;

status_t	init_xml();
void		uninit_xml();
XMLHelper*	create_xml_helper();


#endif	// XML_SUPPORT_H
