/*
 * Overcooked Radio Player
 *
 * Copyright (C) 2015-2016 Arnaud Rebillout
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
#include <libnotify/notify.h>

#include "additions/glib.h"

#include "libgszn/gszn.h"

#include "framework/log.h"
#include "framework/ock-feature.h"
#include "framework/ock-param-specs.h"

#include "core/ock-core.h"

#include "ui/feat/ock-notifications.h"

//#define ICON "audio-x-generic"
#define ICON PACKAGE_NAME

/*
 * Properties
 */

#define DEFAULT_TIMEOUT_ENABLED FALSE
#define MIN_TIMEOUT_SECONDS     1
#define MAX_TIMEOUT_SECONDS     10
#define DEFAULT_TIMEOUT_SECONDS 5

enum {
	/* Reserved */
	PROP_0,
	/* Properties */
	PROP_TIMEOUT_ENABLED,
	PROP_TIMEOUT_SECONDS,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _OckNotificationsPrivate {
	/* Properties */
	gboolean timeout_enabled;
	guint    timeout_ms;
	/* Notifications */
	NotifyNotification *notif_station;
	NotifyNotification *notif_metadata;
};

typedef struct _OckNotificationsPrivate OckNotificationsPrivate;

struct _OckNotifications {
	/* Parent instance structure */
	OckFeature               parent_instance;
	/* Private data */
	OckNotificationsPrivate *priv;
};

G_DEFINE_TYPE_WITH_PRIVATE(OckNotifications, ock_notifications, OCK_TYPE_FEATURE)

/*
 * Helpers
 */

static NotifyNotification *
make_notification(gint timeout_ms)
{
	NotifyNotification *notif;

	notif = notify_notification_new("", NULL, ICON);
	notify_notification_set_urgency(notif, NOTIFY_URGENCY_LOW);
	notify_notification_set_timeout(notif, timeout_ms);

	return notif;
}

static gboolean
update_notification_station(NotifyNotification *notif, OckStation *station)
{
	const gchar *str;
	gchar *text;

	if (station == NULL)
		return FALSE;

	str = ock_station_get_name(station);
	if (str) {
		text = g_strdup_printf("Playing %s", str);
	} else {
		str = ock_station_get_uri(station);
		text = g_strdup_printf("Playing <%s>", str);
	}

	notify_notification_update(notif, text, NULL, ICON);

	g_free(text);

	return TRUE;
}

static gboolean
update_notification_metadata(NotifyNotification *notif, OckMetadata *metadata)
{
	const gchar *artist;
	const gchar *title;
	const gchar *album;
	const gchar *year;
	const gchar *genre;
	gchar *album_year;
	gchar *text;

	if (metadata == NULL)
		return FALSE;

	artist = ock_metadata_get_artist(metadata);
	title = ock_metadata_get_title(metadata);
	album = ock_metadata_get_album(metadata);
	year = ock_metadata_get_year(metadata);
	genre = ock_metadata_get_genre(metadata);

	/* If there's only the 'title' field, don't bother being clever,
	 * just display it as it. Actually, most radios fill only this field,
	 * and put everything in (title + artist + some more stuff).
	 */
	if (title && !artist && !album && !year && !genre) {
		notify_notification_update(notif, title, NULL, ICON);
		return TRUE;
	}

	/* Otherwise, each existing field is displayed on a line */
	if (title == NULL)
		title = "(Unknown title)";

	album_year = ock_metadata_make_album_year(metadata, FALSE);
	text = g_strjoin_null("\n", 4, title, artist, album_year, genre);

	notify_notification_update(notif, text, NULL, ICON);

	g_free(text);
	g_free(album_year);

	return TRUE;
}

/*
 * Signal handlers & callbacks
 */

static void
on_player_notify(OckPlayer        *player,
                 GParamSpec       *pspec,
                 OckNotifications *self)
{
	OckNotificationsPrivate *priv = self->priv;
	const gchar *property_name = g_param_spec_get_name(pspec);
	NotifyNotification *notif = NULL;
	gboolean must_notify = FALSE;
	GError *error = NULL;

	TRACE("%p, %s, %p", player, property_name, self);

	/* Check what changed, and create notification if needed */
	if (!g_strcmp0(property_name, "state")) {
		OckPlayerState state;

		notif = priv->notif_station;
		state = ock_player_get_state(player);

		if (state == OCK_PLAYER_STATE_PLAYING) {
			OckStation *station;

			station = ock_player_get_station(player);
			must_notify = update_notification_station(notif, station);
		}
	} else if (!g_strcmp0(property_name, "metadata")) {
		OckMetadata *metadata;

		notif = priv->notif_metadata;
		metadata = ock_player_get_metadata(player);
		must_notify = update_notification_metadata(notif, metadata);
	}

	/* There might be nothing to notify */
	if (notif == NULL || must_notify == FALSE)
		return;

	/* Show notification */
	if (notify_notification_show(notif, &error) != TRUE) {
		CRITICAL("Could not send notification: %s", error->message);
		g_error_free(error);
	}
}

static GSignalHandler player_handlers[] = {
	{ "notify", G_CALLBACK(on_player_notify) },
	{ NULL,     NULL }
};

/*
 * Property accessors
 */

static gboolean
ock_notifications_get_timeout_enabled(OckNotifications *self)
{
	return self->priv->timeout_enabled;
}

static void
ock_notifications_set_timeout_enabled(OckNotifications *self, gboolean enabled)
{
	OckNotificationsPrivate *priv = self->priv;
	gint timeout_ms;

	/* Bail out if needed */
	if (priv->timeout_enabled == enabled)
		return;

	/* Set to existing notifications */
	if (enabled == TRUE)
		timeout_ms = self->priv->timeout_ms;
	else
		timeout_ms = NOTIFY_EXPIRES_DEFAULT;

	if (priv->notif_station)
		notify_notification_set_timeout(priv->notif_station, timeout_ms);

	if (priv->notif_metadata)
		notify_notification_set_timeout(priv->notif_metadata, timeout_ms);

	/* Save and notify */
	priv->timeout_enabled = enabled;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TIMEOUT_ENABLED]);
}

