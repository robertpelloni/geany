/* bobguisecurememoryprivate.h - Allocator for non-pageable memory

   Copyright 2007  Stefan Walter
   Copyright 2020  GNOME Foundation

   SPDX-License-Identifier: LGPL-2.0-or-later

   The Gnome Keyring Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Keyring Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   see <http://www.gnu.org/licenses/>.

   Author: Stef Walter <stef@memberwebs.com>
*/

#pragma once

#include <stdlib.h>
#include <glib.h>

/*
 * Main functionality
 *
 * Allocations return NULL on failure.
 */

#define BOBGUI_SECURE_USE_FALLBACK     0x0001

void *          bobgui_secure_alloc_full   (const char *tag,
                                         size_t length,
                                         int options);

void *          bobgui_secure_realloc_full (const char *tag,
                                         void *p,
                                         size_t length,
                                         int options);

void            bobgui_secure_free         (void *p);

void            bobgui_secure_free_full    (void *p,
                                         int fallback);

void            bobgui_secure_clear        (void *p,
                                         size_t length);

int             bobgui_secure_check        (const void *p);

void            bobgui_secure_validate     (void);

char *          bobgui_secure_strdup_full  (const char *tag,
                                         const char *str,
                                         int options);

char *          bobgui_secure_strndup_full (const char *tag,
                                         const char *str,
                                         size_t length,
                                         int options);

void            bobgui_secure_strclear     (char *str);

void            bobgui_secure_strfree      (char *str);

/* Simple wrappers */

static inline void *bobgui_secure_alloc (size_t length) {
  return bobgui_secure_alloc_full ("bobgui", length, BOBGUI_SECURE_USE_FALLBACK);
}

static inline void *bobgui_secure_realloc (void *p, size_t length) {
  return bobgui_secure_realloc_full ("bobgui", p, length, BOBGUI_SECURE_USE_FALLBACK);
}

static inline void *bobgui_secure_strdup (const char *str) {
  return bobgui_secure_strdup_full ("bobgui", str, BOBGUI_SECURE_USE_FALLBACK);
}

static inline void *bobgui_secure_strndup (const char *str, size_t length) {
  return bobgui_secure_strndup_full ("bobgui", str, length, BOBGUI_SECURE_USE_FALLBACK);
}

typedef struct {
  const char *tag;
  size_t request_length;
  size_t block_length;
} bobgui_secure_rec;

bobgui_secure_rec *   bobgui_secure_records    (unsigned int *count);
