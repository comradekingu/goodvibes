# Process this file with autoconf

# Init autoconf
# 2.60: obsoleting plenty of stuff
AC_PREREQ([2.60])
AC_INIT([overcooked], [0.1-dev])
: ${CFLAGS="-std=gnu99 -Wall -Wextra -Wshadow"}

# Init automake
# 1.11: silent-rules
# 1.14: obsoleting AM_PROG_CC_C_O
AM_INIT_AUTOMAKE([1.14 foreign subdir-objects])
AM_SILENT_RULES([yes])

# Load our own set of m4 macros 
AC_CONFIG_MACRO_DIRS([m4])

# Additional package information
OCK_ADD_GLOBAL_VAR([PACKAGE_CAMEL_NAME],   ["Overcooked"])
OCK_ADD_GLOBAL_VAR([PACKAGE_LONG_NAME],    ["Overcooked Radio Player"])
OCK_ADD_GLOBAL_VAR([PACKAGE_AUTHOR_NAME],  ["Arnaud Rebillout (elboulangero)"])
OCK_ADD_GLOBAL_VAR([PACKAGE_AUTHOR_EMAIL], ["<elboulangero@gmail.com>"])
OCK_ADD_GLOBAL_VAR([PACKAGE_WEBSITE],      ["https://github.com/elboulangero/Overcooked"])

# ------------------------------------- #
#              Toolchain                #
# ------------------------------------- #

# Check for toolchain and install components

AS_ECHO(["---- Toolchain ----"])

AC_PROG_CC()
AC_PROG_INSTALL()
PKG_PROG_PKG_CONFIG()

# To build libgszn as a static library
AC_PROG_RANLIB()
AM_PROG_AR()

# ------------------------------------- #
#         Mandatory dependencies        #
# ------------------------------------- #

# Check for core dependencies

AS_ECHO(["---- Core ----"])

PKG_CHECK_MODULES([GLIB],    [glib-2.0, gio-2.0, gobject-2.0 >= 2.42])
PKG_CHECK_MODULES([LIBSOUP], [libsoup-2.4 >= 2.42])
PKG_CHECK_MODULES([GST],     [gstreamer-1.0, gstreamer-base-1.0, gstreamer-audio-1.0 >= 1.4.4])
PKG_CHECK_MODULES([XML],     [libxml-2.0 >= 2.9.4])

# Libgszn flags
GSZN_CFLAGS="-I.."
GSZN_LIBS="../libgszn/libgszn.a"
AC_SUBST(GSZN_CFLAGS)
AC_SUBST(GSZN_LIBS)

# ------------------------------------- #
#         Optional dependencies         #
# ------------------------------------- #

# The default behavior is as following:
# - a feature that doesn't require an extra library is enabled.
# - a feature that requires an extra library is set to 'auto',
#   and will be enabled only if the required library is found.

feat_noextra="yes"
feat_extra="auto"

# The default behavior can be modified with the '--enable-all' switch.
# If you want to use it, expect the following behavior:
# - --enable-all   Build a full-featured version (maximum dependencies).
#                  This will force the compilation of every feature,
#                  and fail the build if a dependency is not met.
# - --disable-all  Build a lightweight version (minimum dependencies).
#                  This will exclude every feature from the build.

AC_ARG_ENABLE(all, AS_HELP_STRING([--enable-all], [Enable every features]))

if test -n "$enable_all"; then
  feat_noextra="$enable_all"
  feat_extra="$enable_all"
fi

# For a higher degree of granularity, every feature has its own switch.
# If you want to use it, expect the following behavior:
# - --enable-FEATURE   Force the compilation of a feature, and fail
#                      the build if dependencies are not met.
# - --disable-FEATURE  Exclude a feature from the build.

# Check for ui dependencies
# Notice that the UI is managed as a feature here.

AS_ECHO(["---- Ui ----"])

OCK_FEATURE_ENABLE([ui], [Gtk+ graphical user interface], [$feat_extra])
OCK_FEATURE_PKG_CHECK([ui], [UI], [gtk+-3.0 >= 3.12.0])
AM_CONDITIONAL([UI_ENABLED], [test "$enable_ui" = "yes"])

# Check for core features dependencies

AS_ECHO(["---- Core Features ----"])

