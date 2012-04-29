//##############################################################################
// volumeicon
//
// volumeicon.c - implements the gtk status icon and preferences window
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

#include <stdlib.h>
#include <assert.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>
#include <signal.h>
#include <unistd.h>

#ifdef COMPILEWITH_NOTIFY
	#ifndef NOTIFY_CHECK_VERSION
	#define NOTIFY_CHECK_VERSION(a,b,c) 0
	#endif
	#include <libnotify/notify.h>
#endif
#ifdef COMPILEWITH_OSS
	#include "oss_backend.h"
#else
	#include "alsa_backend.h"
#endif
#include "keybinder.h"
#include "config.h"

enum HOTKEY
{
	UP,
	DOWN,
	MUTE
};

//##############################################################################
// Definitions
//##############################################################################
// Resources
#ifndef DATADIR
#define DATADIR "../data"
#endif
#define PREFERENCES_UI_FILE   DATADIR "/gui/preferences.ui"
#define ICONS_DIR             DATADIR "/icons"
#define APP_ICON              DATADIR "/gui/appicon.svg"

// About
#define APPNAME "Volume Icon"
#define COPYRIGHT "Copyright (c) Maato 2011"
#define COMMENTS "Volume control for your system tray."
#define WEBSITE "http://softwarebakery.com/maato/volumeicon.html"

// Scale constants
#define SCALE_HIDE_DELAY 500
#define TIMER_INTERVAL 50

//##############################################################################
// Static variables
//##############################################################################
#ifdef COMPILEWITH_NOTIFY
static NotifyNotification * m_notification = NULL;
#endif
static GtkStatusIcon * m_status_icon = NULL;
static GtkWidget * m_scale_window = NULL;
static GtkWidget * m_scale = NULL;
static gboolean m_setting_scale_value = FALSE;

// Backend Interface
static void (*backend_setup)(const gchar * card, const gchar * channel,
	void (*volume_changed)(int,gboolean)) = NULL;
static void (*backend_set_channel)(const gchar * channel) = NULL;
static void (*backend_set_volume)(int volume) = NULL;
static void (*backend_set_mute)(gboolean mute) = NULL;
static int (*backend_get_volume)(void) = NULL;
static gboolean (*backend_get_mute)(void) = NULL;
static const gchar * (*backend_get_channel)(void) = NULL;
static const GList * (*backend_get_channel_names)(void) = NULL;

// Status
static int m_volume = 0;
static gboolean m_mute = FALSE;

// Icons
#define ICON_COUNT 8
static GdkPixbuf * m_icons[ICON_COUNT];

//##############################################################################
// Function prototypes
//##############################################################################
static void status_icon_update(gboolean mute, gboolean force);
static void hotkey_handle(const char * key, void * user_data);
static void volume_icon_load_icons();
static void scale_update();
static void notification_show();

//##############################################################################
// Type definitions
//##############################################################################
typedef struct
{
	GtkBuilder * builder;
	GtkWidget * window;
	GtkAdjustment * volume_adjustment;
	GtkEntry * mixer_entry;
	GtkComboBox * channel_combobox;
	GtkListStore * channel_store;
	GtkComboBox * theme_combobox;
	GtkListStore * theme_store;
	GtkListStore * hotkey_store;
	GtkButton * close_button;
	GtkRadioButton * mute_radiobutton;
	GtkRadioButton * slider_radiobutton;
	GtkRadioButton * mmb_mute_radiobutton;
	GtkRadioButton * mmb_mixer_radiobutton;
	GtkCheckButton * use_horizontal_slider_checkbutton;
	GtkCheckButton * show_sound_level_checkbutton;
	GtkCellRenderer * cra_hotkey;
	GtkCellRendererToggle * crt_hotkey;
} PreferencesGui;

//##############################################################################
// Static functions
//##############################################################################
// Preferences handlers
static gboolean preferences_window_delete_event(GtkWidget * widget,
	GdkEvent * event, gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	gtk_widget_destroy(gui->window);
	return FALSE;
}

static void preferences_window_destroy(GObject * object, gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	free(gui);
	config_write();
}

