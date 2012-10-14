#!/bin/sh

autoreconf -if || exit 1
glib-gettextize --force --copy || exit 1
intltoolize --force --copy --automake
