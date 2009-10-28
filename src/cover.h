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

#ifndef __XFMPC_COVER_H__
#define __XFMPC_COVER_H__

#include <glib-object.h>

G_BEGIN_DECLS

#define XFMPC_TYPE_SSIGNAL                (xfmpc_ssignal_get_type())

#define XFMPC_SSIGNAL(obj)                (G_TYPE_CHECK_INSTANCE_CAST ((obj), XFMPC_TYPE_SSIGNAL, XfmpcSsignal))
#define XFMPC_SSIGNAL_CLASS(klass)        (G_TYPE_CHECK_CLASS_CAST ((klass), XFMPC_TYPE_SSIGNAL, XfmpcSsignalClass))

#define XFMPC_IS_SSIGNAL(obj)             (G_TYPE_CHECK_INSTANCE_TYPE ((obj), XFMPC_TYPE_SSIGNAL))
#define XFMPC_IS_SSIGNAL_CLASS(klass)     (G_TYPE_CHECK_CLASS_TYPE ((klass), XFMPC_TYPE_SSIGNAL))

#define XFMPC_SSIGNAL_GET_CLASS(obj)      (G_TYPE_INSTANCE_GET_CLASS ((obj), XFMPC_TYPE_SSIGNAL, XfmpcSsignalClass))

typedef struct _XfmpcSsignalClass         XfmpcSsignalClass;
typedef struct _XfmpcSsignal              XfmpcSsignal;
typedef struct _XfmpcSsignalPrivate       XfmpcSsignalPrivate;

GType                   xfmpc_ssignal_get_type                () G_GNUC_CONST;

XfmpcSsignal *          xfmpc_ssignal_get                     ();

const GdkPixbuf*        xfmpc_cover_get_picture               (XfmpcSsignal *cover, gint width);

G_END_DECLS

#endif