static gint
ock_notifications_get_timeout_seconds(OckNotifications *self)
{
	return self->priv->timeout_ms / 1000;
}

static void
ock_notifications_set_timeout_seconds(OckNotifications *self, guint timeout)
{
	OckNotificationsPrivate *priv = self->priv;

	/* No need to validate the incoming value, this function is private, it's
	 * only invoked by set_property(), where the value was validated already.
	 */

	/* Incoming value in seconds, we want to store it in ms for convenience */
	timeout *= 1000;

	/* Bail out if needed */
	if (priv->timeout_ms == timeout)
		return;

	/* Set to existing notifications */
	if (priv->notif_station && priv->timeout_enabled)
		notify_notification_set_timeout(priv->notif_station, timeout);

	if (priv->notif_metadata && priv->timeout_enabled)
		notify_notification_set_timeout(priv->notif_metadata, timeout);

	/* Save and notify */
	priv->timeout_ms = timeout;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_TIMEOUT_SECONDS]);
}

static void
ock_notifications_get_property(GObject    *object,
                               guint       property_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
	OckNotifications *self = OCK_NOTIFICATIONS(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_TIMEOUT_ENABLED:
		g_value_set_boolean(value, ock_notifications_get_timeout_enabled(self));
		break;
	case PROP_TIMEOUT_SECONDS:
		g_value_set_uint(value, ock_notifications_get_timeout_seconds(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
ock_notifications_set_property(GObject      *object,
                               guint         property_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
	OckNotifications *self = OCK_NOTIFICATIONS(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_TIMEOUT_ENABLED:
		ock_notifications_set_timeout_enabled(self, g_value_get_boolean(value));
		break;
	case PROP_TIMEOUT_SECONDS:
		ock_notifications_set_timeout_seconds(self, g_value_get_uint(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Feature methods
 */

static void
ock_notifications_disable(OckFeature *feature)
{
	OckNotificationsPrivate *priv = OCK_NOTIFICATIONS(feature)->priv;
	OckPlayer *player = ock_core_player;

	/* Signal handlers */
	g_signal_handlers_disconnect_by_data(player, feature);

	/* Unref notifications */
	g_object_unref(priv->notif_station);
	priv->notif_station = NULL;
	g_object_unref(priv->notif_metadata);
	priv->notif_metadata = NULL;

	/* Cleanup libnotify */
	if (notify_is_initted() == TRUE)
		notify_uninit();

	/* Chain up */
	OCK_FEATURE_CHAINUP_DISABLE(ock_notifications, feature);
}

static void
ock_notifications_enable(OckFeature *feature)
{
	OckNotificationsPrivate *priv = OCK_NOTIFICATIONS(feature)->priv;
	OckPlayer *player = ock_core_player;

	/* Chain up */
	OCK_FEATURE_CHAINUP_ENABLE(ock_notifications, feature);

	/* Init libnotify */
	g_assert(notify_is_initted() == FALSE);
	if (notify_init(PACKAGE_CAMEL_NAME) == FALSE)
		CRITICAL("Failed to initialize libnotify");

	/* Create notifications */
	priv->notif_station = make_notification
	                      (priv->timeout_enabled ? (gint) priv->timeout_ms : -1);
	priv->notif_metadata = make_notification
	                       (priv->timeout_enabled ? (gint) priv->timeout_ms : -1);

	/* Signal handlers */
	g_signal_handlers_connect(player, player_handlers, feature);
}

/*
 * GObject methods
 */

static void
ock_notifications_init(OckNotifications *self)
{
	OckNotificationsPrivate *priv;

	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = ock_notifications_get_instance_private(self);

	/* Initialize properties */
	priv = self->priv;
	priv->timeout_enabled = DEFAULT_TIMEOUT_ENABLED;
	priv->timeout_ms = DEFAULT_TIMEOUT_SECONDS * 1000;
}

static void
ock_notifications_class_init(OckNotificationsClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);
	OckFeatureClass *feature_class = OCK_FEATURE_CLASS(class);

	TRACE("%p", class);

	/* Override OckFeature methods */
	feature_class->enable  = ock_notifications_enable;
	feature_class->disable = ock_notifications_disable;

	/* Properties */
	object_class->get_property = ock_notifications_get_property;
	object_class->set_property = ock_notifications_set_property;

	properties[PROP_TIMEOUT_ENABLED] =
	        g_param_spec_boolean("timeout-enabled", "Timeout Enabled",
	                             "Whether to use a custom timeout for the notifications",
	                             DEFAULT_TIMEOUT_ENABLED,
	                             OCK_PARAM_DEFAULT_FLAGS | GSZN_PARAM_SERIALIZE |
	                             G_PARAM_READWRITE);

	properties[PROP_TIMEOUT_SECONDS] =
	        g_param_spec_uint("timeout-seconds", "Timeout in seconds",
	                          "How long the notifications should be displayed",
	                          MIN_TIMEOUT_SECONDS,
	                          MAX_TIMEOUT_SECONDS,
	                          DEFAULT_TIMEOUT_SECONDS,
	                          OCK_PARAM_DEFAULT_FLAGS | GSZN_PARAM_SERIALIZE |
	                          G_PARAM_READWRITE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
