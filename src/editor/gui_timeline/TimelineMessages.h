/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#ifndef TIMELINE_MESSAGES_H
#define TIMELINE_MESSAGES_H

enum {
	MSG_DRAG_CLIP				= 'drgc',

	// context menu message constants
	MSG_REMOVE_ITEM				= 'rmim',
	MSG_SET_VIDEO_MUTED			= 'mtvd',
	MSG_SET_AUDIO_MUTED			= 'mtad',
	MSG_SELECT_AND_SHOW_CLIP	= 'slcp',

	MSG_ADD_NAVIGATOR_INFO		= 'adni',
	MSG_EDIT_NAVIGATOR_INFO		= 'edni',
	MSG_REMOVE_NAVIGATOR_INFO	= 'rmni',
	MSG_SET_NAVIGATOR_INFO		= 'stni'
};


#endif // TIMELINE_MESSAGES_H
