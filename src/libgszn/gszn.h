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

#ifndef __OVERCOOKED_LIBGSZN_GSZN_H__
#define __OVERCOOKED_LIBGSZN_GSZN_H__

#include "libgszn/gszn-backend.h"
#include "libgszn/gszn-backend-xml.h"
#include "libgszn/gszn-backend-keyfile.h"
#include "libgszn/gszn-param-specs.h"
#include "libgszn/gszn-serializer.h"
#include "libgszn/gszn-deserializer.h"

/* Functions */

void gszn_init(void);
void gszn_cleanup(void);

#endif /* __OVERCOOKED_LIBGSZN_GSZN_H__ */