static void preferences_close_button_clicked(GtkWidget * widget,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	gtk_widget_destroy(gui->window);
}

static void preferences_mute_radiobutton_toggled(GtkToggleButton * togglebutton,
	gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(togglebutton);
	config_set_left_mouse_slider(!active);
}

static void preferences_mmb_radiobutton_toggled(GtkToggleButton * togglebutton,
	gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(togglebutton);
	config_set_middle_mouse_mute(!active);
}

static void scale_setup();

static void preferences_use_horizontal_slider_checkbutton_toggled(GtkCheckButton * widget,
	gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	config_set_use_horizontal_slider(active);
	gtk_widget_destroy(m_scale);
	gtk_widget_destroy(m_scale_window);
	scale_setup();
}

static void preferences_show_sound_level_checkbutton_toggled(GtkCheckButton * widget,
	gpointer user_data)
{
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
	config_set_show_sound_level(active);
	gtk_scale_set_draw_value(GTK_SCALE(m_scale), active);
}

static void preferences_theme_combobox_changed(GtkComboBox * widget,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	GtkTreeIter iter;

	// Get the theme from the combobox
	if(gtk_combo_box_get_active_iter(gui->theme_combobox, &iter))
	{
		gchar * theme;
		gtk_tree_model_get(GTK_TREE_MODEL(gui->theme_store), &iter, 0,
			&theme, -1);
		config_set_theme(theme);
		g_free(theme);
	}
	volume_icon_load_icons();
	status_icon_update(m_mute, TRUE);
}

static void preferences_channel_combobox_changed(GtkComboBox * widget,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	GtkTreeIter iter;

	// Get the channel from the combobox
	if(gtk_combo_box_get_active_iter(gui->channel_combobox, &iter))
	{
		gchar * channel;
		gtk_tree_model_get(GTK_TREE_MODEL(gui->channel_store), &iter, 0,
			&channel, -1);
		backend_set_channel(channel);
		config_set_channel(channel);
		g_free(channel);
		m_volume = backend_get_volume();
		m_mute = backend_get_mute();
	}
	status_icon_update(m_mute, TRUE);
	scale_update();
}

static void preferences_volume_adjustment_changed(GtkSpinButton * spinbutton,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	config_set_stepsize((int)gtk_adjustment_get_value(gui->volume_adjustment));
}

static void preferences_mixer_entry_changed(GtkEditable * editable,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	config_set_helper(gtk_entry_get_text(gui->mixer_entry));
}

static void preferences_crt_toggled(GtkCellRendererToggle * cell_renderer,
	gchar * path, gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	GtkTreeIter iter;

	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gui->hotkey_store), &iter, path))
	{
		gboolean enabled = FALSE;
		gchar * accel_name = NULL;
		enum HOTKEY hotkey = 0;
		gtk_tree_model_get(GTK_TREE_MODEL(gui->hotkey_store), &iter, 1, &accel_name, 2, &hotkey, 3, &enabled, -1);
		enabled = !enabled;

		if(!enabled)
			keybinder_unbind(accel_name, hotkey_handle);

		if(enabled && !keybinder_bind(accel_name, hotkey_handle, (void*)hotkey))
		{
			g_fprintf(stderr, "Failed to bind %s\n", accel_name);
		}
		else
		{
			gtk_list_store_set(GTK_LIST_STORE(gui->hotkey_store), &iter, 3, enabled, -1);
			switch(hotkey)
			{
				case UP: config_set_hotkey_up_enabled(enabled); break;
				case DOWN: config_set_hotkey_down_enabled(enabled); break;
				case MUTE: config_set_hotkey_mute_enabled(enabled); break;
			}
		}

		g_free(accel_name);
	}
}

