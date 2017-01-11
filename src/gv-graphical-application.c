#include <glib.h>
#include <glib-object.h>
#include <gtk/gtk.h>

#include "additions/glib-object.h"

#include "framework/log.h"

#include "framework/gv-framework.h"
#include "core/gv-core.h"
#include "ui/gv-ui.h"

#include "ui/gv-ui-helpers.h"


#include "gv-graphical-application.h"
#include "options.h"

/*
 * Properties
 */

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	// TODO fill with your properties
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * Signals
 */

/*
 * GObject definitions
 */

struct _GvGraphicalApplicationPrivate {
	// TODO fill with your data
};

typedef struct _GvGraphicalApplicationPrivate GvGraphicalApplicationPrivate;

struct _GvGraphicalApplication {
	/* Parent instance structure */
	GtkApplication parent_instance;
	/* Private data */
	GvGraphicalApplicationPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(GvGraphicalApplication, gv_graphical_application, GTK_TYPE_APPLICATION)

/*
 * Helpers
 */

static const gchar *
stringify_list(const gchar *prefix, GList *list)
{
	GList *item;
	GString *str;
	static gchar *text;

	str = g_string_new(prefix);
	g_string_append(str, "[");

	for (item = list; item; item = item->next) {
		GObject *object;
		const gchar *object_name;

		object = item->data;
		object_name = G_OBJECT_TYPE_NAME(object);

		g_string_append_printf(str, "%s, ", object_name);
	}

	if (list != NULL)
		g_string_set_size(str, str->len - 2);

	g_string_append(str, "]");

	g_free(text);
	text = g_string_free(str, FALSE);

	return text;
}

/*
 * Property accessors
 */

static void
gv_graphical_application_get_property(GObject    *object,
                                      guint       property_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	// TODO handle properties
	(void) self;
	(void) value;

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
gv_graphical_application_set_property(GObject      *object,
                                      guint         property_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	// TODO handle properties
	(void) self;
	(void) value;

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

GApplication *
gv_graphical_application_new(void)
{
	return G_APPLICATION(g_object_new(GV_TYPE_GRAPHICAL_APPLICATION,
	                                  "application-id", "org." PACKAGE_CAMEL_NAME,
	                                  "flags", G_APPLICATION_FLAGS_NONE,
	                                  NULL));
}

/*
 * GApplication actions
 */

static void
preferences_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                      GVariant      *parameters G_GNUC_UNUSED,
                      gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_present_preferences();
}

static void
help_action_cb(GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameters G_GNUC_UNUSED,
               gpointer       user_data G_GNUC_UNUSED)
{
	g_app_info_launch_default_for_uri(PACKAGE_WEBSITE, NULL, NULL);
}

static void
about_action_cb(GSimpleAction *action G_GNUC_UNUSED,
                GVariant      *parameters G_GNUC_UNUSED,
                gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_present_about();
}

static void
quit_action_cb(GSimpleAction *action G_GNUC_UNUSED,
               GVariant      *parameters G_GNUC_UNUSED,
               gpointer       user_data G_GNUC_UNUSED)
{
	gv_ui_quit();
}

static const GActionEntry gv_graphical_application_actions[] = {
	{ "preferences", preferences_action_cb, NULL, NULL, NULL, {0} },
	{ "help",        help_action_cb,        NULL, NULL, NULL, {0} },
	{ "about",       about_action_cb,       NULL, NULL, NULL, {0} },
	{ "quit",        quit_action_cb,        NULL, NULL, NULL, {0} },
	{ NULL,          NULL,                  NULL, NULL, NULL, {0} }
};

/*
 * GApplication methods
 */

static void
gv_graphical_application_shutdown(GApplication *app)
{
	DEBUG(">>>> Shutting down application <<<<");

	/* Cool down */
	DEBUG("---- Cooling down ui ----");
	gv_ui_cool_down();

	DEBUG("---- Cooling down core ----");
	gv_core_cool_down();

	/* Cleanup */
	DEBUG("---- Cleaning up ui ----");
	gv_ui_cleanup();

	DEBUG("---- Cleaning up core ----");
	gv_core_cleanup();

	DEBUG("---- Cleaning up framework ----");
	gv_framework_cleanup();

	/* Mandatory chain-up */
	G_APPLICATION_CLASS(gv_graphical_application_parent_class)->shutdown(app);
}

static void
gv_graphical_application_startup(GApplication *app)
{
	gboolean prefers_app_menu;

	DEBUG(">>>> Starting application <<<<");

	/* Mandatory chain-up, see:
	 * https://developer.gnome.org/gtk3/stable/GtkApplication.html#gtk-application-new
	 */
	G_APPLICATION_CLASS(gv_graphical_application_parent_class)->startup(app);

	/* Add actions to the application */
	g_action_map_add_action_entries(G_ACTION_MAP(app),
	                                gv_graphical_application_actions,
	                                -1,
	                                NULL);

	/* Check how the application prefers to display it's main menu */
	prefers_app_menu = gtk_application_prefers_app_menu(GTK_APPLICATION(app));
	DEBUG("Application prefers... %s", prefers_app_menu ? "app-menu" : "menubar");

	/* Gnome-based desktop environments prefer an application menu.
	 * Legacy mode also needs this app menu, although it won't be displayed,
	 * but instead it will be brought up with a right-click.
	 */
	if (prefers_app_menu || options.status_icon == TRUE) {
		GtkBuilder *builder;
		GMenuModel *model;
		gchar *uifile;

		gv_builder_load("ui/app-menu.glade", &builder, &uifile);
		model = G_MENU_MODEL(gtk_builder_get_object(builder, "app-menu"));
		gtk_application_set_app_menu(GTK_APPLICATION(app), model);
		DEBUG("App menu set from ui file '%s'", uifile);
		g_free(uifile);
		g_object_unref(builder);
	}

	/* Unity-based and traditional desktop environments prefer a menu bar */
	if (!prefers_app_menu && options.status_icon == FALSE) {
		GtkBuilder *builder;
		GMenuModel *model;
		gchar *uifile;

		gv_builder_load("ui/menubar.glade", &builder, &uifile);
		model = G_MENU_MODEL(gtk_builder_get_object(builder, "menubar"));
		gtk_application_set_menubar(GTK_APPLICATION(app), model);
		DEBUG("Menubar set from ui file '%s'", uifile);
		g_free(uifile);
		g_object_unref(builder);
	}

	/* Initialization */
	DEBUG("---- Initializing framework ----");
	gv_framework_init();

	DEBUG("---- Initializing core ----");
	gv_core_init();

	DEBUG("---- Initializing ui ----");
	gv_ui_init(app, options.status_icon);

	/* Debug messages */
	DEBUG("---- Peeping into lists ----");
	DEBUG("%s", stringify_list("Feature list     : ", gv_framework_feature_list));
	DEBUG("%s", stringify_list("Configurable list: ", gv_framework_configurable_list));
	DEBUG("%s", stringify_list("Errorable list   : ", gv_framework_errorable_list));

	/* Warm-up */
	DEBUG("---- Warming up core ----");
	gv_core_warm_up(options.uri_to_play);

	DEBUG("---- Warming up ui ----");
	gv_ui_warm_up();

	/* Hold application */
	// TODO: move that somewhere else
	g_application_hold(app);
}

static void
gv_graphical_application_activate(GApplication *app G_GNUC_UNUSED)
{
	DEBUG("Activated !");

	gv_ui_present_main();
}

/*
 * GObject methods
 */

static void
gv_graphical_application_finalize(GObject *object)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(object);
	GvGraphicalApplicationPrivate *priv = self->priv;

	TRACE("%p", object);

	// TODO job to be done
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(gv_graphical_application, object);
}

static void
gv_graphical_application_constructed(GObject *object)
{
	GvGraphicalApplication *self = GV_GRAPHICAL_APPLICATION(object);
	GvGraphicalApplicationPrivate *priv = self->priv;

	TRACE("%p", object);

	/* Initialize properties */
	// TODO
	(void) priv;

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(gv_graphical_application, object);
}

static void
gv_graphical_application_init(GvGraphicalApplication *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = gv_graphical_application_get_instance_private(self);
}

static void
gv_graphical_application_class_init(GvGraphicalApplicationClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	GApplicationClass *application_class = G_APPLICATION_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = gv_graphical_application_finalize;
	object_class->constructed = gv_graphical_application_constructed;

	/* Override GApplication methods */
	application_class->startup =  gv_graphical_application_startup;
	application_class->shutdown = gv_graphical_application_shutdown;
	application_class->activate = gv_graphical_application_activate;

	/* Properties */
	object_class->get_property = gv_graphical_application_get_property;
	object_class->set_property = gv_graphical_application_set_property;

	// TODO define your properties here
	//      use GV_PARAM_DEFAULT_FLAGS

	g_object_class_install_properties(object_class, PROP_N, properties);
}
