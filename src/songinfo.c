/*
 *      songinfo.c
 *
 *      Copyright 2009 Peter Savichev <psavichev@gmail.com>
 *
 *      This program is free software; you can redistribute it and/or modify
 *      it under the terms of the GNU General Public License as published by
 *      the Free Software Foundation; either version 2 of the License, or
 *      (at your option) any later version.
 *
 *      This program is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *      GNU General Public License for more details.
 *
 *      You should have received a copy of the GNU General Public License
 *      along with this program; if not, write to the Free Software
 *      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 *      MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "etc.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include "songinfo.h"
#include "preferences.h"
#include "mpdclient.h"
#include "cover.h"
#include "lastfm.h"

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_SONGINFO, XfmpcSonginfoPrivate))

static void             xfmpc_songinfo_class_init             (XfmpcSonginfoClass *klass);
static void             xfmpc_songinfo_init                   (XfmpcSonginfo *songinfo);
static void             xfmpc_songinfo_dispose                (GObject *object);
static void             xfmpc_songinfo_finalize               (GObject *object);
static gboolean         xfmpc_interface_reconnect             (XfmpcSonginfo *songinfo);
static void             cb_song_changed                       (XfmpcSonginfo *songinfo);
static void             cb_stopped                            (XfmpcSonginfo *songinfo);
static void             cb_pp_changed                         (XfmpcSonginfo *songinfo,
																																gboolean is_playing);
static void             cb_cover_changed                      (XfmpcSonginfo *songinfo);

void             xfmpc_songinfo_append_row (XfmpcSonginfo *songinfo,
																			GtkWidget *box,
																			const gchar *title,
										GtkWidget *label,
										gint id);

/* List table row identifiers */
enum
{
	ROW_TITLE,
	ROW_ARTIST,
	ROW_ALBUM,
	ROW_YEAR,
	ROW_TRACK,
	ROW_GENRE,
	ROW_FILE,
	N_ROWS,
};

struct _XfmpcSonginfoClass
{
	GtkVBoxClass          parent_class;
};

struct _XfmpcSonginfo
{
	GtkVBox                   parent;
	XfmpcMpdclient           *mpdclient;
	XfmpcPreferences         *preferences;
	XfmpcSsignal             *cover;
	/*<private>*/
	XfmpcSonginfoPrivate     *priv;
};

struct _XfmpcSonginfoPrivate
{
	GtkWidget *expSong;
		GtkWidget *esCover;
		GtkWidget *esTitle;
		GtkWidget *esArtist;
		GtkWidget *esAlbum;
		GtkWidget *esYear;
		GtkWidget *esTrack;
		GtkWidget *esGenre;
		GtkWidget *esFile;
	GtkWidget *expLyric;
		GtkWidget *elText;
	GtkWidget *expArtist;
		GtkWidget *erText;
	GtkWidget *expAlbum;
		GtkWidget *eaText;

	gchar *dir;
	gchar *cover;
};

static GObjectClass *parent_class = NULL;

GtkWidget*
xfmpc_songinfo_new ()
{
	return g_object_new (XFMPC_TYPE_SONGINFO, NULL);
}

GType
xfmpc_songinfo_get_type ()
{
	static GType xfmpc_songinfo_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_songinfo_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_songinfo_info =
				{
					sizeof (XfmpcSonginfoClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_songinfo_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcSonginfo),
					0,
					(GInstanceInitFunc) xfmpc_songinfo_init,
					NULL
				};
			xfmpc_songinfo_type = g_type_register_static (GTK_TYPE_VBOX, "XfmpcSonginfo", &xfmpc_songinfo_info, 0);
		}

	return xfmpc_songinfo_type;
}

static void
xfmpc_songinfo_class_init (XfmpcSonginfoClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcSonginfoPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = xfmpc_songinfo_dispose;
	gobject_class->finalize = xfmpc_songinfo_finalize;
}

