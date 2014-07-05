//##############################################################################
// volumeicon
//
// alsa_backend.c - implements a volume control abstraction using alsa-lib
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

#include <alsa/asoundlib.h>

#include <glib.h>
#include <gtk/gtk.h>
#include <math.h>

#include "alsa_backend.h"
#include "alsa_volume_mapping.h"

//##############################################################################
// Static variables
//##############################################################################
static snd_mixer_elem_t * m_elem = NULL;
static char * m_channel = NULL;
static char * m_device = NULL;
static snd_mixer_t * m_mixer = NULL;
static GList * m_channel_names = NULL;
static GList * m_device_names = NULL;
static void (*m_volume_changed)(int,gboolean);

//##############################################################################
// Static functions
//##############################################################################
static gboolean asound_channel_exists(const gchar * channel)
{
	GList * channel_name = m_channel_names;
	while(channel_name)
	{
		const gchar * name = (const gchar*)channel_name->data;
		if(g_strcmp0(channel, name) == 0)
			return TRUE;
		channel_name = channel_name->next;
	}
	return FALSE;
}

static int asound_elem_event(snd_mixer_elem_t * elem, unsigned int mask)
{
	assert(m_elem == elem);

	m_volume_changed(asound_get_volume(), asound_get_mute());
	return 0;
}

static gboolean asound_poll_cb(GIOChannel * source, GIOCondition condition,
	gpointer data)
{
	int retval = snd_mixer_handle_events(m_mixer);
	if(retval < 0) {
		fprintf(stderr, "snd_mixer_handle_events: %s\n", snd_strerror(retval));
		gtk_main_quit();
		return FALSE;
	}
	return TRUE;
}

//##############################################################################
// Exported functions
//##############################################################################
const gchar * asound_get_channel()
{
	return m_channel;
}

const gchar * asound_get_device()
{
	return m_device;
}

const GList * asound_get_channel_names()
{
	return m_channel_names;
}

const GList * asound_get_device_names()
{
	return m_device_names;
}

int asound_get_volume()
{
	if(m_elem == NULL) {
		return 0;
	}

	// Return the current volume value from [0-100]
	return rint(100 * get_normalized_playback_volume(m_elem, 0));
}

gboolean asound_get_mute()
{
	if(m_elem == NULL) {
		return TRUE;
	}

	gboolean mute = FALSE;
	if(snd_mixer_selem_has_playback_switch(m_elem))
	{
		int pswitch;
		snd_mixer_selem_get_playback_switch(m_elem, 0, &pswitch);
		mute = pswitch ? FALSE : TRUE;
	}
	return mute;
}

