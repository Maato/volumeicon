//##############################################################################
// volumeicon
//
// oss_backend.h - implements a volume control abstraction using oss
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

#ifndef __OSS_BACKEND_H__
#define __OSS_BACKEND_H__

void oss_setup(const gchar * card, const gchar * channel,
	void (*volume_changed)(int,gboolean));

void oss_set_channel(const gchar * channel);
void oss_set_volume(int volume);
void oss_set_mute(gboolean mute);

int oss_get_volume();
gboolean oss_get_mute();
const gchar * oss_get_channel();
const GList * oss_get_channel_names();

#endif
