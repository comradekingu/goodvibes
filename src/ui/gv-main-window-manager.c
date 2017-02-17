/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "framework/gv-framework.h"
#include "ui/gv-ui-internal.h"
#include "ui/gv-main-window.h"

#include "ui/gv-main-window-manager.h"

/*
 * Reasons to have a separate object to deal with the main window configuration,
 * aka size, position, maximized.
 *
 * Delayed method calls
 * --------------------
 *
 * We invoke methods on the main window in two contexts:
 * - at init time (to load configuration)
 * - in signal handlers (to save configuration)
 *
 * Both of these contexts are not suitable to query the window size, and because
 * of that we need to delay all the method calls. This logic is better taken out
 * of the main window, and handled here in the window manager.
 *
 * Window finalization
 * -------------------
 *
 * Saving the window configuration is an operation that is delayed by a timeout,
 * and therefore there's a possibility that the main window is finalized before
 * the timeout is reached. If this 'save timeout' is handled by the main window,
 * then we have to try to save the window configuration from within the finalize
 * method of the window itself, and it doesn't work. It's too late to query the
 * window at this moment.
 *
 * The window manager solves this problem elegantly, since the 'save timeout' is
 * taken out of the main window, and handled by the manager. The manager is sure
 * to be finalized before the main window, and therefore can safely query it at
 * this moment.
 */

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_MAIN_WINDOW,
	PROP_STATUS_ICON_MODE,
	PROP_AUTOSET_HEIGHT,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _GvMainWindowManagerPrivate {
	/* Properties */
	GvMainWindow *main_window;
	gboolean      status_icon_mode;
	gboolean      autoset_height;
	/* Window configuration */
	guint save_window_configuration_source_id;
};

typedef struct _GvMainWindowManagerPrivate GvMainWindowManagerPrivate;

struct _GvMainWindowManager {
	/* Parent instance structure */
	GObject                     parent_instance;
	/* Private data */
	GvMainWindowManagerPrivate *priv;
};

static void gv_main_window_manager_configurable_interface_init(GvConfigurableInterface *iface);

G_DEFINE_TYPE_WITH_CODE(GvMainWindowManager, gv_main_window_manager, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(GvMainWindowManager)
                        G_IMPLEMENT_INTERFACE(GV_TYPE_CONFIGURABLE,
                                        gv_main_window_manager_configurable_interface_init))

/*
 * Private methods
 */

static void
gv_main_window_manager_save_configuration_now(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;
	GtkWindow *window = GTK_WINDOW(priv->main_window);
	GSettings *settings = gv_ui_settings;
	gint width, height, old_width, old_height;
	gint x, y, old_x, old_y;

	TRACE("%p", self);

	/* This is never invoked if the window is in status icon mode */
	g_assert_false(priv->status_icon_mode);

	/* Don't save anything when window is maximized */
	if (gtk_window_is_maximized(window))
		return;

	/* Save size if changed */
	gtk_window_get_size(window, &width, &height);
	g_settings_get(settings, "window-size", "(ii)", &old_width, &old_height);
	if ((width != old_width) || (height != old_height))
		g_settings_set(settings, "window-size", "(ii)", width, height);

	/* Save position if changed */
	gtk_window_get_position(window, &x, &y);
	g_settings_get(settings, "window-position", "(ii)", &old_x, &old_y);
	if ((x != old_x) || (y != old_y))
		g_settings_set(settings, "window-position", "(ii)", x, y);
}

