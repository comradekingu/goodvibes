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
#include <gst/gst.h>
#include <gst/audio/streamvolume.h>

#include "additions/glib-object.h"
#include "additions/gst.h"

#include "framework/log.h"
#include "framework/ock-framework.h"

#include "core/ock-engine.h"
#include "core/ock-core-enum-types.h"
#include "core/ock-metadata.h"

/* Uncomment to dump more stuff from GStreamer */

//#define DUMP_TAGS
//#define DUMP_GST_STATE_CHANGES

/*
 * Properties
 */

#define DEFAULT_VOLUME 1.0
#define DEFAULT_MUTE   FALSE

enum {
	/* Reserved */
	PROP_0,
	/* Properties - refer to class_init() for more details */
	PROP_STATE,
	PROP_VOLUME,
	PROP_MUTE,
	PROP_STREAM_URI,
	PROP_METADATA,
	/* Number of properties */
	PROP_N
};

static GParamSpec *properties[PROP_N];

/*
 * GObject definitions
 */

struct _OckEnginePrivate {
	/* GStreamer stuff */
	GstElement     *playbin;
	GstBus         *bus;
	/* Properties */
	OckEngineState  state;
	gdouble         volume;
	gboolean        mute;
	gchar          *stream_uri;
	OckMetadata    *metadata;
};

typedef struct _OckEnginePrivate OckEnginePrivate;

struct _OckEngine {
	/* Parent instance structure */
	GObject           parent_instance;
	/* Private data */
	OckEnginePrivate *priv;
};

static void ock_engine_errorable_interface_init(OckErrorableInterface *iface G_GNUC_UNUSED) {}

G_DEFINE_TYPE_WITH_CODE(OckEngine, ock_engine, G_TYPE_OBJECT,
                        G_ADD_PRIVATE(OckEngine)
                        G_IMPLEMENT_INTERFACE(OCK_TYPE_ERRORABLE,
                                        ock_engine_errorable_interface_init))

/*
 * GStreamer helpers
 */

static void
set_gst_state(GstElement *playbin, GstState state)
{
	const gchar *state_name = gst_element_state_get_name(state);
	GstStateChangeReturn change_return;

	change_return = gst_element_set_state(playbin, state);

	switch (change_return) {
	case GST_STATE_CHANGE_SUCCESS:
		DEBUG("Setting gst state '%s'... success", state_name);
		break;
	case GST_STATE_CHANGE_ASYNC:
		DEBUG("Setting gst state '%s'... will change async", state_name);
		break;
	case GST_STATE_CHANGE_FAILURE:
		/* This might happen if the uri is invalid */
		DEBUG("Setting gst state '%s'... failed !", state_name);
		break;
	case GST_STATE_CHANGE_NO_PREROLL:
		DEBUG("Setting gst state '%s'... no preroll", state_name);
		break;
	default:
		WARNING("Unhandled state change: %d", change_return);
		break;
	}
}

#if 0
static GstState
get_gst_state(GstElement *playbin)
{
	GstStateChangeReturn change_return;
	GstState state, pending;

	change_return = gst_element_get_state(playbin, &state, &pending, 100 * GST_MSECOND);

	switch (change_return) {
	case GST_STATE_CHANGE_SUCCESS:
		DEBUG("Getting gst state... success");
		break;
	case GST_STATE_CHANGE_ASYNC:
		DEBUG("Getting gst state... will change async");
		break;
	case GST_STATE_CHANGE_FAILURE:
		DEBUG("Getting gst state... failed !");
		break;
	case GST_STATE_CHANGE_NO_PREROLL:
		DEBUG("Getting gst state... no preroll");
		break;
	default:
		WARNING("Unhandled state change: %d", change_return);
		break;
	}

	DEBUG("Gst state '%s', pending '%s'",
	      gst_element_state_get_name(state),
	      gst_element_state_get_name(pending));

	return state;
}
#endif