void asound_setup(const gchar * card, const gchar * channel,
	void (*volume_changed)(int,gboolean))
{
	gchar * card_override = NULL; // used to hold a string like hw:0 if a nice
	                              // device name was given as 'card'

	// Clean up resources from previous calls to setup
	g_free(m_channel);
	m_channel = NULL;
	if(m_elem) {
		snd_mixer_elem_set_callback(m_elem, NULL);
		m_elem = NULL;
	}
	if(m_mixer) {
		snd_mixer_close(m_mixer);
		m_mixer = NULL;
	}
	g_list_free_full(m_channel_names, g_free);
	m_channel_names = NULL;
	g_list_free_full(m_device_names, g_free);
	m_device_names = NULL;

	// Save card, volume_changed
	g_free(m_device);
	m_device = g_strdup(card);
	m_volume_changed = volume_changed;

	// Populate list of device names
	int card_number = -1;
	int ret = snd_card_next(&card_number);
	snd_ctl_card_info_t * info = NULL;
	snd_ctl_card_info_alloca(&info);
	m_device_names = g_list_append(m_device_names, (gpointer)g_strdup("default"));
	while(ret == 0 && card_number != -1) {
		char buf[16];
		sprintf(buf, "hw:%d", card_number);
		snd_ctl_t * ctl = NULL;
		if(snd_ctl_open(&ctl, buf, 0) < 0) {
			continue;
		}
		if(snd_ctl_card_info(ctl, info) < 0) {
			snd_ctl_close(ctl);
			continue;
		}
		snd_ctl_close(ctl);

		gchar * nice_name = g_strdup(snd_ctl_card_info_get_name(info));
		m_device_names = g_list_append(m_device_names, (gpointer)nice_name);

		if(g_strcmp0(nice_name, m_device) == 0) {
			g_free(card_override);
			card_override = g_strdup_printf("hw:%d", card_number);
		}
		ret = snd_card_next(&card_number);
	}

	// Load the mixer for the provided cardname
	snd_mixer_open(&m_mixer, 0);
	if(snd_mixer_attach(m_mixer, (card_override != NULL ? card_override : m_device)) < 0) {
		fprintf(stderr, "Failed to open sound device with name: %s\n",
			(card_override != NULL ? card_override : m_device));
		snd_mixer_close(m_mixer);
		m_mixer = NULL;
		g_free(card_override);
		return;
	} else {
		g_free(card_override);
	}
	snd_mixer_selem_register(m_mixer, NULL, NULL);
	snd_mixer_load(m_mixer);

	// Setup g_io_watch for the mixer
	int count = snd_mixer_poll_descriptors_count(m_mixer);
	if(count >= 1)
	{
		struct pollfd pfd;

		count = snd_mixer_poll_descriptors(m_mixer, &pfd, 1);
		if(count == 1)
		{
			GIOChannel * giochannel = g_io_channel_unix_new(pfd.fd);
			g_io_add_watch_full(giochannel, G_PRIORITY_DEFAULT,
				G_IO_IN | G_IO_ERR , asound_poll_cb, NULL, NULL);
		}
	}

	// Iterate over the elements in the mixer and store them in m_channel_names
	int elemcount = snd_mixer_get_count(m_mixer);
	snd_mixer_elem_t * elem = snd_mixer_first_elem(m_mixer);
	int loop;
	for(loop = 0; loop < elemcount; loop++)
	{
		const char * elemname = snd_mixer_selem_get_name(elem);
		if(snd_mixer_selem_has_playback_volume(elem))
		{
			m_channel_names = g_list_append(m_channel_names,
				(gpointer)g_strdup(elemname));
		}
		elem = snd_mixer_elem_next(elem);
	}

	// Setup m_elem using the provided channelname
	if(channel != NULL && asound_channel_exists(channel))
		asound_set_channel(channel);
	else if(m_channel_names != NULL)
		asound_set_channel((const gchar*)m_channel_names->data);
}

void asound_set_channel(const gchar * channel)
{
	if(m_mixer == NULL || channel == NULL) {
		return;
	}
	if(g_strcmp0(channel, m_channel) == 0)
		return;

	// Clean up any previously set channels
	g_free(m_channel);
	m_channel = g_strdup(channel);
	if(m_elem)
	{
		snd_mixer_elem_set_callback(m_elem, NULL);
		m_elem = NULL;
	}

	// Setup m_elem using the provided channelname
	snd_mixer_selem_id_t * sid;
	snd_mixer_selem_id_malloc(&sid);
	snd_mixer_selem_id_set_name(sid, channel);
	m_elem = snd_mixer_find_selem(m_mixer, sid);
	if(m_elem != NULL)
	{
		snd_mixer_elem_set_callback(m_elem, asound_elem_event);
		snd_mixer_selem_id_free(sid);
	}
}

void asound_set_mute(gboolean mute)
{
	if(m_elem == NULL) {
		return;
	}

	if(snd_mixer_selem_has_playback_switch(m_elem))
	{
		snd_mixer_selem_set_playback_switch_all(m_elem, !mute);
	}
	else if(mute)
	{
		asound_set_volume(0);
	}
}

void asound_set_volume(int volume)
{
	if(m_elem == NULL) {
		return;
	}
	volume = (volume < 0 ? 0 : (volume > 100 ? 100 : volume));

	set_normalized_playback_volume_all(m_elem, volume / 100.0, 0);
}