static gboolean
when_timeout_save_window_configuration(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	gv_main_window_manager_save_configuration_now(self);

	priv->save_window_configuration_source_id = 0;

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_manager_save_configuration_delayed(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	if (priv->save_window_configuration_source_id > 0)
		g_source_remove(priv->save_window_configuration_source_id);

	priv->save_window_configuration_source_id =
	        g_timeout_add_seconds(1, (GSourceFunc) when_timeout_save_window_configuration,
	                              self);
}

static gboolean on_main_window_configure_event(GtkWindow *window,
                GdkEventConfigure *event,
                GvMainWindowManager *self);
static void
gv_main_window_manager_load_configuration_now(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;
	GSettings *settings = gv_ui_settings;
	gint width, height;
	gint x, y;

	TRACE("%p", self);

	/* In status icon mode, configuration is ignored. We neither read nor write it.
	 * The only thing we want is to ensure that the height is automatically set,
	 * and to initialize it properly.
	 */
	if (priv->status_icon_mode) {
		GvMainWindow *window = GV_MAIN_WINDOW(priv->main_window);
		gint natural_height;

		gv_main_window_manager_set_autoset_height(self, TRUE);
		natural_height = gv_main_window_get_natural_height(window);
		gv_main_window_resize_height(window, natural_height);

		return;
	}

	/* Get settings */
	g_settings_get(settings, "window-size", "(ii)", &width, &height);
	g_settings_get(settings, "window-position", "(ii)", &x, &y);
	g_settings_bind(settings, "window-autoset-height",
	                self, "autoset-height", G_SETTINGS_BIND_DEFAULT);

	/* Set initial window size */
	if (width != -1 && height != -1) {
		GtkWindow *window = GTK_WINDOW(priv->main_window);

		DEBUG("Restoring window size (%d, %d)", width, height);
		gtk_window_resize(window, width, height);

	} else {
		GvMainWindow *window = GV_MAIN_WINDOW(priv->main_window);
		gint natural_height;

		DEBUG("No window size specified yet, setting automatically");
		natural_height = gv_main_window_get_natural_height(window);
		gv_main_window_resize_height(window, natural_height);
	}

	/* Set initial window position */
	if (x != -1 || y != -1) {
		GtkWindow *window = GTK_WINDOW(priv->main_window);

		DEBUG("Restoring window position (%d, %d)", x, y);
		gtk_window_move(window, x, y);
	}

	/* Connect to the 'configure-event' signal to save changes when the
	 * window size or position is modified.
	 */
	g_signal_connect_object(priv->main_window, "configure-event",
	                        G_CALLBACK(on_main_window_configure_event),
	                        self, 0);
}

static gboolean
when_idle_load_window_configuration(GvMainWindowManager *self)
{
	gv_main_window_manager_load_configuration_now(self);

	return G_SOURCE_REMOVE;
}

static void
gv_main_window_manager_load_configuration_delayed(GvMainWindowManager *self)
{
	/* This is called only once, no need to bother with the source id */
	g_idle_add((GSourceFunc) when_idle_load_window_configuration, self);
}

/*
 * Window signal handlers
 */

static void
on_main_window_notify_natural_height(GvMainWindow *window,
                                     GParamSpec *pspec G_GNUC_UNUSED,
                                     GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;
	gint natural_height;

	/* This signal handler is invoked when the natural height of the main
	 * window was changed, and therefore we want to resize it.
	 */

	/* Should never be invoked if autoset-height is FALSE */
	g_assert_true(priv->autoset_height);

	natural_height = gv_main_window_get_natural_height(window);
	gv_main_window_resize_height(window, natural_height);
}

static gboolean
on_main_window_configure_event(GtkWindow *window G_GNUC_UNUSED,
                               GdkEventConfigure *event G_GNUC_UNUSED,
                               GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* This signal handler is invoked multiple times during resizing (really).
	 * It only means that resizing is in progress, and there is no way to know
	 * that resizing is finished.
	 * The (dumb) strategy to handle that is just to delay our reaction.
	 */

	/* Should never be invoked in status icon mode */
	g_assert_false(priv->status_icon_mode);

	gv_main_window_manager_save_configuration_delayed(self);

	return FALSE;
}

/*
 * Public functions
 */

GvMainWindowManager *
gv_main_window_manager_new(GvMainWindow *main_window, gboolean status_icon_mode)
{
	return g_object_new(GV_TYPE_MAIN_WINDOW_MANAGER,
	                    "main-window", main_window,
	                    "status-icon-mode", status_icon_mode,
	                    NULL);
}

/*
 * Property accessors
 */

static void
gv_main_window_manager_set_main_window(GvMainWindowManager *self, GvMainWindow *main_window)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_null(priv->main_window);
	g_assert_nonnull(main_window);
	g_set_object(&priv->main_window, main_window);
}