static void preferences_cra_accel_edited(GtkCellRendererAccel * renderer,
	gchar * path, guint accel_key, GdkModifierType mask, guint hardware_keycode,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)user_data;
	GtkTreeIter iter;
	
	if(gtk_tree_model_get_iter_from_string(GTK_TREE_MODEL(gui->hotkey_store), &iter, path))
	{
		enum HOTKEY hotkey = 0;
		gboolean enabled = FALSE;
		gchar * old_value = NULL; 
		gchar * new_value = gtk_accelerator_name(accel_key, mask);
		gtk_tree_model_get(GTK_TREE_MODEL(gui->hotkey_store), &iter, 1, &old_value, 2, &hotkey, 3, &enabled, -1);

		if(enabled && !keybinder_bind(new_value, hotkey_handle, (void*)hotkey))
		{
			g_fprintf(stderr, "Failed to bind %s\n", new_value);
		}
		else
		{
			gtk_list_store_set(GTK_LIST_STORE(gui->hotkey_store), &iter, 1, new_value, -1);
			keybinder_unbind(old_value, hotkey_handle);
			switch(hotkey)
			{
				case UP: config_set_hotkey_up(new_value); break;
				case DOWN: config_set_hotkey_down(new_value); break;
				case MUTE: config_set_hotkey_mute(new_value); break;
			}
		}
		g_free(new_value);
		g_free(old_value);
	}
}

