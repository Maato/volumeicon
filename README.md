# VolumeIcon

## Description

Volume Icon aims to be a lightweight volume control that sits in your systray.

## Installation

  `./configure --prefix=/usr
  make
  sudo make install`

## Configuration

`--enable-oss`
  By default Volume Icon will be built with ALSA as its  
  backend. Note that it is not possible to build with both ALSA  
  and OSS support at the moment, so using this flag will  
  disable ALSA support.

`--enable-notify`
  Enables notifications, this adds a dependency for  
  libnotify >= 0.5.0.

## Usage

  Type `volumeicon &` in terminal.  
  *Note:* Add this command into your startup app list to enable showing sound applet on system boot

## Know Issues

  If you have few soundcards in your system and getting such like error message on app start:  
  `volumeicon: alsa_backend.c:86: asound_get_volume: Assertion 'm_elem != ((void *)0)' failed.
  Aborted`  
  Then you need to change in `$HOME/.config/volumeicon/volumeicon` integer value from 0 to 1 in *[Alsa]* block.
