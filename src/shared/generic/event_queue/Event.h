/*
 * Copyright 2001-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2001-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef EVENT_H
#define EVENT_H

#include <OS.h>

class Event {
 public:
								Event(bool autoDelete = true);
								Event(bigtime_t time, bool autoDelete = true);
	virtual						~Event();

			void				SetTime(bigtime_t time);
			bigtime_t			Time() const;

			void				SetAutoDelete(bool autoDelete);
			bool				AutoDelete() const
									{ return fAutoDelete; }

	virtual	void				Execute();

 private:
			bigtime_t			fTime;
			bool				fAutoDelete;
};

#endif	// EVENT_H
