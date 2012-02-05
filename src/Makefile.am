AM_CFLAGS = -Wall -DDATADIR=\"@datadir@/volumeicon\"
AM_CFLAGS += @GTK_CFLAGS@ @ALSA_CFLAGS@ @OSS_CFLAGS@ @X11_CFLAGS@ @NOTIFY_CFLAGS@

LIBS = @GTK_LIBS@ @ALSA_LIBS@ @X11_LIBS@ @NOTIFY_LIBS@

bin_PROGRAMS = volumeicon

if ENABLE_OSS
BACKEND = oss_backend.c oss_backend.h
else
BACKEND = alsa_backend.c alsa_backend.h
endif

volumeicon_SOURCES = \
	volumeicon.c \
	config.c \
	config.h \
	bind.c \
	keybinder.h \
	$(BACKEND)
