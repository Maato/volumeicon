Volume Icon
===========
A lightweight volume control applet with support for global keybindings

Compilation
-----------

```raw
  $ ./autogen.sh
  $ ./configure --prefix=/usr
  $ make
  $ sudo make install
```

Compilation Flags
-----------------
```
--enable-oss: By default Volume Icon will be built with ALSA as its
              backend. Note that it is not possible to build with both ALSA
              and OSS support at the moment, so using this flag will
              disable ALSA support.

--enable-notify: Enables notifications, this adds a dependency for
                 libnotify >= 0.5.0.

--with-oss-include-path: Location of soundcard.h, defaults first to the value
                         specified in /etc/oss.conf. If that does not exist it
                         defaults to /usr/lib/oss/include/sys.

--with-default-mixerapp: Set the default mixer application, defaults to alsamixer.
```

Build Dependencies
------------------
To run `./autogen.sh`, *intltool* must be installed.

The following packages must be installed for compilation (Debian names given):
* libasound2-dev
* libglib2.0-dev
* libgtk-3-dev
* perl (uses `pod2man` to generate man pages)