// Menu handlers
static void menu_preferences_on_activate(GtkMenuItem * menuitem,
	gpointer user_data)
{
	PreferencesGui * gui = (PreferencesGui*)malloc(sizeof(PreferencesGui));

	gui->builder = gtk_builder_new();
	gtk_builder_add_from_file(gui->builder, PREFERENCES_UI_FILE, NULL);

	// Get widgets from builder
	#define getobj(x) gtk_builder_get_object(gui->builder,x)
	gui->window = GTK_WIDGET(getobj("window1"));
	gui->volume_adjustment = GTK_ADJUSTMENT(getobj("volume_adjustment"));
	gui->mixer_entry = GTK_ENTRY(getobj("mixer_entry"));
	gui->channel_combobox = GTK_COMBO_BOX(getobj("channel_combobox"));
	gui->channel_store = GTK_LIST_STORE(getobj("channel_name_model"));
	gui->theme_combobox = GTK_COMBO_BOX(getobj("theme_combobox"));
	gui->theme_store = GTK_LIST_STORE(getobj("theme_name_model"));
	gui->hotkey_store = GTK_LIST_STORE(getobj("hotkey_binding_model"));
	gui->close_button = GTK_BUTTON(getobj("close_button"));
	gui->mute_radiobutton = GTK_RADIO_BUTTON(getobj("mute_radiobutton"));
	gui->slider_radiobutton = GTK_RADIO_BUTTON(getobj("slider_radiobutton"));
	gui->mmb_mute_radiobutton = GTK_RADIO_BUTTON(getobj("mmb_mute_radiobutton"));
	gui->mmb_mixer_radiobutton = GTK_RADIO_BUTTON(getobj("mmb_mixer_radiobutton"));
	gui->use_horizontal_slider_checkbutton = GTK_CHECK_BUTTON(getobj("use_horizontal_slider"));
	gui->show_sound_level_checkbutton = GTK_CHECK_BUTTON(getobj("show_sound_level"));
	gui->cra_hotkey = GTK_CELL_RENDERER(getobj("cra_hotkey"));
	gui->crt_hotkey = GTK_CELL_RENDERER_TOGGLE(getobj("crt_hotkey"));
	#undef getobj

	// Set the window icon
	gtk_window_set_default_icon_from_file(APP_ICON, NULL);

	// Set the radiobuttons
	if(config_get_left_mouse_slider())
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->slider_radiobutton),
			TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->mute_radiobutton),
			TRUE);
	}

	if(config_get_middle_mouse_mute())
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->mmb_mute_radiobutton),
			TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->mmb_mixer_radiobutton),
			TRUE);
	}

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->use_horizontal_slider_checkbutton),
		config_get_use_horizontal_slider());

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(gui->show_sound_level_checkbutton),
		config_get_show_sound_level());

	// Fill the channel model and combobox
	GtkTreeIter tree_iter;
	const GList * list_iter = backend_get_channel_names();
	while(list_iter)
	{
		gtk_list_store_append(gui->channel_store, &tree_iter);
		gtk_list_store_set(gui->channel_store, &tree_iter, 0,
			(gchar*)list_iter->data, -1);
		if(g_strcmp0((gchar*)list_iter->data, backend_get_channel()) == 0)
			gtk_combo_box_set_active_iter(gui->channel_combobox, &tree_iter);
		list_iter = g_list_next(list_iter);
	}

	// Fill the theme name model and combobox
	gtk_list_store_append(gui->theme_store, &tree_iter);
	gtk_list_store_set(gui->theme_store, &tree_iter, 0, "Default", -1);
	if(config_get_use_gtk_theme())
		gtk_combo_box_set_active_iter(gui->theme_combobox, &tree_iter);
	GDir * themedir = g_dir_open(ICONS_DIR, 0, NULL);
	const gchar * name;
	while((name = g_dir_read_name(themedir)))
	{
		gtk_list_store_append(gui->theme_store, &tree_iter);
		gtk_list_store_set(gui->theme_store, &tree_iter, 0, name, -1);
		if(g_strcmp0(name, config_get_theme()) == 0)
			gtk_combo_box_set_active_iter(gui->theme_combobox, &tree_iter);
	}
	g_dir_close(themedir);

	// Fill the hotkey binding model
	gtk_list_store_append(gui->hotkey_store, &tree_iter);
	gtk_list_store_set(gui->hotkey_store, &tree_iter, 0, "Volume Up", 1,
		config_get_hotkey_up(), 2, (int)UP, 3, config_get_hotkey_up_enabled(), -1);
	gtk_list_store_append(gui->hotkey_store, &tree_iter);
	gtk_list_store_set(gui->hotkey_store, &tree_iter, 0, "Volume Down", 1,
		config_get_hotkey_down(), 2, (int)DOWN, 3, config_get_hotkey_down_enabled(), -1);
	gtk_list_store_append(gui->hotkey_store, &tree_iter);
	gtk_list_store_set(gui->hotkey_store, &tree_iter, 0, "Mute", 1,
		config_get_hotkey_mute(), 2, (int)MUTE, 3, config_get_hotkey_mute_enabled(), -1);

	// Initialize widgets / connect signals
	gtk_entry_set_text(gui->mixer_entry, config_get_helper());
	gtk_adjustment_set_value(gui->volume_adjustment,
		(gdouble)config_get_stepsize());
	g_signal_connect(G_OBJECT(gui->window), "destroy", G_CALLBACK(
		preferences_window_destroy), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->window), "delete-event", G_CALLBACK(
		preferences_window_delete_event), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->close_button), "clicked", G_CALLBACK(
		preferences_close_button_clicked), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->channel_combobox), "changed", G_CALLBACK(
		preferences_channel_combobox_changed), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->theme_combobox), "changed", G_CALLBACK(
		preferences_theme_combobox_changed), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->volume_adjustment), "value-changed",
		G_CALLBACK(preferences_volume_adjustment_changed), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->mixer_entry), "changed", G_CALLBACK(
		preferences_mixer_entry_changed), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->mute_radiobutton), "toggled", G_CALLBACK(
		preferences_mute_radiobutton_toggled), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->mmb_mixer_radiobutton), "toggled", G_CALLBACK(
		preferences_mmb_radiobutton_toggled), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->use_horizontal_slider_checkbutton), "toggled", G_CALLBACK(
		preferences_use_horizontal_slider_checkbutton_toggled), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->show_sound_level_checkbutton), "toggled", G_CALLBACK(
		preferences_show_sound_level_checkbutton_toggled), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->cra_hotkey), "accel-edited", G_CALLBACK(
		preferences_cra_accel_edited), (gpointer)gui);
	g_signal_connect(G_OBJECT(gui->crt_hotkey), "toggled", G_CALLBACK(
		preferences_crt_toggled), (gpointer)gui);

	gtk_widget_show_all(gui->window);
}

static void menu_quit_on_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	gtk_main_quit();
}

static void menu_about_on_activate(GtkMenuItem * menuitem, gpointer user_data)
{
	GtkWidget * aboutDialog = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name(GTK_ABOUT_DIALOG(aboutDialog), APPNAME);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(aboutDialog), VERSION);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(aboutDialog),
		gdk_pixbuf_new_from_file(APP_ICON, NULL));
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(aboutDialog), COPYRIGHT);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(aboutDialog), COMMENTS);
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(aboutDialog),	WEBSITE);
	gtk_dialog_run(GTK_DIALOG(aboutDialog));
	gtk_widget_destroy(aboutDialog);
}