static OckMetadata *
taglist_to_metadata(GstTagList *taglist)
{
	OckMetadata *metadata;
	const gchar *artist = NULL;
	const gchar *title = NULL;
	const gchar *album = NULL;
	const gchar *genre = NULL;
	const gchar *comment = NULL;
	GDate *date = NULL;
	gchar *year = NULL;
	guint  bitrate = 0;

	/* Get info from taglist */
	gst_tag_list_peek_string_index(taglist, GST_TAG_ARTIST, 0, &artist);
	gst_tag_list_peek_string_index(taglist, GST_TAG_TITLE, 0, &title);
	gst_tag_list_peek_string_index(taglist, GST_TAG_ALBUM, 0, &album);
	gst_tag_list_peek_string_index(taglist, GST_TAG_GENRE, 0, &genre);
	gst_tag_list_peek_string_index(taglist, GST_TAG_COMMENT, 0, &comment);
	gst_tag_list_get_uint_index   (taglist, GST_TAG_BITRATE, 0, &bitrate);
	gst_tag_list_get_date_index   (taglist, GST_TAG_DATE, 0, &date);
	if (date && g_date_valid(date))
		year = g_strdup_printf("%d", g_date_get_year(date));

	/* Create new metadata object */
	metadata = g_object_new(OCK_TYPE_METADATA,
	                        "artist", artist,
	                        "title", title,
	                        "album", album,
	                        "genre", genre,
	                        "year", year,
	                        "comment", comment,
	                        "bitrate", &bitrate,
	                        NULL);

	/* Freedom for the braves */
	g_free(year);
	if (date)
		g_date_free(date);

	return metadata;
}

/*
 * Property accessors
 */

OckEngineState
ock_engine_get_state(OckEngine *self)
{
	return self->priv->state;
}

static void
ock_engine_set_state(OckEngine *self, OckEngineState state)
{
	OckEnginePrivate *priv = self->priv;

	if (priv->state == state)
		return;

	priv->state = state;
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STATE]);
}

gdouble
ock_engine_get_volume(OckEngine *self)
{
	return self->priv->volume;
}

void
ock_engine_set_volume(OckEngine *self, gdouble volume)
{
	OckEnginePrivate *priv = self->priv;

	volume = volume > 1.0 ? 1.0 :
	         volume < 0.0 ? 0.0 : volume;

	if (priv->volume == volume)
		return;

	priv->volume = volume;
	gst_stream_volume_set_volume(GST_STREAM_VOLUME(priv->playbin),
	                             GST_STREAM_VOLUME_FORMAT_CUBIC, volume);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_VOLUME]);
}

gboolean
ock_engine_get_mute(OckEngine *self)
{
	return self->priv->mute;
}

void
ock_engine_set_mute(OckEngine *self, gboolean mute)
{
	OckEnginePrivate *priv = self->priv;

	if (priv->mute == mute)
		return;

	priv->mute = mute;
	gst_stream_volume_set_mute(GST_STREAM_VOLUME(priv->playbin), mute);
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_MUTE]);
}

OckMetadata *
ock_engine_get_metadata(OckEngine *self)
{
	return self->priv->metadata;
}

static void
ock_engine_set_metadata(OckEngine *self, OckMetadata *metadata)
{
	OckEnginePrivate *priv = self->priv;

	/* Compare content */
	if (priv->metadata && metadata &&
	    ock_metadata_is_equal(priv->metadata, metadata)) {
		DEBUG("Metadata identical, ignoring...");
		return;
	}

	/* Assign */
	if (g_set_object(&priv->metadata, metadata))
		g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_METADATA]);
}

const gchar *
ock_engine_get_stream_uri(OckEngine *self)
{
	return self->priv->stream_uri;
}

static void
ock_engine_set_stream_uri(OckEngine *self, const gchar *uri)
{
	OckEnginePrivate *priv = self->priv;

	/* Do nothing in case this stream is already set */
	if (!g_strcmp0(priv->stream_uri, uri))
		return;

	/* Set new stream uri */
	g_free(priv->stream_uri);
	priv->stream_uri = g_strdup(uri);

	/* Notify */
	g_object_notify_by_pspec(G_OBJECT(self), properties[PROP_STREAM_URI]);

	/* Care for a little bit of debug ? */
	DEBUG("Stream uri set to '%s'", uri);
}

