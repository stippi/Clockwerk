/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * Copyright 2006-2009, Ingo Weinhold <ingo_weinhold@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef COMPONENT_QUALITY_INFO_H
#define COMPONENT_QUALITY_INFO_H

#include <Entry.h>

class ComponentQualityInfo {
 public:
			int32				permanent_problem_count;
			int32				temporary_problem_count;

								ComponentQualityInfo();
								~ComponentQualityInfo();

			status_t			Init(const entry_ref& appRef);
			status_t			Save() const;

 private:
			struct Data;
			entry_ref			fRef;
};

#endif // COMPONENT_QUALITY_INFO_H