static void volume_icon_launch_helper()
{
	assert(config_get_helper() != NULL);
	pid_t pid = fork();
	if(pid == 0)
	{
		execl("/bin/sh", "/bin/sh", "-c", config_get_helper(), (char*)NULL);
		_exit(0);
	}
}

static void menu_volcontrol_on_activate(GtkMenuItem * menuitem,
	gpointer user_data)
{
	volume_icon_launch_helper();
}

static gboolean scale_point_in_rect(GdkRectangle * rect, gint x, gint y)
{
	return (x >= rect->x &&
		x <= rect->x + rect->width &&
		y >= rect->y &&
		y <= rect->y + rect->height);
}

static gboolean scale_timeout(gpointer data)
{
	static int counter = SCALE_HIDE_DELAY;
	GdkRectangle window;
	GdkRectangle icon;

	gtk_window_get_position(GTK_WINDOW(m_scale_window), &window.x, &window.y);
	gtk_window_get_size(GTK_WINDOW(m_scale_window), &window.width,
		&window.height);
	gtk_status_icon_get_geometry(m_status_icon, NULL, &icon, NULL);

	gint x, y;
	gdk_window_get_pointer(gtk_widget_get_root_window(m_scale_window),
		&x, &y, NULL);

	if(scale_point_in_rect(&window, x, y) || scale_point_in_rect(&icon, x, y))
	{
		counter = SCALE_HIDE_DELAY;
		return TRUE;
	}
	else if(counter > 0)
	{
		counter -= TIMER_INTERVAL;
		return TRUE;
	}
	gtk_widget_hide(m_scale_window);
	return FALSE;
}

// StatusIcon handlers
static void status_icon_on_button_release(GtkStatusIcon * status_icon,
	GdkEventButton * event, gpointer user_data)
{
	if(event->button == 1 && config_get_left_mouse_slider() &&
		!gtk_widget_get_visible(m_scale_window))
	{
		gint sizex;
		gint sizey;
		gtk_window_get_size(GTK_WINDOW(m_scale_window), &sizex, &sizey);

		gint x = (gint)event->x_root - sizex / 2;
		gint y = (gint)event->y_root - sizey;

		if(gtk_status_icon_is_embedded(m_status_icon))
		{
			GdkRectangle area;
			gtk_status_icon_get_geometry(m_status_icon, NULL, &area, NULL);
			if(config_get_use_horizontal_slider()) {
				y = area.y + area.height / 2 - sizey / 2;
				if(area.x > sizex) // popup left
					x = area.x - sizex;
				else // popup right
					x = area.x + area.width;
			} else {
				x = area.x + area.width / 2 - sizex / 2;
				if(area.y > sizey) // popup up
					y = area.y - sizey;
				else // popup down
					y = area.y + area.height;
			}
		}

		gtk_window_move(GTK_WINDOW(m_scale_window), x, y);
		gtk_window_present_with_time(GTK_WINDOW(m_scale_window), event->time);
		g_timeout_add(TIMER_INTERVAL, scale_timeout, NULL);
	}
	else if(event->button == 1 ||
		(event->button == 2 && config_get_middle_mouse_mute()))
	{
		m_mute = !m_mute;
		backend_set_volume(m_volume);
		backend_set_mute(m_mute);
		status_icon_update(m_mute, FALSE);
	}
	else if(event->button == 2)
	{
		volume_icon_launch_helper();
	}
}

static void status_icon_on_scroll_event(GtkStatusIcon * status_icon,
	GdkEventScroll * event, gpointer user_data)
{
	switch(event->direction)
	{
		case(GDK_SCROLL_UP):
		case(GDK_SCROLL_RIGHT):
			m_volume += config_get_stepsize();
			break;
		case(GDK_SCROLL_DOWN):
		case(GDK_SCROLL_LEFT):
			m_volume -= config_get_stepsize();
			break;
	}
	if(m_volume < 0)
		m_volume = 0;
	if(m_volume > 100)
		m_volume = 100;
	backend_set_volume(m_volume);
	if(m_mute)
	{
		m_mute = FALSE;
		backend_set_mute(m_mute);
	}
	status_icon_update(m_mute, FALSE);
	scale_update();
	notification_show();
}

