/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef CONFIG_VIEW_H
#define CONFIG_VIEW_H

#include <View.h>

class Tool;

class ConfigView : public BView {
 public:
								ConfigView(::Tool* tool);
	virtual						~ConfigView();

	// ie the interface language has changed
	virtual	void				UpdateStrings();
	// user switched tool
	virtual	void				SetActive(bool active);
	// en/disable controls
	virtual	void				SetEnabled(bool enable);

			::Tool*				Tool() const
									{ return fTool; }

 protected:
			::Tool*				fTool;
};

#endif // CONFIG_VIEW_H
