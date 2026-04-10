/* bobguitexttag.c - text tag object
 *
 * Copyright (c) 1992-1994 The Regents of the University of California.
 * Copyright (c) 1994-1997 Sun Microsystems, Inc.
 * Copyright (c) 2000      Red Hat, Inc.
 * Tk -> Bobgui port by Havoc Pennington <hp@redhat.com>
 *
 * This software is copyrighted by the Regents of the University of
 * California, Sun Microsystems, Inc., and other parties.  The
 * following terms apply to all files associated with the software
 * unless explicitly disclaimed in individual files.
 *
 * The authors hereby grant permission to use, copy, modify,
 * distribute, and license this software and its documentation for any
 * purpose, provided that existing copyright notices are retained in
 * all copies and that this notice is included verbatim in any
 * distributions. No written agreement, license, or royalty fee is
 * required for any of the authorized uses.  Modifications to this
 * software may be copyrighted by their authors and need not follow
 * the licensing terms described here, provided that the new terms are
 * clearly indicated on the first page of each file where they apply.
 *
 * IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY
 * PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
 * DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION,
 * OR ANY DERIVATIVES THEREOF, EVEN IF THE AUTHORS HAVE BEEN ADVISED
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, AND
 * NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS,
 * AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
 * MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 *
 * GOVERNMENT USE: If you are acquiring this software on behalf of the
 * U.S. government, the Government shall have only "Restricted Rights"
 * in the software and related documentation as defined in the Federal
 * Acquisition Regulations (FARs) in Clause 52.227.19 (c) (2).  If you
 * are acquiring the software on behalf of the Department of Defense,
 * the software shall be classified as "Commercial Computer Software"
 * and the Government shall have only "Restricted Rights" as defined
 * in Clause 252.227-7013 (c) (1) of DFARs.  Notwithstanding the
 * foregoing, the authors grant the U.S. Government and others acting
 * in its behalf permission to use and distribute the software in
 * accordance with the terms specified in this license.
 *
 */

#pragma once


#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguienums.h>


G_BEGIN_DECLS

typedef struct _BobguiTextIter BobguiTextIter;
typedef struct _BobguiTextTagTable BobguiTextTagTable;

#define BOBGUI_TYPE_TEXT_TAG            (bobgui_text_tag_get_type ())
#define BOBGUI_TEXT_TAG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TEXT_TAG, BobguiTextTag))
#define BOBGUI_TEXT_TAG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TEXT_TAG, BobguiTextTagClass))
#define BOBGUI_IS_TEXT_TAG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TEXT_TAG))
#define BOBGUI_IS_TEXT_TAG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TEXT_TAG))
#define BOBGUI_TEXT_TAG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TEXT_TAG, BobguiTextTagClass))

typedef struct _BobguiTextTag             BobguiTextTag;
typedef struct _BobguiTextTagPrivate      BobguiTextTagPrivate;
typedef struct _BobguiTextTagClass        BobguiTextTagClass;

struct _BobguiTextTag
{
  GObject parent_instance;

  BobguiTextTagPrivate *priv;
};

struct _BobguiTextTagClass
{
  GObjectClass parent_class;

  /*< private >*/

  gpointer padding[8];
};

GDK_AVAILABLE_IN_ALL
GType        bobgui_text_tag_get_type     (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiTextTag  *bobgui_text_tag_new          (const char        *name);
GDK_AVAILABLE_IN_ALL
int          bobgui_text_tag_get_priority (BobguiTextTag        *tag);
GDK_AVAILABLE_IN_ALL
void         bobgui_text_tag_set_priority (BobguiTextTag        *tag,
                                        int                priority);
GDK_AVAILABLE_IN_ALL
void         bobgui_text_tag_changed      (BobguiTextTag        *tag,
                                        gboolean           size_changed);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTextTag, g_object_unref)

G_END_DECLS