static void status_icon_on_popup_menu(GtkStatusIcon * status_icon, guint button,
	guint activation_time, gpointer user_data)
{
	GtkWidget * gtkMenu = gtk_menu_new();

	GtkWidget * volcontrol = gtk_image_menu_item_new_with_label("Open Mixer");
	GtkWidget * separator1 = gtk_separator_menu_item_new();
	GtkWidget * preferences = gtk_image_menu_item_new_from_stock(
		"gtk-preferences", NULL);
	GtkWidget * about = gtk_image_menu_item_new_from_stock("gtk-about", NULL);
	GtkWidget * separator2 = gtk_separator_menu_item_new();
	GtkWidget * quit = gtk_image_menu_item_new_from_stock("gtk-quit", NULL);
	g_signal_connect(G_OBJECT(volcontrol), "activate",
		G_CALLBACK(menu_volcontrol_on_activate), NULL);
	g_signal_connect(G_OBJECT(preferences), "activate",
		G_CALLBACK(menu_preferences_on_activate), NULL);
	g_signal_connect(G_OBJECT(quit), "activate",
		G_CALLBACK(menu_quit_on_activate), NULL);
	g_signal_connect(G_OBJECT(about), "activate",
		G_CALLBACK(menu_about_on_activate), NULL);

	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), volcontrol);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), separator1);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), preferences);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), about);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), separator2);
	gtk_menu_shell_append(GTK_MENU_SHELL(gtkMenu), quit);

	gtk_widget_show_all(gtkMenu);

	gtk_menu_popup(GTK_MENU(gtkMenu), NULL, NULL,
		gtk_status_icon_position_menu, status_icon,
		button, activation_time);
}

static int status_icon_get_number(int volume, gboolean mute)
{
	if(mute) return 1;
	if(volume <= 0) return 1;
	if(volume <= 16) return 2;
	if(volume <= 33) return 3;
	if(volume <= 50) return 4;
	if(volume <= 67) return 5;
	if(volume <= 84) return 6;
	if(volume <= 99) return 7;
	return 8;
}

// Use the ignore_cache parameter to force the status icon to be loaded
// from file, for example after a theme change.
static void status_icon_update(gboolean mute, gboolean ignore_cache)
{
	static int volume_cache = -1;
	static int icon_cache = -1;
	int volume = backend_get_volume();

	int icon_number = status_icon_get_number(volume, mute);
	if(icon_number != icon_cache || ignore_cache)
	{
		if(config_get_use_gtk_theme())
		{
			if(icon_number == 1)
				gtk_status_icon_set_from_icon_name(m_status_icon,
					"audio-volume-muted");
			else if(icon_number <= 3)
				gtk_status_icon_set_from_icon_name(m_status_icon,
					"audio-volume-low");
			else if(icon_number <= 6)
				gtk_status_icon_set_from_icon_name(m_status_icon,
					"audio-volume-medium");
			else
				gtk_status_icon_set_from_icon_name(m_status_icon,
					"audio-volume-high");
		}
		else
		{
			gtk_status_icon_set_from_pixbuf(m_status_icon, m_icons[icon_number-1]);
		}

		#ifdef COMPILEWITH_NOTIFY
		if(icon_number == 1)
			notify_notification_update(m_notification, APPNAME, NULL,
				"audio-volume-muted");
		else if(icon_number <= 3)
			notify_notification_update(m_notification, APPNAME, NULL,
				"audio-volume-low");
		else if(icon_number <= 6)
			notify_notification_update(m_notification, APPNAME, NULL,
				"audio-volume-medium");
		else
			notify_notification_update(m_notification, APPNAME, NULL,
				"audio-volume-high");
		#endif

		icon_cache = icon_number;
	}

	if((volume != volume_cache || ignore_cache) && backend_get_channel())
	{
		gchar buffer[32];
		g_sprintf(buffer, "%s: %d%%", backend_get_channel(), volume);
		gtk_status_icon_set_tooltip_text(m_status_icon, buffer);

		#ifdef COMPILEWITH_NOTIFY
		notify_notification_set_hint_int32(m_notification, "value",
			(gint)volume);
		#endif

		volume_cache = volume;
	}
}