static void
gv_main_window_manager_set_status_icon_mode(GvMainWindowManager *self, gboolean status_icon_mode)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* This is a construct-only property */
	g_assert_false(priv->status_icon_mode);
	priv->status_icon_mode = status_icon_mode;
}

gboolean
gv_main_window_manager_get_autoset_height(GvMainWindowManager *self)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	return priv->autoset_height;
}

void
gv_main_window_manager_set_autoset_height(GvMainWindowManager *self, gboolean autoset_height)
{
	GvMainWindowManagerPrivate *priv = self->priv;

	/* In status icon mode, this property is forced to TRUE */
	if (priv->status_icon_mode) {
		DEBUG("Status icon mode: 'autoset-height' forced to TRUE");
		autoset_height = TRUE;
	}

	if (priv->autoset_height == autoset_height)
		return;

	if (autoset_height) {
		g_signal_connect_object
		(priv->main_window, "notify::natural-height",
		 G_CALLBACK(on_main_window_notify_natural_height),
		 self, 0);
	} else {
		g_signal_handlers_disconnect_by_func
		(priv->main_window,
		 G_CALLBACK(on_main_window_notify_natural_height),
		 self);
	}

	priv->autoset_height = autoset_height;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_AUTOSET_HEIGHT]);
}

static void
gv_main_window_manager_get_property(GObject    *object,
                                    guint       property_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_AUTOSET_HEIGHT:
		g_value_set_boolean(value, gv_main_window_manager_get_autoset_height(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_main_window_manager_set_property(GObject      *object,
                                    guint         property_id,
                                    const GValue *value,
                                    GParamSpec   *pspec)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_MAIN_WINDOW:
		gv_main_window_manager_set_main_window(self, g_value_get_object(value));
		break;
	case PROP_STATUS_ICON_MODE:
		gv_main_window_manager_set_status_icon_mode(self, g_value_get_boolean(value));
		break;
	case PROP_AUTOSET_HEIGHT:
		gv_main_window_manager_set_autoset_height(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * GvConfigurable interface
 */

static void
gv_main_window_manager_configure(GvConfigurable *configurable)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(configurable);

	gv_main_window_manager_load_configuration_delayed(self);
}

static void
gv_main_window_manager_configurable_interface_init(GvConfigurableInterface *iface)
{
	iface->configure = gv_main_window_manager_configure;
}

/*
 * GObject methods
 */

static void
gv_main_window_manager_finalize(GObject *object)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);
	GvMainWindowManagerPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Run any pending save operation */
	if (priv->save_window_configuration_source_id > 0)
		when_timeout_save_window_configuration(self);

	/* Release resources */
	g_object_unref(priv->main_window);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_main_window_manager, object);
}

static void
gv_main_window_manager_constructed(GObject *object)
{
	GvMainWindowManager *self = GV_MAIN_WINDOW_MANAGER(object);
	GvMainWindowManagerPrivate *priv =self->priv;

	TRACE("%p", self);

	/* Ensure construct-only properties have been set */
	g_assert_nonnull(priv->main_window);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_main_window_manager, object);
}

static void
gv_main_window_manager_init(GvMainWindowManager *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_main_window_manager_get_instance_private(self);
}

static void
gv_main_window_manager_class_init(GvMainWindowManagerClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_main_window_manager_finalize;
	object_class->constructed = gv_main_window_manager_constructed;

	/* Properties */
	object_class->get_property = gv_main_window_manager_get_property;
	object_class->set_property = gv_main_window_manager_set_property;

	properties[PROP_MAIN_WINDOW] =
	        g_param_spec_object("main-window", "Main window", NULL,
	                            GV_TYPE_MAIN_WINDOW,
	                            GV_PARAM_DEFAULT_FLAGS | G_PARAM_CONSTRUCT_ONLY |
	                            G_PARAM_WRITABLE);

	properties[PROP_STATUS_ICON_MODE] =
	        g_param_spec_boolean("status-icon-mode", "Status icon mode", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_CONSTRUCT_ONLY |
	                             G_PARAM_WRITABLE);

	properties[PROP_AUTOSET_HEIGHT] =
	        g_param_spec_boolean("autoset-height", "Autoset height", NULL,
	                             FALSE,
	                             GV_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