static void
xfmpc_songinfo_init (XfmpcSonginfo *songinfo)
{
	XfmpcSonginfoPrivate *priv = songinfo->priv = GET_PRIVATE (songinfo);
	songinfo->preferences = xfmpc_preferences_get ();
	songinfo->mpdclient = xfmpc_mpdclient_get ();
	songinfo->cover = xfmpc_ssignal_get ();

	g_object_get (G_OBJECT (songinfo->preferences),
								"cover-filename", &priv->cover,
								"library-dir", &priv->dir,
								NULL);

	/* Expanders */
	priv->expSong = gtk_expander_new (_("Song"));
	priv->expLyric = gtk_expander_new (_("Lyrics"));
	priv->expArtist = gtk_expander_new (_("Artist"));
	priv->expAlbum = gtk_expander_new (_("Album"));
	gtk_expander_set_expanded(GTK_EXPANDER(priv->expSong), TRUE);
	gtk_expander_set_expanded(GTK_EXPANDER(priv->expLyric), TRUE);
	gtk_expander_set_expanded(GTK_EXPANDER(priv->expArtist), TRUE);
	gtk_expander_set_expanded(GTK_EXPANDER(priv->expAlbum), TRUE);

	/* Cover */
	priv->esCover = gtk_image_new_from_file (NULL);

	/* Labels */
	GtkWidget *hbox = gtk_hbox_new (FALSE, 0);
	GtkWidget *table = gtk_table_new (N_ROWS, 2, FALSE);
	priv->esTitle = gtk_label_new(NULL);
	priv->esArtist = gtk_label_new(NULL);
	priv->esAlbum = gtk_label_new(NULL);
	priv->esYear = gtk_label_new(NULL);
	priv->esTrack = gtk_label_new(NULL);
	priv->esGenre = gtk_label_new(NULL);
	priv->esFile = gtk_label_new(NULL);
	priv->elText = gtk_label_new(NULL);
	priv->erText = gtk_label_new(NULL);
	priv->eaText = gtk_label_new(NULL);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>Title: </b>"), priv->esTitle, ROW_TITLE);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>Artist: </b>"), priv->esArtist, ROW_ARTIST);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>Album: </b>"), priv->esAlbum, ROW_ALBUM);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>Year: </b>"), priv->esYear, ROW_YEAR);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>Track: </b>"), priv->esTrack, ROW_TRACK);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>Genre: </b>"), priv->esGenre, ROW_GENRE);
	xfmpc_songinfo_append_row(songinfo, table, _("<b>File: </b>"), priv->esFile, ROW_FILE);
	gtk_container_add (GTK_CONTAINER (priv->expSong), hbox);
	gtk_box_pack_start (GTK_BOX (hbox), priv->esCover, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 3);

	gtk_container_add(GTK_CONTAINER (priv->expLyric), priv->elText);
	gtk_container_add(GTK_CONTAINER (priv->expArtist), priv->erText);
	gtk_container_add(GTK_CONTAINER (priv->expAlbum), priv->eaText);

	/* Scrolled window */
	GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
																	GTK_POLICY_AUTOMATIC,
																	GTK_POLICY_AUTOMATIC);
	/* === Containers === */
	GtkWidget *vbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), priv->expSong, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), priv->expLyric, FALSE, FALSE, 5);
	gtk_box_pack_start (GTK_BOX (vbox), priv->expAlbum, FALSE, FALSE, 5);
	gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled),
																					vbox);
	gtk_box_pack_start (GTK_BOX (songinfo), scrolled, TRUE, TRUE, 5);

	g_signal_connect_swapped (songinfo->mpdclient, "connected",
														G_CALLBACK (xfmpc_interface_reconnect), songinfo);
	g_signal_connect_swapped (songinfo->mpdclient, "song-changed",
														G_CALLBACK (cb_song_changed), songinfo);
	g_signal_connect_swapped (songinfo->mpdclient, "stopped",
														G_CALLBACK (cb_stopped), songinfo);
	g_signal_connect_swapped (songinfo->mpdclient, "pp-changed",
														G_CALLBACK (cb_pp_changed), songinfo);
	g_signal_connect_swapped (songinfo->cover, "cover-update",
														G_CALLBACK (cb_cover_changed), songinfo);
}

