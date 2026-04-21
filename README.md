# mywm-x11

A minimal X11 tiling and floating window manager written in C.

## Overview

mywm-x11 is a simple window manager built using Xlib. It provides a basic master-stack tiling layout along with a floating mode for manual window control. This project is intended for learning how window managers work at a low level.

## Features

* Master-stack tiling layout
* Toggleable floating mode
* Mouse-based window move and resize (floating mode)
* Basic window focus handling
* Simple keybindings for launching applications and managing windows

## Requirements

* Linux system with X11
* Xlib development libraries

### Install dependencies

Arch Linux:

```sh
sudo pacman -S libx11
```

Debian/Ubuntu:

```sh
sudo apt install libx11-dev
```

## Build

```sh
gcc mywm.c -o mywm -lX11
```

## Run

```sh
startx ./mywm
```

## Keybindings

* Super + T : Launch terminal (kitty)
* Super + F : Launch Firefox
* Super + Q : Close focused window
* Super + V : Toggle tiling / floating mode

## Mouse Controls (Floating Mode)

* Super + Left Click : Move window
* Super + Right Click : Resize window

## Notes

* This is a minimal implementation and does not include advanced features found in full window managers.
* Behavior may vary depending on system configuration.

## License

MIT License
