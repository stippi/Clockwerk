/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TRACK_PROPERTIES_H
#define TRACK_PROPERTIES_H

#include <String.h>

//#include "XMLStorable.h"

class BMessage;

class TrackProperties /*: public XMLStorable*/ {
public:
								TrackProperties(uint32 track);
								TrackProperties(const BMessage* archive);
								TrackProperties(const TrackProperties& other);
	virtual						~TrackProperties();

			TrackProperties&	operator=(const TrackProperties& other);
			bool				operator==(const TrackProperties& other) const;
			bool				operator!=(const TrackProperties& other) const;

	// XMLStorable interface
//	virtual	status_t			XMLStore(XMLHelper& xmlHelper) const;
//	virtual	status_t			XMLRestore(XMLHelper& xmlHelper);

			status_t			Archive(BMessage* into,
									bool deep = true) const;
			status_t			Unarchive(const BMessage* from);

			void				SetTrack(uint32 track);
	inline	uint32				Track() const
									{ return fTrack; }

			void				SetEnabled(bool enabled);
	inline	bool				IsEnabled() const
									{ return fEnabled; }

			void				SetName(const BString& name);
	inline	BString				Name() const
									{ return fName; }

			void				SetAlpha(uint8 alpha);
	inline	uint8				Alpha() const
									{ return fAlpha; }

			bool				IsDefault() const;

 private:
			uint32				fTrack;
			bool				fEnabled;
			BString				fName;
			uint8				fAlpha;
};

#endif // TRACK_PROPERTIES_H
