//##############################################################################
// volumeicon
//
// alsa_backend.h - implements a volume control abstraction using alsa-lib
// 
// Copyright 2011 Maato
//
// Authors:
//    Maato <maato@softwarebakery.com>
//
// This program is free software: you can redistribute it and/or modify it
// under the terms of the GNU General Public License version 3, as published
// by the Free Software Foundation.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranties of
// MERCHANTABILITY, SATISFACTORY QUALITY, or FITNESS FOR A PARTICULAR
// PURPOSE.  See the GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this program.  If not, see <http://www.gnu.org/licenses/>.
//##############################################################################

#ifndef __ALSA_BACKEND_H__
#define __ALSA_BACKEND_H__

void asound_setup(const gchar * card, const gchar * channel,
	void (*volume_changed)(int,gboolean));

void asound_set_channel(const gchar * channel);
void asound_set_volume(int volume);
void asound_set_mute(gboolean mute);

int asound_get_volume();
gboolean asound_get_mute();
const gchar * asound_get_channel();
const GList * asound_get_channel_names();
const gchar * asound_get_device();
const GList * asound_get_device_names();

#endif
