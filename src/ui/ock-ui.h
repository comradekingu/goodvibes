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

#ifndef __OVERCOOKED_UI_OCK_UI_H__
#define __OVERCOOKED_UI_OCK_UI_H__

#include <glib.h>

void ock_ui_init     (void);
void ock_ui_cleanup  (void);
void ock_ui_warm_up  (void);
void ock_ui_cool_down(void);

/*
 * Underlying toolkit
 */

GOptionGroup *ock_ui_toolkit_init_get_option_group (void);
const gchar  *ock_ui_toolkit_runtime_version_string(void);
const gchar  *ock_ui_toolkit_compile_version_string(void);

#endif /* __OVERCOOKED_UI_OCK_UI_H__ */