static void status_icon_setup(gboolean mute)
{
	m_status_icon = gtk_status_icon_new();
	g_signal_connect(G_OBJECT(m_status_icon), "button_release_event",
		G_CALLBACK(status_icon_on_button_release), NULL);
	g_signal_connect(G_OBJECT(m_status_icon), "scroll_event",
		G_CALLBACK(status_icon_on_scroll_event), NULL);
	g_signal_connect(G_OBJECT(m_status_icon), "popup-menu",
		G_CALLBACK(status_icon_on_popup_menu), NULL);
	status_icon_update(mute, FALSE);
	gtk_status_icon_set_visible(m_status_icon, TRUE);
}

static void volume_icon_on_volume_changed(int volume, gboolean mute)
{
	m_mute = mute;
	m_volume = volume;
	status_icon_update(m_mute, FALSE);
	scale_update();
}

static void volume_icon_load_icons()
{
	if(config_get_use_gtk_theme())
		return;
	static int icons_loaded = 0;
	int i;
	for(i = 0; i < ICON_COUNT; i++)
	{
		gchar * icon_path = g_strdup_printf(ICONS_DIR "/%s/%d.png",
			config_get_theme(), i+1);
		if(icons_loaded && (NULL != m_icons[i]))
			g_object_unref(m_icons[i]);
		m_icons[i] = gdk_pixbuf_new_from_file(icon_path, NULL);
		if(NULL == m_icons[i])
			g_message("Failed to load '%s'", icon_path);
		g_free(icon_path);
	}
	icons_loaded = 1;
}

static void scale_update()
{
	assert(m_scale != NULL);
	m_setting_scale_value = TRUE;
	gtk_range_set_value(GTK_RANGE(m_scale), (double)m_volume);
	m_setting_scale_value = FALSE;
}

static void scale_value_changed(GtkRange * range, gpointer user_data)
{
	if(m_setting_scale_value)
		return;
	double value = gtk_range_get_value(range);
	m_volume = (int)value;

	if(m_volume < 0)
		m_volume = 0;
	if(m_volume > 100)
		m_volume = 100;
	backend_set_volume(m_volume);
	if(m_mute)
	{
		m_mute = FALSE;
		backend_set_mute(m_mute);
	}
	status_icon_update(m_mute, FALSE);
	scale_update();
}

static void notification_show()
{
	#ifdef COMPILEWITH_NOTIFY
	notify_notification_show(m_notification, NULL);
	#endif
}

static void scale_setup()
{
	if(config_get_use_horizontal_slider())
		m_scale = gtk_hscale_new_with_range(0.0, 100.0, 1.0);
	else
		m_scale =  gtk_vscale_new_with_range(0.0, 100.0, 1.0);
	gtk_range_set_inverted(GTK_RANGE(m_scale), TRUE);
	gtk_scale_set_draw_value(GTK_SCALE(m_scale), config_get_show_sound_level());

	m_scale_window = gtk_window_new(GTK_WINDOW_POPUP);
	gtk_window_set_decorated(GTK_WINDOW(m_scale_window), FALSE);
	gtk_window_set_skip_pager_hint(GTK_WINDOW(m_scale_window), TRUE);
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(m_scale_window), TRUE);
	gtk_window_set_keep_above(GTK_WINDOW(m_scale_window), TRUE);
	gtk_container_add(GTK_CONTAINER(m_scale_window), m_scale);

	if(config_get_use_horizontal_slider()) {
		gtk_window_set_default_size(GTK_WINDOW(m_scale_window), 120, 0);
		gtk_scale_set_value_pos(GTK_SCALE(m_scale), GTK_POS_RIGHT);
	} else {
		gtk_window_set_default_size(GTK_WINDOW(m_scale_window), 0, 120);
		gtk_scale_set_value_pos(GTK_SCALE(m_scale), GTK_POS_TOP);
	}

	gtk_widget_show(m_scale);
	scale_update();

	g_signal_connect(G_OBJECT(m_scale), "value-changed",
		G_CALLBACK(scale_value_changed), NULL);
}

