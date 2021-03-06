/*
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
 *  Copyright (c) 2009 Vincent Legout <vincent@xfce.org>
 *  Copyright (c) 2009 Peter Savichev <psavichev@gmail.com>
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define DATADIR "/usr/share"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "cover.h"
#include "mpdclient.h"
#include "preferences.h"

static guint cover_changed_signal;

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_SSIGNAL, XfmpcSsignalPrivate))

static void             xfmpc_ssignal_class_init              (XfmpcSsignalClass *klass);
static void             xfmpc_ssignal_init                    (XfmpcSsignal *ssignal);
static void             xfmpc_ssignal_finalize                (GObject *object);

static void             cb_song_changed                     (XfmpcSsignal *signal);
static void             cb_stopped                          (XfmpcSsignal *signal);
static void             cb_pp_changed                       (XfmpcSsignal *signal,
																														gboolean is_playing);
GdkPixbuf*        xfmpc_cover_get_default             ();

struct _XfmpcSsignalClass
{
	GObjectClass              parent_class;

	void (*connected)         (XfmpcSsignal *ssignal, gpointer user_data);
};

struct _XfmpcSsignal
{
	GObject                   parent;
	XfmpcMpdclient           *mpdclient;
	XfmpcPreferences         *preferences;
	XfmpcSongInfo            *songinfo;
	/*<private>*/
	XfmpcSsignalPrivate    *priv;
};

struct _XfmpcSsignalPrivate
{
	gchar *dir;
	gchar *cover;
	GdkPixbuf *img;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_ssignal_get_type ()
{
	static GType xfmpc_ssignal_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_ssignal_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_ssignal_info =
				{
					sizeof (XfmpcSsignalClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_ssignal_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcSsignal),
					0,
					(GInstanceInitFunc) xfmpc_ssignal_init,
					NULL
				};
			xfmpc_ssignal_type = g_type_register_static (G_TYPE_OBJECT, "XfmpcSsignal", &xfmpc_ssignal_info, 0);
		}

	return xfmpc_ssignal_type;
}

static void
xfmpc_ssignal_class_init (XfmpcSsignalClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcSsignalPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = xfmpc_ssignal_finalize;

	cover_changed_signal = g_signal_new ("cover-update", G_TYPE_FROM_CLASS (klass),
																												G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
																												0, NULL, NULL,
																												g_cclosure_marshal_VOID__VOID,
																												G_TYPE_NONE,
																												0);
}

GdkPixbuf*
xfmpc_cover_get_default ()
{
	return gdk_pixbuf_new_from_file (g_build_filename (DATADIR, "pixmaps", "sonatacd_large.png", NULL), NULL);
}

static void
xfmpc_ssignal_init (XfmpcSsignal *ssignal)
{
	XfmpcSsignalPrivate *priv = ssignal->priv = GET_PRIVATE (ssignal);
	ssignal->preferences = xfmpc_preferences_get ();
	ssignal->mpdclient = xfmpc_mpdclient_get ();

	g_object_get (G_OBJECT (ssignal->preferences),
								"cover-filename", &priv->cover,
								NULL);

	g_object_get (G_OBJECT (ssignal->preferences),
								"library-dir", &priv->dir,
								NULL);

	/* Image */
	priv->img = xfmpc_cover_get_default();

	/* Signals */
	g_signal_connect_swapped (ssignal->mpdclient, "song-changed", G_CALLBACK (cb_song_changed), ssignal);
	g_signal_connect_swapped (ssignal->mpdclient, "stopped", G_CALLBACK (cb_stopped), ssignal);
	g_signal_connect_swapped (ssignal->mpdclient, "pp-changed", G_CALLBACK (cb_pp_changed), ssignal);
}

static void
xfmpc_ssignal_finalize (GObject *object)
{
	//XfmpcSsignal *ssignal = XFMPC_SSIGNAL (object);
	//XfmpcSsignalPrivate *priv = XFMPC_SSIGNAL (ssignal)->priv;

	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

XfmpcSsignal *
xfmpc_ssignal_get ()
{
	static XfmpcSsignal *cover = NULL;

	if (G_UNLIKELY (NULL == cover))
	{
		cover = g_object_new (XFMPC_TYPE_SSIGNAL, NULL);
		g_object_add_weak_pointer (G_OBJECT (cover), (gpointer)&cover);
	}
	else g_object_ref (G_OBJECT (cover));

	return cover;
}

static void
cb_song_changed (XfmpcSsignal *cover)
{
	XfmpcSsignalPrivate *priv = XFMPC_SSIGNAL (cover)->priv;
	const gchar* fn = xfmpc_mpdclient_get_file(cover->mpdclient);
	gchar* dir = g_path_get_dirname(fn);
	gchar* cfn = g_build_filename (priv->dir, dir, priv->cover, NULL);
	priv->img = gdk_pixbuf_new_from_file (cfn, NULL);
	if(!priv->img) priv->img = xfmpc_cover_get_default();

	g_free(dir);
	g_free(cfn);
	g_signal_emit (cover, cover_changed_signal, 0);
}

static void
cb_stopped (XfmpcSsignal *cover)
{
	XfmpcSsignalPrivate *priv = XFMPC_SSIGNAL (cover)->priv;
	priv->img = xfmpc_cover_get_default();
	g_signal_emit (cover, cover_changed_signal, 0);
}

static void
cb_pp_changed (XfmpcSsignal *cover, gboolean is_playing)
{
	if(is_playing) cb_song_changed(cover);
	else cb_stopped(cover);
}

GdkPixbuf*
xfmpc_cover_get_picture (XfmpcSsignal *cover, gint width)
{
	XfmpcSsignalPrivate *priv = XFMPC_SSIGNAL (cover)->priv;
	GdkPixbuf* picture;
	gint height =
		width*gdk_pixbuf_get_height(priv->img)/gdk_pixbuf_get_width(priv->img);
	picture = gdk_pixbuf_scale_simple(priv->img, width, height, GDK_INTERP_BILINEAR);
	return picture;
}
