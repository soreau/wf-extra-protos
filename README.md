# wf-extra-protos
Extra wayland protocol support for wayfire, featuring wf-menu

## Build

meson build

ninja -C build

sudo ninja -C build install

## Runtime

Enable Extra Protocols plugin

Right click on gtk titlebar to show menu

Optionally show menu from your client using [this xdg function](https://gitlab.freedesktop.org/wayland/wayland-protocols/-/blob/master/stable/xdg-shell/xdg-shell.xml#L669-687)
