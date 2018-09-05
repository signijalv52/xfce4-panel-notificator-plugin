# xfce4-panel-notificator-plugin aka andromeda
This is simple xfce4-panel notificator plugin written on C using GTK3 library. Plugin can recieve text messages or something else via D-Bus. May be useful as alternative libnotify.
## required
written and tested using:
- Debian GNU/Linux 9.4 (x86 or x86-64)
- xfce4-panel 4.12.1-2
- dbus-glib-1 0.108-2
## compilation
gcc -shared -Wall -fPIC -o libandromeda.so andromeda.c \`pkg-config --cflags --libs libxfce4panel-2.0 dbus-glib-1\`
## build dependencies
- libxfce4panel-2.0-dev
- libdbus-glib-1-dev
## installation
- change in file *.desktop i386-linux-gnu to x86_64-linux-gnu if you use Debian 64bit
- sudo cp andromeda.desktop /usr/share/xfce4/panel-plugins/andromeda.desktop
- sudo cp libandromeda.so /usr/lib/i386-linux-gnu/xfce4/panel-plugins/libandromeda.so (or .../x86_64-linux-gnu/... for 64 bit system)
## debug info
- xfce4-panel -q
- PANEL_DEBUG=1 xfce4-panel
## known bugs
- panel must be placed on bottom only
## user manual
- add plugin on panel
- you can show/hide popup window by clicking on plugin button
- you can set text in popup window via dbus methods. Working example:
dbus-send --session --type=method_call --dest=andromeda.server / andromeda.listener.print string:'hello!'
