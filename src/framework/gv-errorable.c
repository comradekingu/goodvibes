/*
 * Goodvibes Radio Player
 *
 * Copyright (C) 2015-2017 Arnaud Rebillout
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
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

#include <glib-object.h>

#include "framework/log.h"
#include "framework/gv-errorable.h"

/*
 * GObject definitions
 */

enum {
	SIGNAL_ERROR,
	/* Number of signals */
	SIGNAL_N
};

static guint signals[SIGNAL_N];

G_DEFINE_INTERFACE(GvErrorable, gv_errorable, G_TYPE_OBJECT)

/*
 * Public methods
 */

void
gv_errorable_emit_error(GvErrorable *self, const gchar *fmt, ...)
{
	va_list args;
	gchar *string;

	va_start(args, fmt);
	string = g_strdup_vprintf(fmt, args);
	va_end(args);

	g_signal_emit(self, signals[SIGNAL_ERROR], 0, string);
	g_free(string);
}

/*
 * GObject methods
 */

static void
gv_errorable_default_init(GvErrorableInterface *iface)
{
	TRACE("%p", iface);

	signals[SIGNAL_ERROR] =
	        g_signal_new("error", G_TYPE_FROM_INTERFACE(iface),
			     G_SIGNAL_RUN_LAST,
	                     G_STRUCT_OFFSET(GvErrorableInterface, error),
	                     NULL, NULL, NULL,
			     G_TYPE_NONE, 1, G_TYPE_STRING);
}