static void
ock_engine_get_property(GObject    *object,
                        guint       property_id,
                        GValue     *value,
                        GParamSpec *pspec)
{
	OckEngine *self = OCK_ENGINE(object);

	TRACE_GET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_STATE:
		g_value_set_enum(value, ock_engine_get_state(self));
		break;
	case PROP_VOLUME:
		g_value_set_double(value, ock_engine_get_volume(self));
		break;
	case PROP_MUTE:
		g_value_set_boolean(value, ock_engine_get_mute(self));
		break;
	case PROP_STREAM_URI:
		g_value_set_string(value, ock_engine_get_stream_uri(self));
		break;
	case PROP_METADATA:
		g_value_set_object(value, ock_engine_get_metadata(self));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void
ock_engine_set_property(GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	OckEngine *self = OCK_ENGINE(object);

	TRACE_SET_PROPERTY(object, property_id, value, pspec);

	switch (property_id) {
	case PROP_VOLUME:
		ock_engine_set_volume(self, g_value_get_double(value));
		break;
	case PROP_MUTE:
		ock_engine_set_mute(self, g_value_get_boolean(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

/*
 * Public methods
 */

void
ock_engine_play(OckEngine *self, const gchar *uri)
{
	OckEnginePrivate *priv = self->priv;

	/* Ensure there's an uri */
	if (uri == NULL) {
		WARNING("Uri is NULL !");
		return;
	}

	/* Set the uri */
	ock_engine_set_stream_uri(self, uri);

	/* According to the doc:
	 *
	 * > State changes to GST_STATE_READY or GST_STATE_NULL never return
	 * > GST_STATE_CHANGE_ASYNC.
	 *
	 * https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/
	 * GstElement.html#gst-element-set-state
	 */

	/* Ensure playback is stopped */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Clear metadata */
	ock_engine_set_metadata(self, NULL);

	/* Set the stream uri */
	g_object_set(priv->playbin, "uri", priv->stream_uri, NULL);

	/* Go to the ready stop (not sure it's needed) */
	set_gst_state(priv->playbin, GST_STATE_READY);

	/* Set gst state to PAUSE, so that the playbin starts buffering data.
	 * Playback will start as soon as buffering is finished.
	 */
	set_gst_state(priv->playbin, GST_STATE_PAUSED);
	ock_engine_set_state(self, OCK_ENGINE_STATE_CONNECTING);
}

void
ock_engine_stop(OckEngine *self)
{
	OckEnginePrivate *priv = self->priv;

	/* Radical way to stop: set state to NULL */
	set_gst_state(priv->playbin, GST_STATE_NULL);
	ock_engine_set_state(self, OCK_ENGINE_STATE_STOPPED);
}

OckEngine *
ock_engine_new(void)
{
	return g_object_new(OCK_TYPE_ENGINE, NULL);
}

/*
 * Gst bus signal handlers
 */

static gboolean
on_bus_message_eos(GstBus *bus G_GNUC_UNUSED, GstMessage *msg G_GNUC_UNUSED, OckEngine *self)
{
	/* This shouldn't happen, as far as I know */
	WARNING("Unexpected eos message");

	/* Emit an error */
	ock_errorable_emit_error(OCK_ERRORABLE(self), "End of stream");

	return TRUE;
}

static gboolean
on_bus_message_error(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, OckEngine *self)
{
	GError *error;
	gchar  *debug;

	/* Parse message */
	gst_message_parse_error(msg, &error, &debug);

	/* Display error */
	DEBUG("Gst bus error msg: %s:%d: %s",
	      g_quark_to_string(error->domain), error->code, error->message);
	DEBUG("Gst bus error debug : %s", debug);

	/* Emit an error signal */
	ock_errorable_emit_error(OCK_ERRORABLE(self), error->message);

	/* Cleanup */
	g_error_free(error);
	g_free(debug);

	return TRUE;

	/* Here are some gst error messages that I've dealt with so far.
	 *
	 * GST_RESOURCE_ERROR: GST_RESOURCE_ERROR_NOT_FOUND
	 *
	 * Could not resolve server name.
	 *
	 *   This might happen for different reasons:
	 *   - wrong url, the protocol (http, likely) is correct, but the name of
	 *     the server is wrong.
	 *   - name resolution is down.
	 *   - network is down.
	 *
	 *   To reproduce:
	 *   - in the url, set a wrong server name.
	 *   - just disconnect from the network.
	 *
	 * File Not Found
	 *
	 *   It means that the protocol (http, lilely) is correct, the server
	 *   name as well, but the file is wrong or couldn't be found. I guess
	 *   the server could be reached and replied something like "file not found".
	 *
	 *   To reproduce: at the end of the url, set a wrong filename
	 *
	 * Not Available
	 *
	 *   Don't remember about this one...
	 *
	 * GST_CORE_ERROR: GST_CORE_ERROR_MISSING_PLUGIN
	 *
	 * No URI handler implemented for ...
	 *
	 *   It means that the protocol is wrong or unsupported.
	 *
	 *   To reproduce: in the url, replace 'http' by some random stuff
	 *
	 * Your GStreamer installation is missing a plug-in.
	 *
	 *   It means that gstreamer doesn't know what to do with what it received
	 *   from the server. So it's likely that the server didn't send the stream
	 *   expected. And it's even more likely that the server returned an html
	 *   page, because that's what servers usually do.
	 *   Some cases where it can happen:
	 *   - wrong url, that exists but is not a stream.
	 *   - outgoing requests are blocked, and return an html page. For example,
	 *     you're connected to a public Wi-Fi that expects you to authenticate
	 *     or agree the terms of use, before allowing outgoing traffic.
	 *
	 *   To reproduce: in the url, remove anything after the server name
	 */
}

static gboolean
on_bus_message_warning(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, OckEngine *self G_GNUC_UNUSED)
{
	GError *warning;
	gchar  *debug;

	/* Parse message */
	gst_message_parse_warning(msg, &warning, &debug);

	/* Display warning */
	DEBUG("Gst bus warning msg: %s:%d: %s",
	      g_quark_to_string(warning->domain), warning->code, warning->message);
	DEBUG("Gst bus warning debug : %s", debug);

	/* Cleanup */
	g_error_free(warning);
	g_free(debug);

	return TRUE;
}

static gboolean
on_bus_message_info(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, OckEngine *self G_GNUC_UNUSED)
{
	GError *info;
	gchar  *debug;

	/* Parse message */
	gst_message_parse_info(msg, &info, &debug);

	/* Display info */
	DEBUG("Gst bus info msg: %s:%d: %s",
	      g_quark_to_string(info->domain), info->code, info->message);
	DEBUG("Gst bus info debug : %s", debug);

	/* Cleanup */
	g_error_free(info);
	g_free(debug);

	return TRUE;
}

#ifdef DUMP_TAGS
static void
tag_list_foreach_dump(const GstTagList *list, const gchar *tag,
                      gpointer data G_GNUC_UNUSED)
{
	gchar *str = NULL;

	switch (gst_tag_get_type(tag)) {
	case G_TYPE_STRING:
		gst_tag_list_get_string_index(list, tag, 0, &str);
		break;
	case G_TYPE_INT: {
		gint val;
		gst_tag_list_get_int_index(list, tag, 0, &val);
		str = g_strdup_printf("%d", val);
		break;
	}
	case G_TYPE_UINT: {
		guint val;
		gst_tag_list_get_uint_index(list, tag, 0, &val);
		str = g_strdup_printf("%u", val);
		break;
	}
	case G_TYPE_DOUBLE: {
		gdouble val;
		gst_tag_list_get_double_index(list, tag, 0, &val);
		str = g_strdup_printf("%lg", val);
		break;
	}
	case G_TYPE_BOOLEAN: {
		gboolean val;
		gst_tag_list_get_boolean_index(list, tag, 0, &val);
		str = g_strdup_printf("%s", val ? "true" : "false");
		break;
	}
	}

	DEBUG("%10s '%20s' (%d elem): %s",
	      g_type_name(gst_tag_get_type(tag)),
	      tag,
	      gst_tag_list_get_tag_size(list, tag),
	      str);
	g_free(str);

}
#endif /* DUMP_TAGS */

static gboolean
on_bus_message_tag(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, OckEngine *self)
{
	OckMetadata *metadata;
	GstTagList *taglist = NULL;
	const gchar *tag_title = NULL;

	TRACE("... %s, %p", GST_OBJECT_NAME(msg->src), self);

	/* Parse message */
	gst_message_parse_tag(msg, &taglist);

#ifdef DUMP_TAGS
	/* Dumping may be needed to debug */
	DEBUG("-- Dumping taglist...");
	gst_tag_list_foreach(taglist, (GstTagForeachFunc) tag_list_foreach_dump, NULL);
	DEBUG("-- Done --");
#endif /* DUMP_TAGS */

	/* Tags can be quite noisy, so let's cut it short.
	 * From my experience, 'title' is the most important field,
	 * and it's likely that it's the only one filled, containing
	 * everything (title, artist and more).
	 * So, we require this field to be filled. If it's not, this
	 * metadata is considered to be noise, and discarded.
	 */
	gst_tag_list_peek_string_index(taglist, GST_TAG_TITLE, 0, &tag_title);
	if (tag_title == NULL) {
		DEBUG("No 'title' field in the tag list, discarding");
		goto taglist_unref;
	}

	/* Turn taglist into metadata and assign it */
	metadata = taglist_to_metadata(taglist);
	ock_engine_set_metadata(self, metadata);
	g_object_unref(metadata);

taglist_unref:
	/* Unref taglist */
	gst_tag_list_unref(taglist);

	return TRUE;
}

static gboolean
on_bus_message_buffering(GstBus *bus G_GNUC_UNUSED, GstMessage *msg, OckEngine *self)
{
	OckEnginePrivate *priv = self->priv;
	static gint prev_percent = 0;
	gint percent = 0;

	/* Handle the buffering message. Some documentation:
	 * https://gstreamer.freedesktop.org/data/doc/gstreamer/head/gstreamer/html/
	 * GstMessage.html#gst-message-new-buffering
	 */

	/* Parse message */
	gst_message_parse_buffering(msg, &percent);

	/* Display buffering steps 20 by 20 */
	if (ABS(percent - prev_percent) > 20) {
		prev_percent = percent;
		DEBUG("Buffering (%3u %%)", percent);
	}

	/* Now, let's react according to our current state */
	switch (priv->state) {
	case OCK_ENGINE_STATE_STOPPED:
		/* This shouldn't happen */
		WARNING("Received 'bus buffering' while stopped");
		break;

	case OCK_ENGINE_STATE_CONNECTING:
		/* We successfully connected ! */
		ock_engine_set_state(self, OCK_ENGINE_STATE_BUFFERING);

	/* NO BREAK HERE !
	 * This is to handle the (very special) case where the first
	 * buffering message received is already 100%. I'm not sure this
	 * can happen, but it doesn't hurt to be ready.
	 */

	case OCK_ENGINE_STATE_BUFFERING:
		/* When buffering complete, start playing */
		if (percent >= 100) {
			DEBUG("Buffering complete, starting playback");
			set_gst_state(priv->playbin, GST_STATE_PLAYING);
			ock_engine_set_state(self, OCK_ENGINE_STATE_PLAYING);
		}
		break;

	case OCK_ENGINE_STATE_PLAYING:
		/* In case buffering is < 100%, according to the documentation,
		 * we should pause. However, I observed a radio for which I
		 * constantly receive buffering < 100% messages. In such case,
		 * pausing/playing screws the playback. Ignoring the messages
		 * works just fine. So let's ignore for now.
		 * In case it matters, I observed that with radio Nova.
		 */
		if (percent < 100) {
			DEBUG("Buffering < 100%%, ignoring instead of setting to pause");
			//set_gst_state(priv->playbin, GST_STATE_PAUSED);
			//ock_engine_set_state(self, OCK_ENGINE_STATE_BUFFERING);
		}
		break;

	default:
		WARNING("Unhandled engine state %d", priv->state);
	}

	return TRUE;
}

static gboolean
on_bus_message_state_changed(GstBus *bus G_GNUC_UNUSED, GstMessage *msg,
                             OckEngine *self G_GNUC_UNUSED)
{
#ifdef DUMP_GST_STATE_CHANGES
	GstState old, new, pending;

	/* Parse message */
	gst_message_parse_state_changed(msg, &old, &new, &pending);

	/* Just used for debug */
	DEBUG("Gst state changed: old: %s, new: %s, pending: %s",
	      gst_element_state_get_name(old),
	      gst_element_state_get_name(new),
	      gst_element_state_get_name(pending));
#else
	(void) msg;
#endif

	return TRUE;
}

/*
 * GObject methods
 */

static void
ock_engine_finalize(GObject *object)
{
	OckEngine *self = OCK_ENGINE(object);
	OckEnginePrivate *priv = self->priv;

	TRACE("%p", object);

	/* Stop playback at first */
	set_gst_state(priv->playbin, GST_STATE_NULL);

	/* Unref metadata */
	if (priv->metadata)
		g_object_unref(priv->metadata);

	/* Free stream uri */
	g_free(priv->stream_uri);

	/* Unref the bus */
	g_signal_handlers_disconnect_by_data(priv->bus, self);
	gst_bus_remove_signal_watch(priv->bus);
	g_object_unref(priv->bus);

	/* Unref the playbin */
	g_object_unref(priv->playbin);

	/* Chain up */
	G_OBJECT_CHAINUP_FINALIZE(ock_engine, object);
}

static void
ock_engine_constructed(GObject *object)
{
	OckEngine *self = OCK_ENGINE(object);
	OckEnginePrivate *priv = self->priv;
	GstElement *playbin;
	GstElement *fakesink;
	GstBus *bus;

	/* Initialize properties */
	priv->volume = DEFAULT_VOLUME;
	priv->mute   = DEFAULT_MUTE;

	/* Gstreamer must be initialized, let's check that */
	g_assert(gst_is_initialized());

	/* Make the playbin - returns floating ref */
	playbin = gst_element_factory_make("playbin", "playbin");
	g_assert(playbin != NULL);
	priv->playbin = g_object_ref_sink(playbin);

	/* Disable video - returns floating ref */
	fakesink = gst_element_factory_make("fakesink", "fakesink");
	g_assert(fakesink != NULL);
	g_object_set(playbin, "video-sink", fakesink, NULL);

	/* Get a reference to the message bus - returns full ref */
	bus = gst_element_get_bus(playbin);
	g_assert(bus != NULL);
	priv->bus = bus;

	/* Add a bus signal watch (so that 'message' signals are emitted) */
	gst_bus_add_signal_watch(bus);

	/* Connect to signals from the bus */
	g_signal_connect(bus, "message::eos", G_CALLBACK(on_bus_message_eos), self);
	g_signal_connect(bus, "message::error", G_CALLBACK(on_bus_message_error), self);
	g_signal_connect(bus, "message::warning", G_CALLBACK(on_bus_message_warning), self);
	g_signal_connect(bus, "message::info", G_CALLBACK(on_bus_message_info), self);
	g_signal_connect(bus, "message::tag", G_CALLBACK(on_bus_message_tag), self);
	g_signal_connect(bus, "message::buffering", G_CALLBACK(on_bus_message_buffering), self);
	g_signal_connect(bus, "message::state-changed", G_CALLBACK(on_bus_message_state_changed),
	                 self);

	/* Chain up */
	G_OBJECT_CHAINUP_CONSTRUCTED(ock_engine, object);
}

static void
ock_engine_init(OckEngine *self)
{
	TRACE("%p", self);

	/* Initialize private pointer */
	self->priv = ock_engine_get_instance_private(self);
}

static void
ock_engine_class_init(OckEngineClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS(class);

	TRACE("%p", class);

	/* Override GObject methods */
	object_class->finalize = ock_engine_finalize;
	object_class->constructed = ock_engine_constructed;

	/* Properties */
	object_class->get_property = ock_engine_get_property;
	object_class->set_property = ock_engine_set_property;

	properties[PROP_STATE] =
	        g_param_spec_enum("state", "Playback state", NULL,
	                          OCK_ENGINE_STATE_ENUM_TYPE,
	                          OCK_ENGINE_STATE_STOPPED,
	                          OCK_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_VOLUME] =
	        g_param_spec_double("volume", "Volume", NULL,
	                            0.0, 1.0, DEFAULT_VOLUME,
	                            OCK_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_MUTE] =
	        g_param_spec_boolean("mute", "Mute", NULL,
	                             FALSE,
	                             OCK_PARAM_DEFAULT_FLAGS | G_PARAM_READWRITE);

	properties[PROP_STREAM_URI] =
	        g_param_spec_string("stream-uri", "Stream uri", NULL, NULL,
	                            OCK_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	properties[PROP_METADATA] =
	        g_param_spec_object("metadata", "Current metadata", NULL,
	                            OCK_TYPE_METADATA,
	                            OCK_PARAM_DEFAULT_FLAGS | G_PARAM_READABLE);

	g_object_class_install_properties(object_class, PROP_N, properties);
}
