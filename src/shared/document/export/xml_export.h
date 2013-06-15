/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_EXPORT_H
#define XML_EXPORT_H

#include <SupportDefs.h>

class BPositionIO;
class PropertyObject;
class ServerObjectManager;
class XMLHelper;

status_t
export_objects(ServerObjectManager* manager, BPositionIO* stream);

status_t
store_objects(XMLHelper& xml, ServerObjectManager* manager);

status_t
store_properties(XMLHelper& xml, PropertyObject* object);


#endif // XML_EXPORT_H