static void
xfmpc_songinfo_dispose (GObject *object)
{
	(*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

static void
xfmpc_songinfo_finalize (GObject *object)
{
	XfmpcSonginfo *songinfo = XFMPC_SONGINFO (object);
	//XfmpcSonginfoPrivate *priv = XFMPC_SONGINFO (songinfo)->priv;

	g_object_unref (G_OBJECT (songinfo->mpdclient));
	g_object_unref (G_OBJECT (songinfo->preferences));
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static gboolean
xfmpc_interface_reconnect (XfmpcSonginfo *songinfo)
{
	if (G_UNLIKELY (xfmpc_mpdclient_connect (songinfo->mpdclient) == FALSE))
		return FALSE;
	if (xfmpc_mpdclient_is_playing (songinfo->mpdclient))
		cb_song_changed (songinfo);
	return TRUE;
}

static void
cb_stopped (XfmpcSonginfo *songinfo)
{
	XfmpcSonginfoPrivate *priv = XFMPC_SONGINFO (songinfo)->priv;
	gtk_label_set_text(GTK_LABEL(priv->esTitle),	NULL);
	gtk_label_set_text(GTK_LABEL(priv->esArtist),	NULL);
	gtk_label_set_text(GTK_LABEL(priv->esAlbum),	NULL);
	gtk_label_set_text(GTK_LABEL(priv->esYear), 	NULL);
	gtk_label_set_text(GTK_LABEL(priv->esTrack),	NULL);
	gtk_label_set_text(GTK_LABEL(priv->esGenre),	NULL);
	gtk_label_set_text(GTK_LABEL(priv->esFile), 	NULL);
}

static void
cb_song_changed (XfmpcSonginfo *songinfo)
{
	XfmpcSonginfoPrivate *priv = XFMPC_SONGINFO (songinfo)->priv;
	XfmpcSongInfo *info = xfmpc_mpdclient_get_current_song_info(songinfo->mpdclient);
	gtk_label_set_text(GTK_LABEL(priv->esTitle),	info->title);
	gtk_label_set_text(GTK_LABEL(priv->esArtist),	info->artist);
	gtk_label_set_text(GTK_LABEL(priv->esAlbum),	info->album);
	gtk_label_set_text(GTK_LABEL(priv->esYear),	info->date);
	gtk_label_set_text(GTK_LABEL(priv->esTrack),	info->track);
	gtk_label_set_text(GTK_LABEL(priv->esGenre),	info->genre);
	gtk_label_set_text(GTK_LABEL(priv->esFile), 	info->filename);
	xfmpc_song_info_free(info);
}

static void
cb_pp_changed (XfmpcSonginfo *songinfo, gboolean is_playing)
{
	//XfmpcSonginfoPrivate *priv = XFMPC_SONGINFO (songinfo)->priv;
	if(is_playing) cb_song_changed (songinfo);
}

void
xfmpc_songinfo_append_row (XfmpcSonginfo *songinfo, GtkWidget *table, const gchar *title, GtkWidget *label, gint id)
{
	//XfmpcSonginfoPrivate *priv = XFMPC_SONGINFO (songinfo)->priv;
	GtkWidget *text = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(text), title);
	gtk_misc_set_alignment(GTK_MISC(text), 0., 0.5);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	gtk_misc_set_alignment(GTK_MISC(label), 0., 0.5);
	//gtk_label_set_line_wrap(GTK_LABEL(label), TRUE);

	gtk_table_attach(GTK_TABLE(table), text, 0, 1, id, id+1, GTK_FILL, GTK_SHRINK, 0, 2);
	gtk_table_attach(GTK_TABLE(table), label, 1, 2, id, id+1, GTK_FILL, GTK_SHRINK, 0, 2);
}

static void
cb_cover_changed (XfmpcSonginfo *songinfo)
{
	XfmpcSonginfoPrivate *priv = XFMPC_SONGINFO (songinfo)->priv;
	GdkPixbuf* cover = xfmpc_cover_get_picture (songinfo->cover, 150);
	gtk_image_set_from_pixbuf(GTK_IMAGE(priv->esCover), cover);
}
