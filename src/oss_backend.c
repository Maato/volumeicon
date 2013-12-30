//##############################################################################
// volumeicon
//
// oss_backend.c - implements a volume control abstraction using oss
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

#include OSS_HEADER
#include <stropts.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <glib.h>

#include "oss_backend.h"

//##############################################################################
// Static variables
//##############################################################################
static char * m_channel = NULL;
static GList * m_channel_names = NULL;
static int m_actual_maxvalue = 0;
static int m_mixer_fd = -1;
static oss_mixext m_ext;

//##############################################################################
// Static functions
//##############################################################################
static int get_raw_value()
{
	assert(m_mixer_fd != -1);

	oss_mixer_value vr;
	vr.dev = m_ext.dev;
	vr.ctrl = m_ext.ctrl;
	vr.timestamp = m_ext.timestamp;

	int result = ioctl(m_mixer_fd, SNDCTL_MIX_READ, &vr);
	if(result == -1)
		return 0;

	/*
	If the control's type is MIXT_STEREOSLIDER16 or MIXT_MONOSLIDER16, we
	map the value read from the ioctl to a 32-bit field comprised of two
	16-bit values, with the left channel of a stereo control using the lower
	16 bits.
	Likewise, if the control's type has 8-bit precision, we map the values
	to a 16-bit field.
	*/
	struct { int16_t upper; int16_t lower; } * long_value;
	struct { int8_t upper; int8_t lower; } * short_value;
	switch(m_ext.type)
	{
		case(MIXT_STEREOSLIDER16):
		case(MIXT_MONOSLIDER16):
			long_value = (void*)&vr.value;
			return long_value->lower;

		case(MIXT_STEREOSLIDER):
		case(MIXT_MONOSLIDER):
			short_value = (void*)&vr.value;
			return short_value->lower;
	}

	return 0;
}

//##############################################################################
// Exported functions
//##############################################################################
const gchar * oss_get_channel()
{
	return m_channel;
}

const GList * oss_get_channel_names()
{
	return m_channel_names;
}

int oss_get_volume()
{
	assert(m_mixer_fd != -1);
	if(m_actual_maxvalue == 0)
		return 0;
	return 100 * get_raw_value() / m_actual_maxvalue;
}

gboolean oss_get_mute()
{
	assert(m_mixer_fd != -1);

	gboolean mute = FALSE;

	// Save current control;
	int parent = m_ext.parent;
	int control = m_ext.ctrl;

	// Check for mute in this group
	m_ext.ctrl = 0;
	while(ioctl(m_mixer_fd, SNDCTL_MIX_EXTINFO, &m_ext) >= 0)
	{
		if(m_ext.parent == parent && m_ext.type == MIXT_MUTE)
		{
			oss_mixer_value vr;
			vr.dev = m_ext.dev;
			vr.ctrl = m_ext.ctrl;
			vr.timestamp = m_ext.timestamp;
			if(ioctl(m_mixer_fd, SNDCTL_MIX_READ, &vr) != -1)
				mute = vr.value ? TRUE : FALSE;
			break;
		}
		m_ext.ctrl++;
	}

	// Restore to previous control
	m_ext.ctrl = control;
	ioctl(m_mixer_fd, SNDCTL_MIX_EXTINFO, &m_ext);

	return mute;
}

void oss_setup(const gchar * card, const gchar * channel,
	void (*volume_changed)(int,gboolean))
{
	// Make sure (for now) that the setup function only gets called once
	static int oss_setup_called = 0;
	assert(oss_setup_called == 0);
	oss_setup_called++;

	// Get ahold of the mixer device
	char * devmixer;
	if((devmixer=getenv("OSS_MIXERDEV")) == NULL)
		devmixer = "/dev/mixer";
	if((m_mixer_fd = open(devmixer, O_RDWR, 0)) == -1)
	{
		perror(devmixer);
		exit(1);
	}

	// Check that there is at least one mixer
	int nmix;
	ioctl(m_mixer_fd, SNDCTL_MIX_NRMIX, &nmix);
	if(nmix <= 0)
	{
		perror(devmixer);
		exit(EXIT_FAILURE);
	}

	m_ext.dev=0;
	m_ext.ctrl = 0;
	while(ioctl(m_mixer_fd, SNDCTL_MIX_EXTINFO, &m_ext) >= 0)
	{
		if(m_ext.type == MIXT_STEREOSLIDER16 || m_ext.type == MIXT_MONOSLIDER16 
		    || m_ext.type == MIXT_STEREOSLIDER || m_ext.type == MIXT_MONOSLIDER)
		{
			m_channel_names = g_list_append(m_channel_names,
				(gpointer)g_strdup(m_ext.extname));
		}
		m_ext.ctrl++;
	}

	// Setup channel using the provided channelname
	if(channel != NULL)
		oss_set_channel(channel);
	else if(channel == NULL && m_channel_names != NULL)
		oss_set_channel((const gchar*)m_channel_names->data);
}

