//##############################################################################
// volumeicon
//
// config.c - a singleton providing configuration values/functions
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

#include <glib/gstdio.h>
#include <assert.h>

#include "config.h"

//##############################################################################
// Definitions
//##############################################################################
#define CONFIG_DIRNAME "volumeicon"
#define CONFIG_FILENAME "volumeicon"

//##############################################################################
// Static variables
//##############################################################################
static char * m_config_file = NULL;
static char * m_helper_program = NULL;
static gchar * m_theme = NULL;
static gboolean m_use_panel_specific_icons = FALSE;
static gchar * m_card = NULL;
static gchar * m_channel = NULL;
static gboolean m_decibel_scale = FALSE;
static gchar * m_hotkey_up = NULL;
static gchar * m_hotkey_down = NULL;
static gchar * m_hotkey_mute = NULL;
static int m_stepsize = 0;
static gboolean m_lmb_slider = FALSE;
static gboolean m_mmb_mute = FALSE;
static gboolean m_use_horizontal_slider = FALSE;
static gboolean m_show_sound_level = FALSE;
static gboolean m_hotkey_up_enabled = FALSE;
static gboolean m_hotkey_down_enabled = FALSE;
static gboolean m_hotkey_mute_enabled = FALSE;
static gboolean m_show_notification = TRUE;
static gint m_notification_type = 0;

//##############################################################################
// Static functions
//##############################################################################
static void config_load_default()
{
	if(!m_helper_program)
		config_set_helper(DEFAULT_MIXERAPP);
	if(!m_channel)
		config_set_channel(NULL);
	if(!m_card)
		config_set_card("default");
	if(!m_stepsize)
		config_set_stepsize(5);
	if(!m_theme)
		config_set_theme("Default");
	if(!m_hotkey_up)
		config_set_hotkey_up("XF86AudioRaiseVolume");
	if(!m_hotkey_down)
		config_set_hotkey_down("XF86AudioLowerVolume");
	if(!m_hotkey_mute)
		config_set_hotkey_mute("XF86AudioMute");
}

static void config_read()
{
	// Clean up previously loaded configuration values
	m_stepsize = 0;
	g_free(m_helper_program);
	g_free(m_channel);
	g_free(m_theme);

	// Load keys from keyfile
	GKeyFile * kf = g_key_file_new();
	g_key_file_load_from_file(kf, m_config_file, G_KEY_FILE_NONE, NULL);
	m_helper_program = g_key_file_get_value(kf, "StatusIcon", "onclick", NULL);
	m_card = g_key_file_get_value(kf, "Alsa", "card", NULL);
	m_channel = g_key_file_get_value(kf, "Alsa", "channel", NULL);
	m_decibel_scale = g_key_file_get_boolean(kf, "Alsa", "decibel_scale", NULL);
	m_stepsize = g_key_file_get_integer(kf, "StatusIcon", "stepsize", NULL);
	m_theme = g_key_file_get_value(kf, "StatusIcon", "theme", NULL);
	m_use_panel_specific_icons = g_key_file_get_boolean(kf, "StatusIcon", "use_panel_specific_icons", NULL);
	m_lmb_slider = g_key_file_get_boolean(kf, "StatusIcon", "lmb_slider", NULL);
	m_mmb_mute = g_key_file_get_boolean(kf, "StatusIcon", "mmb_mute", NULL);
	m_use_horizontal_slider = g_key_file_get_boolean(kf, "StatusIcon", "use_horizontal_slider", NULL);
	m_show_sound_level = g_key_file_get_boolean(kf, "StatusIcon", "show_sound_level", NULL);
	m_hotkey_up = g_key_file_get_value(kf, "Hotkeys", "up", NULL);
	m_hotkey_down = g_key_file_get_value(kf, "Hotkeys", "down", NULL);
	m_hotkey_mute = g_key_file_get_value(kf, "Hotkeys", "mute", NULL);
	m_hotkey_up_enabled = g_key_file_get_boolean(kf, "Hotkeys", "up_enabled", NULL);
	m_hotkey_down_enabled = g_key_file_get_boolean(kf, "Hotkeys", "down_enabled", NULL);
	m_hotkey_mute_enabled = g_key_file_get_boolean(kf, "Hotkeys", "mute_enabled", NULL);
    m_show_notification = g_key_file_get_boolean(kf, "Notification", "show_notification", NULL);
    m_notification_type = g_key_file_get_integer(kf, "Notification", "notification_type", NULL);
	g_key_file_free(kf);

	// Load default values for unset keys
	config_load_default();
}

//##############################################################################
// Exported functions
//##############################################################################
void config_set_helper(const gchar * helper)
{
	g_free(m_helper_program);
	m_helper_program = g_strdup(helper);
}

void config_set_theme(const gchar * theme)
{
	g_free(m_theme);
	m_theme = g_strdup(theme);
}

void config_set_use_panel_specific_icons(gboolean active)
{
	m_use_panel_specific_icons = active;
}

void config_set_card(const gchar * card)
{
	g_free(m_card);
	m_card = g_strdup(card);
}

void config_set_channel(const gchar * channel)
{
	g_free(m_channel);
	m_channel = g_strdup(channel);
}

void config_set_decibel_scale(gboolean decibel_scale)
{
	m_decibel_scale = decibel_scale;
}

void config_set_stepsize(int stepsize)
{
	m_stepsize = stepsize;
}

void config_set_left_mouse_slider(gboolean active)
{
	m_lmb_slider = active;
}

void config_set_middle_mouse_mute(gboolean active)
{
	m_mmb_mute = active;
}

void config_set_use_horizontal_slider(gboolean active)
{
	m_use_horizontal_slider = active;
}

void config_set_show_sound_level(gboolean active)
{
	m_show_sound_level = active;
}

