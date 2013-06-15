/*
 * Copyright 2006-2009, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the GNU GPL v2.
 */

#include "PlaybackNavigator.h"

// static initializers
PlaybackNavigator* PlaybackNavigator::sDefaultInstance = NULL;

// constructor
PlaybackNavigator::PlaybackNavigator()
{
}

// destructor
PlaybackNavigator::~PlaybackNavigator()
{
}

// SetDefault
/*static*/ void
PlaybackNavigator::SetDefault(PlaybackNavigator* navigator)
{
	sDefaultInstance = navigator;
}

// DeleteDefault
/*static*/ void
PlaybackNavigator::DeleteDefault()
{
	delete sDefaultInstance;
}

// Default
/*static*/ PlaybackNavigator*
PlaybackNavigator::Default()
{
	return sDefaultInstance;
}

// Navigate
void
PlaybackNavigator::Navigate(const NavigationInfo* info)
{
}

