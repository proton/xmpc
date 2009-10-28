/*
 *  Copyright (c) 2008 Mike Massonnet <mmassonnet@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __XFMPC_MENU_H__
#define __XFMPC_MENU_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFMPC_TYPE_MENU                (xfmpc_menu_get_type())

#define XFMPC_MENU(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFMPC_TYPE_MENU, XfmpcMenu))
#define XFMPC_MENU_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), XFMPC_TYPE_MENU, XfmpcMenuClass))

#define XFMPC_IS_MENU(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFMPC_TYPE_MENU))
#define XFMPC_IS_MENU_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), XFMPC_TYPE_MENU))

#define XFMPC_MENU_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), XFMPC_TYPE_MENU, XfmpcMenuClass))

typedef struct _XfmpcMenuClass         XfmpcMenuClass;
typedef struct _XfmpcMenu              XfmpcMenu;
typedef struct _XfmpcMenuPrivate       XfmpcMenuPrivate;

GType                   xfmpc_menu_get_type                () G_GNUC_CONST;

XfmpcMenu *             xfmpc_menu_get                     ();
void                    xfmpc_menu_add                     (XfmpcMenu *menu, GtkWidget *pmenu);

G_END_DECLS

#endif

