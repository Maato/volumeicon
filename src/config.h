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

//##############################################################################
// Setter functions
//##############################################################################

// Alsa
void config_set_card(const gchar *card);
void config_set_channel(const gchar *channel);

// Notifications
void config_set_show_notification(gboolean active);
void config_set_notification_type(gint type);

// Status icon
void config_set_stepsize(int stepsize);
void config_set_helper(const gchar *helper);
void config_set_theme(const gchar *theme);
void config_set_use_panel_specific_icons(gboolean active);

// Left mouse button action
void config_set_left_mouse_slider(gboolean active);

// Middle mouse button action
void config_set_middle_mouse_mute(gboolean active);

// Layout
void config_set_use_horizontal_slider(gboolean active);
void config_set_show_sound_level(gboolean active);
void config_set_use_transparent_background(gboolean active);

// Hotkeys
void config_set_hotkey_up_enabled(gboolean enabled);
void config_set_hotkey_down_enabled(gboolean enabled);
void config_set_hotkey_mute_enabled(gboolean enabled);
void config_set_hotkey_up(const gchar *up);
void config_set_hotkey_down(const gchar *down);
void config_set_hotkey_mute(const gchar *mute);

//##############################################################################
// Getter functions
//##############################################################################

// Alsa
const gchar *config_get_card(void);
const gchar *config_get_channel(void);

// Notifications
gboolean config_get_show_notification(void);
gint config_get_notification_type(void);

// Status icon
int config_get_stepsize(void);
const gchar *config_get_helper(void);
const gchar *config_get_theme(void);
gboolean config_get_use_gtk_theme(void);
gboolean config_get_use_panel_specific_icons(void);

// Left mouse button action
gboolean config_get_left_mouse_slider(void);

// Middle mouse button action
gboolean config_get_middle_mouse_mute(void);

// Layout
gboolean config_get_use_horizontal_slider(void);
gboolean config_get_show_sound_level(void);
gboolean config_get_use_transparent_background(void);

// Hotkeys
gboolean config_get_hotkey_up_enabled(void);
gboolean config_get_hotkey_down_enabled(void);
gboolean config_get_hotkey_mute_enabled(void);
const gchar *config_get_hotkey_up(void);
const gchar *config_get_hotkey_down(void);
const gchar *config_get_hotkey_mute(void);

//##############################################################################
// Miscellaneous functions
//##############################################################################

void config_write(void);
void config_initialize(gchar *config_name);

#endif

