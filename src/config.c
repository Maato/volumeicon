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
static struct config {
    gchar *path;

    // Alsa
    gchar *card; // TODO: Rename this to device.
    gchar *channel;

    // Notifications
    gboolean show_notification;
    gint notification_type;

    // Status icon
    int stepsize; // TODO: Rename this to volume_stepsize.
    gchar *helper_program;
    gchar *theme;
    gboolean use_panel_specific_icons;

    // Left mouse button action
    gboolean lmb_slider;

    // Middle mouse button action
    gboolean mmb_mute;

    // Layout
    gboolean use_horizontal_slider;
    gboolean show_sound_level;
    gboolean use_transparent_background;

    // Hotkeys
    gboolean hotkey_up_enabled;
    gboolean hotkey_down_enabled;
    gboolean hotkey_mute_enabled;
    gchar *hotkey_up;
    gchar *hotkey_down;
    gchar *hotkey_mute;
} m_config = {
    .path = NULL,

    // Alsa
    .card = NULL,
    .channel = NULL,

    // Notifications
    .show_notification = TRUE,
    .notification_type = 0,

    // Status icon
    .stepsize = 0,
    .helper_program = NULL,
    .theme = NULL,
    .use_panel_specific_icons = FALSE,

    // Left mouse button action
    .lmb_slider = FALSE,

    // Middle mouse button action
    .mmb_mute = FALSE,

    // Layout
    .use_horizontal_slider = FALSE,
    .show_sound_level = FALSE,
    .use_transparent_background = FALSE,

    // Hotkeys
    .hotkey_up_enabled = FALSE,
    .hotkey_down_enabled = FALSE,
    .hotkey_mute_enabled = FALSE,
    .hotkey_up = NULL,
    .hotkey_down = NULL,
    .hotkey_mute = NULL
};

//##############################################################################
// Static functions
//##############################################################################
static void config_load_default(void)
{
    if(!m_config.helper_program)
        config_set_helper(DEFAULT_MIXERAPP);
    if(!m_config.channel)
        config_set_channel(NULL);
    if(!m_config.card)
        config_set_card("default");
    if(!m_config.stepsize)
        config_set_stepsize(5);
    if(!m_config.theme)
        config_set_theme("Default");
    if(!m_config.hotkey_up)
        config_set_hotkey_up("XF86AudioRaiseVolume");
    if(!m_config.hotkey_down)
        config_set_hotkey_down("XF86AudioLowerVolume");
    if(!m_config.hotkey_mute)
        config_set_hotkey_mute("XF86AudioMute");
}

static void config_read(void)
{
    // Clean up previously loaded configuration values
    m_config.stepsize = 0;
    g_free(m_config.helper_program);
    g_free(m_config.channel);
    g_free(m_config.theme);

    // Load keys from keyfile
    GKeyFile *kf = g_key_file_new();
    g_key_file_load_from_file(kf, m_config.path, G_KEY_FILE_NONE, NULL);

#define GET_VALUE(type, section, key) \
    g_key_file_get_##type(kf, section, key, NULL)
#define GET_STRING(s, k) GET_VALUE(value, s, k)
#define GET_BOOL(s, k) GET_VALUE(boolean, s, k)
#define GET_INT(s, k) GET_VALUE(integer, s, k)

    // Alsa
    m_config.card = GET_STRING("Alsa", "card");
    m_config.channel = GET_STRING("Alsa", "channel");

    // Notifications
    m_config.show_notification = GET_BOOL("Notification", "show_notification");
    m_config.notification_type = GET_INT("Notification", "notification_type");

    // Status icon
    m_config.stepsize = GET_INT("StatusIcon", "stepsize");
    m_config.helper_program = GET_STRING("StatusIcon", "onclick");
    m_config.theme = GET_STRING("StatusIcon", "theme");
    m_config.use_panel_specific_icons = GET_BOOL(
        "StatusIcon", "use_panel_specific_icons");

    // Left mouse button action
    m_config.lmb_slider = GET_BOOL("StatusIcon", "lmb_slider");

    // Middle mouse button action
    m_config.mmb_mute = GET_BOOL("StatusIcon", "mmb_mute");

    // Layout
    m_config.use_horizontal_slider = GET_BOOL(
        "StatusIcon", "use_horizontal_slider");
    m_config.show_sound_level = GET_BOOL("StatusIcon", "show_sound_level");
    m_config.use_transparent_background = GET_BOOL(
        "StatusIcon", "use_transparent_background");

    // Hotkeys
    m_config.hotkey_up_enabled = GET_BOOL("Hotkeys", "up_enabled");
    m_config.hotkey_down_enabled = GET_BOOL("Hotkeys", "down_enabled");
    m_config.hotkey_mute_enabled = GET_BOOL("Hotkeys", "mute_enabled");
    m_config.hotkey_up = GET_STRING("Hotkeys", "up");
    m_config.hotkey_down = GET_STRING("Hotkeys", "down");
    m_config.hotkey_mute = GET_STRING("Hotkeys", "mute");

    g_key_file_free(kf);

#undef GET_VALUE
#undef GET_STRING
#undef GET_BOOL
#undef GET_INT

    // Load default values for unset keys
    config_load_default();
}