void config_set_hotkey_up(const gchar * up)
{
	g_free(m_hotkey_up);
	m_hotkey_up = g_strdup(up);
}

void config_set_hotkey_down(const gchar * down)
{
	g_free(m_hotkey_down);
	m_hotkey_down = g_strdup(down);
}

void config_set_hotkey_mute(const gchar * mute)
{
	g_free(m_hotkey_mute);
	m_hotkey_mute = g_strdup(mute);
}

void config_set_hotkey_up_enabled(gboolean enabled)
{
	m_hotkey_up_enabled = enabled;
}

void config_set_hotkey_down_enabled(gboolean enabled)
{
	m_hotkey_down_enabled = enabled;
}

void config_set_hotkey_mute_enabled(gboolean enabled)
{
	m_hotkey_mute_enabled = enabled;
}

void config_set_show_notification(gboolean active)
{
    m_show_notification = active;
}

void config_set_notification_type(gint type)
{
    m_notification_type = type;
}

const gchar * config_get_helper()
{
	return m_helper_program;
}

const gchar * config_get_theme()
{
	return m_theme;
}

const gchar * config_get_card()
{
	return m_card;
}

const gchar * config_get_channel()
{
	return m_channel;
}

gboolean config_get_decibel_scale()
{
	return m_decibel_scale;
}

int config_get_stepsize()
{
	return m_stepsize;
}

gboolean config_get_use_gtk_theme()
{
	return g_strcmp0(m_theme, "Default") == 0 ? TRUE : FALSE;
}

gboolean config_get_use_panel_specific_icons()
{
	return m_use_panel_specific_icons;
}

gboolean config_get_left_mouse_slider()
{
	return m_lmb_slider;
}

gboolean config_get_middle_mouse_mute()
{
	return m_mmb_mute;
}

gboolean config_get_use_horizontal_slider()
{
	return m_use_horizontal_slider;
}

gboolean config_get_show_sound_level()
{
	return m_show_sound_level;
}

const gchar * config_get_hotkey_up()
{
	return m_hotkey_up;
}

const gchar * config_get_hotkey_down()
{
	return m_hotkey_down;
}

const gchar * config_get_hotkey_mute()
{
	return m_hotkey_mute;
}

gboolean config_get_hotkey_up_enabled()
{
	return m_hotkey_up_enabled;
}

gboolean config_get_hotkey_down_enabled()
{
	return m_hotkey_down_enabled;
}

gboolean config_get_hotkey_mute_enabled()
{
	return m_hotkey_mute_enabled;
}

gboolean config_get_show_notification()
{
    return m_show_notification;
}

gint config_get_notification_type()
{
    return m_notification_type;
}

void config_write()
{
	assert(m_config_file != NULL);

	GKeyFile * kf = g_key_file_new();
	g_key_file_set_integer(kf, "StatusIcon", "stepsize", m_stepsize);
	g_key_file_set_boolean(kf, "StatusIcon", "lmb_slider", m_lmb_slider);
	g_key_file_set_boolean(kf, "StatusIcon", "mmb_mute", m_mmb_mute);
	g_key_file_set_boolean(kf, "StatusIcon", "use_horizontal_slider", m_use_horizontal_slider);
	g_key_file_set_boolean(kf, "StatusIcon", "show_sound_level", m_show_sound_level);
	g_key_file_set_boolean(kf, "StatusIcon", "use_panel_specific_icons", m_use_panel_specific_icons);
	g_key_file_set_boolean(kf, "Hotkeys", "up_enabled", m_hotkey_up_enabled);
	g_key_file_set_boolean(kf, "Hotkeys", "down_enabled", m_hotkey_down_enabled);
	g_key_file_set_boolean(kf, "Hotkeys", "mute_enabled", m_hotkey_mute_enabled);
	g_key_file_set_boolean(kf, "Alsa", "decibel_scale", m_decibel_scale);
	if(m_helper_program)
		g_key_file_set_value(kf, "StatusIcon", "onclick", m_helper_program);
	if(m_theme)
		g_key_file_set_value(kf, "StatusIcon", "theme", m_theme);
	if(m_card)
		g_key_file_set_value(kf, "Alsa", "card", m_card);
	if(m_channel)
		g_key_file_set_value(kf, "Alsa", "channel", m_channel);
	if(m_hotkey_up)
		g_key_file_set_value(kf, "Hotkeys", "up", m_hotkey_up);
	if(m_hotkey_down)
		g_key_file_set_value(kf, "Hotkeys", "down", m_hotkey_down);
	if(m_hotkey_mute)
		g_key_file_set_value(kf, "Hotkeys", "mute", m_hotkey_mute);
    g_key_file_set_boolean(kf, "Notification", "show_notification",
                           m_show_notification);
    g_key_file_set_integer(kf, "Notification", "notification_type",
                           m_notification_type);

	gsize length;
	gchar * data = g_key_file_to_data(kf, &length, NULL);
	g_file_set_contents(m_config_file, data, -1, NULL);
	g_free(data);
	g_key_file_free(kf);
}

void config_initialize(gchar * config_name)
{
	// Build config directory name
	gchar * config_dir = g_build_filename(g_get_user_config_dir(),
		CONFIG_DIRNAME, NULL);
	m_config_file = g_build_filename(config_dir,
		config_name ? config_name : CONFIG_FILENAME, NULL);

	// Make sure config directory exists
	if(!g_file_test(config_dir, G_FILE_TEST_IS_DIR))
		g_mkdir(config_dir, 0777);

	// If a config file doesn't exist, create one with defaults otherwise
	// read the existing one.
	if(!g_file_test(m_config_file, G_FILE_TEST_EXISTS))
	{
		config_load_default();
		config_write();
	}
	else
	{
		config_read();
	}

	g_free(config_dir);
}

