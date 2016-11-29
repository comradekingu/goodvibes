Overcooked Radio Player
=======================



About
-----

Overcooked Radio Player is a simple, lightweight internet radio player for the
GNU/Linux platform.

My main motivation is to provide a simple, reliable radio player, and hopefully
to keep it alive for many years. Long-term maintenance is the goal.

Overcooked was originally inspired by [RadioTray](http://radiotray.sourceforge.net/)
from Carlos Ribeiro. Both are similar in appearance, but have nothing in common
under the hood.

Overcooked is released under the [General Public License (GPL) version 3](LICENSE).

The projet is hosted on Github at <https://github.com/elboulangero/Overcooked>.



Using
-----

Overcooked comes with a minimalistic interface: it's just an icon in the system tray.
A left-click displays the control panel, a right-click pops up a short menu.

Adding radios require a little work from you, since you need to know the URL of
the radio stream you want to listen to. And for that, either you're good at 
googling, either you know how to inspect web pages and find a needle in a
haystack. If you're not so talented, you might find Overcooked a bit useless.

Overcooked comes with the following features:
- Music player basics
  - play/stop/previous/next
  - repeat/shuffle/volume
  - autoplay on startup
- Control from tray icon
  - Left-click opens a control panel
  - Right-click opens a popup menu
  - Middle-click and mouse-scroll are configurable
- More control
  - Multimedia hotkeys are supported
  - Native D-Bus server, along with a D-Bus client, allows command-line control
  - MPRIS2 D-Bus server allows a more generic command-line control
- Display
  - Notifications
  - Console output

For the command-line freaks, Overcooked can be launched without the UI.



Compiling
---------

Overcooked uses the `autotools` as a build system.
The procedure to compile is the usual one:

	./autogen.sh
	./configure
	make

Overcooked build is quite modular. Features that require an external library
are compiled only if the required dependencies are found on your system.
Otherwise they're excluded from the build.

You can change this behavior though. You can force a full-featured build, that
will fail if some of the required dependencies are missing.

	./configure --enable-all

Or you can do the opposite, and force a minimal build with none of the optional
features enabled.

	./configure --disable-all

On top of that, you can explicitly enable or disable any optional features,
using the `--enable-FEATURE` or `--disable-FEATURE` options (and replacing
*FEATURE* with a feature's name). Such parameters take precedence over the
`--enable-all` and `--disable-all` options.

For example, to have a minimal build with only the UI enabled:

	./configure --disable-all --enable-ui

To have a full-featured build but disable notifications:

	./configure  --enable-all --disable-notifications

Notice that disabling the UI will also disable all the UI-related features.

For more details, please have a look into the file `configure.ac`, or run:

	./configure -h



Installing
----------

As root, run the following command:

	make install



Required Packages
-----------------

Overcooked depends on the following packages.

To build the core:

|       Library         |         Debian package           |
| --------------------- | -------------------------------- |
| __GLib/GIO/GObject__	| libglib2.0-dev                   |
| __LibSoup__		| libsoup2.4-dev                   |
| __Libxml2__		| libxml2-dev                      |
| __GStreamer__		| libgstreamer1.0-dev              |
|			| libgstreamer-plugins-base1.0-dev |            

To build the full-featured ui:

|       Library         |    Debian package    |
| --------------------- | -------------------- |
| __GTK+__		| libgtk-3-dev         |
| __Libkeybinder__	| libkeybinder-3.0-dev |
| __Libnotify__		| libnotify-dev        |

For more details, please refer to the file `configure.ac`.