//##############################################################################
// Exported setter functions
//##############################################################################

// Alsa
void config_set_card(const gchar *card)
{
    g_free(m_config.card);
    m_config.card = g_strdup(card);
}

void config_set_channel(const gchar *channel)
{
    g_free(m_config.channel);
    m_config.channel = g_strdup(channel);
}

// Notifications
void config_set_show_notification(gboolean active)
{
    m_config.show_notification = active;
}

void config_set_notification_type(gint type)
{
    m_config.notification_type = type;
}

// Status icon
void config_set_stepsize(int stepsize)
{
    m_config.stepsize = stepsize;
}

void config_set_helper(const gchar *helper)
{
    g_free(m_config.helper_program);
    m_config.helper_program = g_strdup(helper);
}

void config_set_theme(const gchar *theme)
{
    g_free(m_config.theme);
    m_config.theme = g_strdup(theme);
}

void config_set_use_panel_specific_icons(gboolean active)
{
    m_config.use_panel_specific_icons = active;
}

// Left mouse button action
void config_set_left_mouse_slider(gboolean active)
{
    m_config.lmb_slider = active;
}

// Middle mouse button action
void config_set_middle_mouse_mute(gboolean active)
{
    m_config.mmb_mute = active;
}

// Layout
void config_set_use_horizontal_slider(gboolean active)
{
    m_config.use_horizontal_slider = active;
}

void config_set_show_sound_level(gboolean active)
{
    m_config.show_sound_level = active;
}

void config_set_use_transparent_background(gboolean active)
{
    m_config.use_transparent_background = active;
}

// Hotkey
void config_set_hotkey_up_enabled(gboolean enabled)
{
    m_config.hotkey_up_enabled = enabled;
}

void config_set_hotkey_down_enabled(gboolean enabled)
{
    m_config.hotkey_down_enabled = enabled;
}

void config_set_hotkey_mute_enabled(gboolean enabled)
{
    m_config.hotkey_mute_enabled = enabled;
}

void config_set_hotkey_up(const gchar *up)
{
    g_free(m_config.hotkey_up);
    m_config.hotkey_up = g_strdup(up);
}

void config_set_hotkey_down(const gchar *down)
{
    g_free(m_config.hotkey_down);
    m_config.hotkey_down = g_strdup(down);
}

void config_set_hotkey_mute(const gchar *mute)
{
    g_free(m_config.hotkey_mute);
    m_config.hotkey_mute = g_strdup(mute);
}

//##############################################################################
// Exported getter functions
//##############################################################################

// Alsa
const gchar *config_get_card(void)
{
    return m_config.card;
}

const gchar *config_get_channel(void)
{
    return m_config.channel;
}

// Notifications
gboolean config_get_show_notification(void)
{
    return m_config.show_notification;
}

gint config_get_notification_type(void)
{
    return m_config.notification_type;
}

// Status icon
int config_get_stepsize(void)
{
    return m_config.stepsize;
}

const gchar *config_get_helper(void)
{
    return m_config.helper_program;
}

const gchar *config_get_theme(void)
{
    return m_config.theme;
}

gboolean config_get_use_gtk_theme(void)
{
    return g_strcmp0(m_config.theme, "Default") == 0 ? TRUE : FALSE;
}

gboolean config_get_use_panel_specific_icons(void)
{
    return m_config.use_panel_specific_icons;
}

// Left mouse button action
gboolean config_get_left_mouse_slider(void)
{
    return m_config.lmb_slider;
}

// Middle mouse button action
gboolean config_get_middle_mouse_mute(void)
{
    return m_config.mmb_mute;
}

