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

#ifndef __OVERCOOKED_UI_FEAT_OCK_HOTKEYS_H__
#define __OVERCOOKED_UI_FEAT_OCK_HOTKEYS_H__

#include <glib-object.h>

#include "framework/ock-feature.h"

/* GObject declarations */

#define OCK_TYPE_HOTKEYS ock_hotkeys_get_type()

G_DECLARE_FINAL_TYPE(OckHotkeys, ock_hotkeys, OCK, HOTKEYS, OckFeature)

#endif /* __OVERCOOKED_UI_FEAT_OCK_HOTKEYS_H__ */
