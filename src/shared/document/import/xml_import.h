/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef XML_IMPORT_H
#define XML_IMPORT_H

#include <SupportDefs.h>

class BPositionIO;
class PropertyObject;
class ServerObjectFactory;
class ServerObjectManager;
class XMLHelper;

status_t
import_objects(ServerObjectManager* manager,
			   ServerObjectFactory* factory,
			   BPositionIO* stream);

status_t
restore_objects(XMLHelper& xml,
				ServerObjectManager* manager,
				ServerObjectFactory* factory);

status_t
restore_properties(XMLHelper& xml, PropertyObject* object);


#endif // XML_IMPORT_H