// Layout
gboolean config_get_use_horizontal_slider(void)
{
    return m_config.use_horizontal_slider;
}

gboolean config_get_show_sound_level(void)
{
    return m_config.show_sound_level;
}

gboolean config_get_use_transparent_background(void)
{
    return m_config.use_transparent_background;
}

// Hotkeys
gboolean config_get_hotkey_up_enabled(void)
{
    return m_config.hotkey_up_enabled;
}

gboolean config_get_hotkey_down_enabled(void)
{
    return m_config.hotkey_down_enabled;
}

gboolean config_get_hotkey_mute_enabled(void)
{
    return m_config.hotkey_mute_enabled;
}

const gchar *config_get_hotkey_up(void)
{
    return m_config.hotkey_up;
}

const gchar *config_get_hotkey_down(void)
{
    return m_config.hotkey_down;
}

const gchar *config_get_hotkey_mute(void)
{
    return m_config.hotkey_mute;
}

//##############################################################################
// Exported miscellaneous functions
//##############################################################################
void config_write(void)
{
    assert(m_config.path);

    GKeyFile *kf = g_key_file_new();

#define SET_VALUE(type, section, key, value) \
    g_key_file_set_##type(kf, section, key, value)
#define SET_STRING(s, k, v) SET_VALUE(value, s, k, v)
#define SET_BOOL(s, k, v) SET_VALUE(boolean, s, k, v)
#define SET_INT(s, k, v) SET_VALUE(integer, s, k, v)

    // Alsa
    if(m_config.card)
        SET_STRING("Alsa", "card", m_config.card);
    if(m_config.channel)
        SET_STRING("Alsa", "channel", m_config.channel);

    // Notifications
    SET_BOOL("Notification", "show_notification", m_config.show_notification);
    SET_INT("Notification", "notification_type", m_config.notification_type);

    // Status icon
    SET_INT("StatusIcon", "stepsize", m_config.stepsize);
    if(m_config.helper_program)
        SET_STRING("StatusIcon", "onclick", m_config.helper_program);
    if(m_config.theme)
        SET_STRING("StatusIcon", "theme", m_config.theme);
    SET_BOOL("StatusIcon", "use_panel_specific_icons",
             m_config.use_panel_specific_icons);

    // Left mouse button action
    SET_BOOL("StatusIcon", "lmb_slider", m_config.lmb_slider);

    // Middle mouse button action
    SET_BOOL("StatusIcon", "mmb_mute", m_config.mmb_mute);

    // Layout
    SET_BOOL("StatusIcon", "use_horizontal_slider",
             m_config.use_horizontal_slider);
    SET_BOOL("StatusIcon", "show_sound_level", m_config.show_sound_level);
    SET_BOOL("StatusIcon", "use_transparent_background",
             m_config.use_transparent_background);

    // Hotkeys
    SET_BOOL("Hotkeys", "up_enabled", m_config.hotkey_up_enabled);
    SET_BOOL("Hotkeys", "down_enabled", m_config.hotkey_down_enabled);
    SET_BOOL("Hotkeys", "mute_enabled", m_config.hotkey_mute_enabled);
    if(m_config.hotkey_up)
        SET_STRING("Hotkeys", "up", m_config.hotkey_up);
    if(m_config.hotkey_down)
        SET_STRING("Hotkeys", "down", m_config.hotkey_down);
    if(m_config.hotkey_mute)
        SET_STRING("Hotkeys", "mute", m_config.hotkey_mute);

    gchar *data = g_key_file_to_data(kf, NULL, NULL);
    g_key_file_free(kf);
    g_file_set_contents(m_config.path, data, -1, NULL);
    g_free(data);

#undef SET_VALUE
#undef SET_STRING
#undef SET_BOOL
#undef SET_INT
}

void config_initialize(gchar *config_name)
{
    // Build config directory name
    gchar *config_dir = g_build_filename(g_get_user_config_dir(),
        CONFIG_DIRNAME, NULL);
    m_config.path = g_build_filename(
        config_dir, config_name ? config_name : CONFIG_FILENAME, NULL);

    // Make sure config directory exists
    if(!g_file_test(config_dir, G_FILE_TEST_IS_DIR))
        g_mkdir(config_dir, 0777);

    // If a config file doesn't exist, create one with defaults otherwise
    // read the existing one.
    if(!g_file_test(m_config.path, G_FILE_TEST_EXISTS))
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