void oss_set_channel(const gchar * channel)
{
	assert(channel != NULL);
	assert(m_mixer_fd != -1);

	if(g_strcmp0(channel, m_channel) == 0)
		return;

	// Clean up any previously set channels
	g_free(m_channel);
	m_channel = g_strdup(channel);

	// Find channel and then return
	m_ext.dev=0;
	m_ext.ctrl = 0;
	while(ioctl(m_mixer_fd, SNDCTL_MIX_EXTINFO, &m_ext) >= 0)
	{
		if(g_strcmp0(channel, m_ext.extname) == 0)
		{
			m_actual_maxvalue = m_ext.maxvalue;
			return;
		}
		m_ext.ctrl++;
	}
}

void oss_set_mute(gboolean mute)
{
	assert(m_mixer_fd != -1);

	gboolean mute_found = FALSE;
	int parent = m_ext.parent;
	int control = m_ext.ctrl;

	// Check for mute in this group
	m_ext.ctrl = 0;
	while(ioctl(m_mixer_fd, SNDCTL_MIX_EXTINFO, &m_ext) >= 0)
	{
		if(m_ext.parent == parent && m_ext.type == MIXT_MUTE)
		{
			oss_mixer_value vr;
			vr.dev = m_ext.dev;
			vr.ctrl = m_ext.ctrl;
			vr.timestamp = m_ext.timestamp;
			vr.value = mute ? 1 : 0;
			ioctl(m_mixer_fd, SNDCTL_MIX_WRITE, &vr);
			mute_found = TRUE;
			break;
		}
		m_ext.ctrl++;
	}

	// Restore to previous control
	m_ext.ctrl = control;
	ioctl(m_mixer_fd, SNDCTL_MIX_EXTINFO, &m_ext);

	// If no mute control was found, revert to setting the volume to zero
	if(!mute_found && mute)
	{
		oss_set_volume(0);
	}
}

void oss_set_volume(int volume)
{
	assert(m_mixer_fd != -1);
	volume = (volume < 0 ? 0 : (volume > 100 ? 100 : volume));

	oss_mixer_value vr;
	vr.dev = m_ext.dev;
	vr.ctrl = m_ext.ctrl;
	vr.timestamp = m_ext.timestamp;

	struct { int16_t upper; int16_t lower; } * long_value;
	struct { int8_t upper; int8_t lower; } * short_value;
	switch(m_ext.type) {
		case(MIXT_STEREOSLIDER16):
		case(MIXT_MONOSLIDER16):
			long_value = (void*)&vr.value;;
		case(MIXT_STEREOSLIDER):
		case(MIXT_MONOSLIDER):
			short_value = (void*)&vr.value;;
	}

	switch(m_ext.type)
	{
		case(MIXT_STEREOSLIDER16):
			long_value->upper = m_actual_maxvalue * volume / 100;
			long_value->lower = m_actual_maxvalue * volume / 100;
			break;
		case(MIXT_MONOSLIDER16):
			vr.value = m_actual_maxvalue * volume / 100;
			break;
		case(MIXT_STEREOSLIDER):
			short_value->upper = m_actual_maxvalue * volume / 100;
			short_value->lower = m_actual_maxvalue * volume / 100;
			break;
		case(MIXT_MONOSLIDER):
			vr.value = m_actual_maxvalue * volume / 100;
			break;
		default:
			return;
	}

	ioctl(m_mixer_fd, SNDCTL_MIX_WRITE, &vr);

	if(volume == 100)
		m_actual_maxvalue = get_raw_value();
}