static void hotkey_handle(const char * key, void * user_data)
{
	enum HOTKEY hotkey = (enum HOTKEY)user_data;
	if(hotkey == MUTE)
	{
		m_mute = !m_mute;
		backend_set_volume(m_volume);
		backend_set_mute(m_mute);
		status_icon_update(m_mute, FALSE);
	}
	else
	{
		int step = config_get_stepsize();
		m_volume += (hotkey == UP ? step : -step);
		m_volume = (m_volume > 100 ? 100 : (m_volume < 0 ? 0 : m_volume));
		backend_set_volume(m_volume);
		status_icon_update(m_mute, FALSE);
	}
	scale_update();
	notification_show();
}

//##############################################################################
// Exported functions
//##############################################################################
int main(int argc, char * argv[])
{
    bindtextdomain(GETTEXT_PACKAGE, LOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

	gtk_init(&argc, &argv);
	signal(SIGCHLD, SIG_IGN);

	// Setup OSD Notification
	#ifdef COMPILEWITH_NOTIFY
	if(notify_init(APPNAME))
	{
		#if NOTIFY_CHECK_VERSION(0,7,0)
		m_notification = notify_notification_new(APPNAME, NULL, NULL);
		#else
		m_notification = notify_notification_new(APPNAME, NULL, NULL, NULL);
		#endif
		notify_notification_set_timeout(m_notification, NOTIFY_EXPIRES_DEFAULT);
		notify_notification_set_hint_string(m_notification, "synchronous", "volume");
	}
	else
	{
		g_fprintf(stderr, "Failed to initialize notifications\n");
	}
	#endif

	// Setup backend
	#ifdef COMPILEWITH_OSS
	backend_setup = &oss_setup;
	backend_set_channel = &oss_set_channel;
	backend_set_volume = &oss_set_volume;
	backend_get_volume = &oss_get_volume;
	backend_set_mute = &oss_set_mute;
	backend_get_mute = &oss_get_mute;
	backend_get_channel = &oss_get_channel;
	backend_get_channel_names = &oss_get_channel_names;
	#else
	backend_setup = &asound_setup;
	backend_set_channel = &asound_set_channel;
	backend_set_volume = &asound_set_volume;
	backend_get_volume = &asound_get_volume;
	backend_get_mute = &asound_get_mute;
	backend_set_mute = &asound_set_mute;
	backend_get_channel = &asound_get_channel;
	backend_get_channel_names = &asound_get_channel_names;
	#endif

	// Setup
	config_initialize();
	backend_setup(config_get_card(), config_get_channel(), volume_icon_on_volume_changed);
	m_volume = backend_get_volume();
	m_mute = backend_get_mute();
	volume_icon_load_icons();
	status_icon_setup(m_mute);
	scale_setup();

	keybinder_init();
	if(config_get_hotkey_up_enabled() &&
		!keybinder_bind(config_get_hotkey_up(), hotkey_handle, (void*)UP))
			g_fprintf(stderr, "Failed to bind %s\n", config_get_hotkey_up());
	if(config_get_hotkey_down_enabled() &&
		!keybinder_bind(config_get_hotkey_down(), hotkey_handle, (void*)DOWN))
			g_fprintf(stderr, "Failed to bind %s\n", config_get_hotkey_down());
	if(config_get_hotkey_mute_enabled() &&
		!keybinder_bind(config_get_hotkey_mute(), hotkey_handle, (void*)MUTE))
			g_fprintf(stderr, "Failed to bind %s\n", config_get_hotkey_mute());

	// Main Loop
	gtk_main();

	#ifdef COMPILEWITH_NOTIFY
	g_object_unref(G_OBJECT(m_notification));
	notify_uninit();
	#endif
	
	return EXIT_SUCCESS;
}
