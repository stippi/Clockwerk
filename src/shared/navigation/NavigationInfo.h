/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef NAVIGATION_INFO_H
#define NAVIGATION_INFO_H

#include <String.h>

//#include "XMLStorable.h"

class BMessage;

class NavigationInfo /*: public XMLStorable*/ {
public:
								NavigationInfo();
								NavigationInfo(const NavigationInfo& other);
								NavigationInfo(BMessage* archive);
	virtual						~NavigationInfo();

			bool				operator==(const NavigationInfo& other) const;

			status_t			Archive(BMessage* archive) const;

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

			void				SetTargetID(const char* id);
			const char*			TargetID() const
									{ return fTargetID.String(); }

private:
			BString				fTargetID;
};


#endif // NAVIGATION_INFO_H
