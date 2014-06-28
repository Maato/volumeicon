//##############################################################################
// volumeicon
//
// config.h - a singleton providing configuration values/functions
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

#ifndef __CONFIG_H__
#define __CONFIG_H__

void config_write();
void config_initialize();

void config_set_helper(const gchar * helper);
void config_set_theme(const gchar * theme);
void config_set_use_panel_specific_icons(gboolean active);
void config_set_card(const gchar * card);
void config_set_channel(const gchar * channel);
void config_set_decibel_scale(gboolean decibel_scale);
void config_set_stepsize(int stepsize);
void config_set_left_mouse_slider(gboolean active);
void config_set_middle_mouse_mute(gboolean active);
void config_set_use_horizontal_slider(gboolean active);
void config_set_show_sound_level(gboolean active);
void config_set_hotkey_up(const gchar * up);
void config_set_hotkey_down(const gchar * down);
void config_set_hotkey_mute(const gchar * mute);
void config_set_hotkey_up_enabled(gboolean enabled);
void config_set_hotkey_down_enabled(gboolean enabled);
void config_set_hotkey_mute_enabled(gboolean enabled);
void config_set_use_transparent_background(gboolean active);
void config_set_show_notification(gboolean active);
void config_set_notification_type(gint type);

const gchar * config_get_helper();
const gchar * config_get_theme();
gboolean config_get_use_panel_specific_icons();
const gchar * config_get_card();
const gchar * config_get_channel();
gboolean config_get_decibel_scale();
gboolean config_get_use_gtk_theme();
gboolean config_get_left_mouse_slider();
gboolean config_get_middle_mouse_mute();
gboolean config_get_use_horizontal_slider();
gboolean config_get_show_sound_level();
int config_get_stepsize();
const gchar * config_get_hotkey_up();
const gchar * config_get_hotkey_down();
const gchar * config_get_hotkey_mute();
gboolean config_get_hotkey_up_enabled();
gboolean config_get_hotkey_down_enabled();
gboolean config_get_hotkey_mute_enabled();
gboolean config_get_use_transparent_background();
gboolean config_get_show_notification();
gint config_get_notification_type();

#endif

