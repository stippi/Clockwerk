/*
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

// This class provides some basic functionality for derivation of a
// listener -> observer adapter.
// The derived class should implement constructors similar to the
// ones of this class and pass the respective parameter.
// Each of the listener hook functions should construct a message
// and let it be delivered by DeliverMessage().

#ifndef ABSTRACT_LO_ADAPTER_H
#define ABSTRACT_LO_ADAPTER_H

#include <SupportDefs.h>

class BHandler;
class BLooper;
class BMessage;
class BMessenger;

class AbstractLOAdapter {
 public:
								AbstractLOAdapter(BHandler* handler);
								AbstractLOAdapter(const BMessenger& messenger);
	virtual						~AbstractLOAdapter();

			void				DeliverMessage(BMessage* message);
			void				DeliverMessage(BMessage& message);
			void				DeliverMessage(uint32 command);

 private:
			BHandler*			fHandler;
			BMessenger*			fMessenger;
};

#endif	// ABSTRACT_LO_ADAPTER_H
