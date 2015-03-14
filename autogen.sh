#!/bin/sh

autoreconf -if || exit 1
# output of glib-gettextize is silenced because it is confusing
glib-gettextize --force --copy > /dev/null || { echo "glib-gettextize failed"; exit 1; }
intltoolize --force --copy --automake