OCK_FEATURE_ENABLE([dbus_native],    [native dbus server support], [$feat_noextra])
OCK_FEATURE_ENABLE([dbus_mpris2],    [mpris2 dbus server support], [$feat_noextra])
OCK_FEATURE_ENABLE([console_output], [console output support],     [$feat_noextra])
OCK_FEATURE_ENABLE([inhibit],        [inhibit support],            [$feat_extra])

OCK_FEATURE_PKG_CHECK([inhibit], [INHIBIT], [gio-unix-2.0 >= 2.42])

AM_CONDITIONAL([DBUS_NATIVE_ENABLED],    [test "$enable_dbus_native" = "yes"])
AM_CONDITIONAL([DBUS_MPRIS2_ENABLED],    [test "$enable_dbus_mpris2" = "yes"])
AM_CONDITIONAL([CONSOLE_OUTPUT_ENABLED], [test "$enable_console_output" = "yes"])
AM_CONDITIONAL([INHIBIT_ENABLED],        [test "$enable_inhibit" = "yes"])

# Check for ui features dependencies
# If the ui is disabled, ensure features are disabled as well.

AS_ECHO(["---- Ui Features ----"])

if test "$enable_ui" = "no"; then
  feat_noextra="no"
  feat_extra="no"
fi

OCK_FEATURE_ENABLE([hotkeys],       [hotkeys support],       [$feat_extra])
OCK_FEATURE_ENABLE([notifications], [notifications support], [$feat_extra])

OCK_FEATURE_PKG_CHECK([hotkeys],       [HOTKEYS],       [keybinder-3.0 >= 0.3])
OCK_FEATURE_PKG_CHECK([notifications], [NOTIFICATIONS], [libnotify >= 0.7])

AM_CONDITIONAL([HOTKEYS_ENABLED],       [test "$enable_hotkeys" = "yes"])
AM_CONDITIONAL([NOTIFICATIONS_ENABLED], [test "$enable_notifications" = "yes"])

# ------------------------------------- #
#              Output files             #
# ------------------------------------- #

# Files to be created
AC_CONFIG_FILES([
AUTHORS
Makefile
cli/Makefile
data/Makefile
data/dbus-1/Makefile
data/dbus-1/services/Makefile
data/dbus-1/services/org.$PACKAGE_CAMEL_NAME.service
data/desktop/Makefile
data/desktop/$PACKAGE_NAME.desktop
data/icons/Makefile
data/icons/hicolor/Makefile
data/icons/hicolor/16x16/Makefile
data/icons/hicolor/16x16/apps/Makefile
data/icons/hicolor/22x22/Makefile
data/icons/hicolor/22x22/apps/Makefile
data/icons/hicolor/24x24/Makefile
data/icons/hicolor/24x24/apps/Makefile
data/icons/hicolor/32x32/Makefile
data/icons/hicolor/32x32/apps/Makefile
data/icons/hicolor/48x48/Makefile
data/icons/hicolor/48x48/apps/Makefile
data/icons/hicolor/256x256/Makefile
data/icons/hicolor/256x256/apps/Makefile
data/icons/src/Makefile
data/ui/Makefile
libgszn/Makefile
src/Makefile
])

AC_OUTPUT()

# ------------------------------------- #
#                Summary                #
# ------------------------------------- #

# String for silent rules
is_silent_rules_enabled=no
if test "$AM_DEFAULT_VERBOSITY" = "0"; then
   is_silent_rules_enabled=yes
fi

AS_ECHO(["
-------------------------------------------------------------

	Overcooked Radio Player
	=======================

	Core
	----
	Console output    : $enable_console_output
	Native dbus server: $enable_dbus_native
	Mpris2 dbus server: $enable_dbus_mpris2
	Inhibit           : $enable_inhibit

	Ui                : $enable_ui
	--
	Hotkeys           : $enable_hotkeys
	Notifications     : $enable_notifications

	Build
	-----
	Silent rules      : $is_silent_rules_enabled
	CC                : $CC
	CFLAGS            : $CFLAGS
	CPPFLAGS          : $CPPFLAGS
	LDFLAGS           : $LDFLAGS
	LIBS              : $LIBS

-------------------------------------------------------------

Configuration finished, please type 'make' to compile."])
