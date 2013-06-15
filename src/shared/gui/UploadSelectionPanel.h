/*
 * Copyright 2007-2008, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef UPLOAD_SELECTION_PANEL_H
#define UPLOAD_SELECTION_PANEL_H

#include <Window.h>

#include "Panel.h"
#include "StatusOutput.h"

class BButton;
class BStringView;
class Uploader;
class UploadObjectListView;

class UploadSelectionPanel : public BWindow {
public:
								UploadSelectionPanel(BMessage& settings,
									BWindow* window);
	virtual						~UploadSelectionPanel();

	virtual	bool				QuitRequested();

	virtual void				MessageReceived(BMessage *message);

	// UploadSelectionPanel
			status_t			InitFromUploader(const Uploader* uploader);

			enum return_code {
				RETURN_OK		= 0,
				RETURN_CANCEL
			};

			return_code			Go(bool centerInParentFrame = false);

private:
	sem_id						fExitSemaphore;
	return_code					fReturnCode;

	BButton*					fUploadB;
	BButton*					fCancelB;
	BStringView*				fInfoView;

	UploadObjectListView*		fUploadListView;

	BWindow*					fWindow;
	BMessage&					fSettings;
};

#endif // UPLOAD_SELECTION_PANEL_H
