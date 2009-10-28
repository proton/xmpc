/*
 *  Copyright (c) 2009 Peter Savichev <psavichev@gmail.org>
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
 *  Copyright (c) 2009 Vincent Legout <vincent@xfce.org>
 *
 *  Based on ThunarPreferences:
 *  Copyright (c) 2005-2007 Benedikt Meurer <benny@xfce.org>
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

#include <glib-object.h>

#include "preferences.h"



/* Enum types */
GType
xfmpc_song_format_get_type ()
{
	static GType type = G_TYPE_INVALID;

	static const GEnumValue values[] =
	{
		{ XFMPC_SONG_FORMAT_TITLE, "XFMPC_SONG_FORMAT_TITLE", "Title", },
		{ XFMPC_SONG_FORMAT_ALBUM_TITLE, "XFMPC_SONG_FORMAT_ALBUM_TITLE", "Album - Title", },
		{ XFMPC_SONG_FORMAT_ARTIST_TITLE, "XFMPC_SONG_FORMAT_ARTIST_TITLE", "Artist - Title", },
		{ XFMPC_SONG_FORMAT_ARTIST_TITLE_DATE, "XFMPC_SONG_FORMAT_ARTIST_TITLE_DATE", "Artist - Title (Date)", },
		{ XFMPC_SONG_FORMAT_ARTIST_ALBUM_TITLE, "XFMPC_SONG_FORMAT_ARTIST_ALBUM_TITLE", "Artist - Album - Title", },
		{ XFMPC_SONG_FORMAT_ARTIST_ALBUM_TRACK_TITLE, "XFMPC_SONG_FORMAT_ARTIST_ALBUM_TRACK_TITLE", "Artist - Album - Track. Title", },
		{ XFMPC_SONG_FORMAT_CUSTOM, "XFMPC_SONG_FORMAT_CUSTOM", "Custom...", },
		{ 0, NULL, NULL },
	};
	/*static const GEnumValue values[] =
	{

	};*/

	if (type != G_TYPE_INVALID) return type;

	type = g_enum_register_static ("XfmpcSongFormat", values);
	return type;
}

/* Property identifiers */
enum
{
	PROP_0,
	PROP_LAST_WINDOW_POSX,
	PROP_LAST_WINDOW_POSY,
	PROP_LAST_WINDOW_WIDTH,
	PROP_LAST_WINDOW_HEIGHT,
	PROP_LAST_WINDOW_STATE_STICKY,
	PROP_PLAYLIST_AUTOCENTER,
	PROP_DBBROWSER_LAST_PATH,
	PROP_MPD_HOST,
	PROP_MPD_PORT,
	PROP_MPD_PASSWORD,
	PROP_MPD_USE_DEFAULTS,
	PROP_SHOW_STATUSBAR,
	PROP_SONG_FORMAT,
	PROP_SONG_FORMAT_CUSTOM,
	PROP_COVER_FILENAME,
	PROP_LIBRARY_DIR,
	PROP_TRAY_ICON,
	PROP_CLOSE_TO_TRAY,
	PROP_NOTIFY_SHOW,
	PROP_COMBOBOX,
	PROP_TAB_HEADERS,
	PROP_DBBROWSER_BOLD_PLAYED,
	N_PROPERTIES,
};

static guint stream_update_signal;

static void             xfmpc_preferences_class_init            (XfmpcPreferencesClass *klass);
static void             xfmpc_preferences_init                  (XfmpcPreferences *preferences);
static void             xfmpc_preferences_finalize              (GObject *object);
static void             xfmpc_preferences_get_property          (GObject *object,
																																guint prop_id,
																																GValue *value,
																																GParamSpec *pspec);
static void             xfmpc_preferences_set_property          (GObject *object,
																																guint prop_id,
																																const GValue *value,
																																GParamSpec *pspec);

static void             xfmpc_preferences_load                  (XfmpcPreferences *preferences);

static void             xfmpc_preferences_store                 (XfmpcPreferences *preferences);

static GKeyFile *       xfmpc_preferences_get_keyfile           (XfmpcPreferences *preferences);




struct _XfmpcPreferencesClass
{
	GObjectClass          parent_class;
};

struct _XfmpcPreferences
{
	GObject               parent;
	/*<private>*/
	guint                 stream_count;
	GValue                values[N_PROPERTIES];
};



static GObjectClass *parent_class = NULL;



GType
xfmpc_preferences_get_type ()
{
	static GType xfmpc_preferences_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_preferences_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_preferences_info =
				{
					sizeof (XfmpcPreferencesClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_preferences_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcPreferences),
					0,
					(GInstanceInitFunc) xfmpc_preferences_init,
					NULL
				};
			xfmpc_preferences_type = g_type_register_static (G_TYPE_OBJECT, "XfmpcPreferences", &xfmpc_preferences_info, 0);
		}

	return xfmpc_preferences_type;
}



static void
xfmpc_preferences_class_init (XfmpcPreferencesClass *klass)
{
	GObjectClass *gobject_class;

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = xfmpc_preferences_finalize;
	gobject_class->get_property = xfmpc_preferences_get_property;
	gobject_class->set_property = xfmpc_preferences_set_property;

	g_object_class_install_property (gobject_class,
																	PROP_LAST_WINDOW_POSX,
																	g_param_spec_int ("last-window-posx",
																										"LastWindowPosx",
																										"Window position on axis x",
																										-1, G_MAXINT, -1,
																										G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_LAST_WINDOW_POSY,
																	g_param_spec_int ("last-window-posy",
																										"LastWindowPosy",
																										"Window position on axis y",
																										-1, G_MAXINT, -1,
																										G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_LAST_WINDOW_WIDTH,
																	g_param_spec_int ("last-window-width",
																										"LastWindowWidth",
																										"Window width",
																										-1, G_MAXINT, -1,
																										G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_LAST_WINDOW_HEIGHT,
																	g_param_spec_int ("last-window-height",
																										"LastWindowHeight",
																										"Window height",
																										-1, G_MAXINT, -1,
																										G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_LAST_WINDOW_STATE_STICKY,
																	g_param_spec_boolean ("last-window-state-sticky",
																												"LastWindowStateSticky",
																												"Sticky bit on window",
																												FALSE,
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_PLAYLIST_AUTOCENTER,
																	g_param_spec_boolean ("playlist-autocenter",
																												"PlaylistAutocenter",
																												"Auto-centers the current song in the playlist",
																												TRUE,
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_DBBROWSER_LAST_PATH,
																	g_param_spec_string ("dbbrowser-last-path",
																												"DbbrowserLastPath",
																												"Restores the last path from the database browser",
																												"",
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_MPD_HOST,
																	g_param_spec_string ("mpd-hostname",
																												"MpdHostname",
																												"Hostname of the MPD server",
																												"localhost",
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_MPD_PORT,
																	g_param_spec_int ("mpd-port",
																											"MpdPort",
																											"Port of the MPD server",
																											0, 65536, 6600,
																											G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_MPD_PASSWORD,
																	g_param_spec_string ("mpd-password",
																												"MpdPassword",
																												"Password of the MPD server",
																												NULL,
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_MPD_USE_DEFAULTS,
																	g_param_spec_boolean ("mpd-use-defaults",
																												"MpdUseDefaults",
																												"Use default system settings for the MPD server (MPD_HOST/PORT environment variables)",
																												TRUE,
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_SHOW_STATUSBAR,
																	g_param_spec_boolean ("show-statusbar",
																												"ShowStatusbar",
																												"Show the statusbar",
																												TRUE,
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_SONG_FORMAT,
																	g_param_spec_enum ("song-format",
																											"SongFormat",
																											"Song Format",
																											XFMPC_TYPE_SONG_FORMAT,
																											XFMPC_SONG_FORMAT_ARTIST_TITLE,
																											G_PARAM_CONSTRUCT|G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_SONG_FORMAT_CUSTOM,
																	g_param_spec_string ("song-format-custom",
																												"SongFormatCustom",
																												"Custom Song Format",
																												"\%a - \%t",
																												G_PARAM_READWRITE));

	g_object_class_install_property (gobject_class,
																	PROP_LIBRARY_DIR,
																	g_param_spec_string ("library-dir",
																												"LibraryDir",
																												"Library Directory",
																												"/var/lib/mpd/music",
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_COVER_FILENAME,
																	g_param_spec_string ("cover-filename",
																												"CoverFileName",
																												"Cover Filename",
																												".cover.jpg",
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_TRAY_ICON,
																	g_param_spec_boolean ("tray-icon",
																												"TrayIcon",
																												"Show the tray icon",
																												TRUE,
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_CLOSE_TO_TRAY,
																	g_param_spec_boolean ("close-to-tray",
																												"CloseToTray",
																												"Close to tray",
																												TRUE,
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_NOTIFY_SHOW,
																	g_param_spec_boolean ("show-notify",
																												"ShowNotify",
																												"Show the notications",
																												TRUE,
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_COMBOBOX,
																	g_param_spec_boolean ("combobox",
																												"Combobox",
																												"Show the combobox",
																												TRUE,
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_TAB_HEADERS,
																	g_param_spec_boolean ("tab-headers",
																												"TabHeaders",
																												"Show the tab headers",
																												TRUE,
																												G_PARAM_READWRITE));
	g_object_class_install_property (gobject_class,
																	PROP_DBBROWSER_BOLD_PLAYED,
																	g_param_spec_boolean ("db-bold",
																												"DBElemBold",
																												"Highlight played songs in dbbrowser",
																												FALSE,
																												G_PARAM_READWRITE));

	stream_update_signal = g_signal_new ("stream-update", G_TYPE_FROM_CLASS (klass),
																												G_SIGNAL_RUN_FIRST|G_SIGNAL_ACTION,
																												0, NULL, NULL,
																												g_cclosure_marshal_VOID__VOID,
																												G_TYPE_NONE,
																												0);
}

static void
xfmpc_preferences_init (XfmpcPreferences *preferences)
{
	xfmpc_preferences_load (preferences);
}

static void
xfmpc_preferences_finalize (GObject *object)
{
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

static void
xfmpc_preferences_get_property (GObject *object,
																guint prop_id,
																GValue *value,
																GParamSpec *pspec)
{
	XfmpcPreferences *preferences = XFMPC_PREFERENCES (object);
	GValue *src;

	src = preferences->values + prop_id;
	if (G_LIKELY (G_IS_VALUE (src))) g_value_copy (src, value);
	else g_param_value_set_default (pspec, value);
}

static void
xfmpc_preferences_set_property (GObject *object,
																guint prop_id,
																const GValue *value,
																GParamSpec *pspec)
{
	XfmpcPreferences *preferences = XFMPC_PREFERENCES (object);
	GValue *dst;

	dst = preferences->values + prop_id;
	if (G_UNLIKELY (!G_IS_VALUE (dst)))
	{
		g_value_init (dst, pspec->value_type);
		g_param_value_set_default (pspec, dst);
	}

	if (G_LIKELY (g_param_values_cmp (pspec, value, dst) != 0))
	{
		g_value_copy (value, dst);
		xfmpc_preferences_store (preferences);
	}
}

XfmpcPreferences *
xfmpc_preferences_get ()
{
	static XfmpcPreferences *preferences = NULL;

	if (G_UNLIKELY (NULL == preferences))
	{
		preferences = g_object_new (XFMPC_TYPE_PREFERENCES, NULL);
		g_object_add_weak_pointer (G_OBJECT (preferences), (gpointer)&preferences);
	}
	else g_object_ref (G_OBJECT (preferences));

	return preferences;
}

static void
xfmpc_preferences_load (XfmpcPreferences *preferences)
{
	const gchar          *string;
	GParamSpec          **specs;
	GParamSpec           *spec;
	GValue                dst = { 0, };
	GValue                src = { 0, };
	GKeyFile             *file;
	GError*				  gerror = NULL;
	guint                 nspecs;
	guint                 n;

	file = xfmpc_preferences_get_keyfile (preferences);
	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to load the preferences");
		return;
	}

	g_object_freeze_notify (G_OBJECT (preferences));

	//xfce_rc_set_group (rc, "Configuration");

	specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
	for (n = 0; n < nspecs; ++n)
	{
		spec = specs[n];

		//string = xfce_rc_read_entry (rc, g_param_spec_get_nick (spec), NULL);
		string = g_key_file_get_string (file, "Configuration", g_param_spec_get_nick (spec), &gerror);

		if (G_UNLIKELY (NULL == string)) continue;

		g_value_init (&src, G_TYPE_STRING);
		g_value_set_static_string (&src, string);

		if (spec->value_type == G_TYPE_STRING)
			g_object_set_property (G_OBJECT (preferences), spec->name, &src);
		else if (g_value_type_transformable (G_TYPE_STRING, spec->value_type))
		{
			g_value_init (&dst, spec->value_type);
			if (g_value_transform (&src, &dst))
				g_object_set_property (G_OBJECT (preferences), spec->name, &dst);
			g_value_unset (&dst);
		}
		else g_warning ("Failed to load property \"%s\"", spec->name);

		g_value_unset (&src);
	}
	g_free (specs);

	g_object_thaw_notify (G_OBJECT (preferences));

	g_key_file_free(file);
}

static void
xfmpc_preferences_store (XfmpcPreferences *preferences)
{
	const gchar *string;
	GParamSpec **specs;
	GParamSpec  *spec;
	GValue       dst = { 0, };
	GValue       src = { 0, };
	GKeyFile      *file;
	guint        nspecs;
	guint        n;

	file = xfmpc_preferences_get_keyfile (preferences);

	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to save the preferences");
		return;
	}

	specs = g_object_class_list_properties (G_OBJECT_GET_CLASS (preferences), &nspecs);
	for (n = 0; n < nspecs; ++n)
	{
		spec = specs[n];

		g_value_init (&dst, G_TYPE_STRING);

		if (spec->value_type == G_TYPE_STRING)
			g_object_get_property (G_OBJECT (preferences), spec->name, &dst);
		else
		{
			g_value_init (&src, spec->value_type);
			g_object_get_property (G_OBJECT (preferences), spec->name, &src);
			g_value_transform (&src, &dst);
			g_value_unset (&src);
		}

		string = g_value_get_string (&dst);

		if (G_LIKELY (NULL != string))
			g_key_file_set_string (file, "Configuration", g_param_spec_get_nick (spec), string);

		g_value_unset (&dst);
	}
	g_free (specs);

	g_key_file_free(file);
}

static GKeyFile *
xfmpc_preferences_get_keyfile (XfmpcPreferences *preferences)
{
	GKeyFile *file;
	GError* gerror = NULL;
	gchar* fname = g_strconcat(g_get_home_dir(), ".config/xefia", NULL);
	if(!g_key_file_load_from_file (file, fname, G_KEY_FILE_KEEP_COMMENTS, &gerror)) file=NULL;
	g_free(fname);
	return file;
}

guint
xfmpc_preferences_get_streams (XfmpcPreferences *preferences, struct XfmpcStreamInfo **streamlist)
{
	GKeyFile	*file;
	GError		*gerror = NULL;
	guint        n;
	gchar*       key_name;
	gchar*       key_url;

	file = xfmpc_preferences_get_keyfile (preferences);
	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to load the preferences");
		return 0;
	}

	g_object_freeze_notify (G_OBJECT (preferences));

	//xfce_rc_set_group (rc, "Streams");

	preferences->stream_count = g_key_file_get_integer(file, "Streams", "Count", &gerror);
	//xfce_rc_read_int_entry (rc, "Count", 0);

	*streamlist = g_new(struct XfmpcStreamInfo, preferences->stream_count);

	for (n = 0; n<preferences->stream_count; ++n)
	{
		key_url = g_strdup_printf ("StreamUrl%i", n);
		key_name = g_strdup_printf ("StreamName%i", n);

		(*streamlist)[n].url = g_strdup(g_key_file_get_string (file, "Streams", key_url, &gerror));
		(*streamlist)[n].name = g_strdup(g_key_file_get_string (file, "Streams", key_name, &gerror));

		g_free (key_url);
		g_free (key_name);
	}

	g_object_thaw_notify (G_OBJECT (preferences));

	g_key_file_free(file);

	return preferences->stream_count;
}

void
xfmpc_preferences_add_stream (XfmpcPreferences *preferences, const gchar* name, const gchar* url)
{
	GKeyFile	*file;
	gchar*       key_name;
	gchar*       key_url;

	file = xfmpc_preferences_get_keyfile (preferences);
	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to load the preferences");
		return;
	}

	g_object_freeze_notify (G_OBJECT (preferences));

	key_url = g_strdup_printf ("StreamUrl%i", preferences->stream_count);
	key_name = g_strdup_printf ("StreamName%i", preferences->stream_count);

	g_key_file_set_string (file, "Streams", key_url, url);
	g_key_file_set_string (file, "Streams", key_name, name);

	++preferences->stream_count;

	g_free (key_url);
	g_free (key_name);

	g_key_file_set_integer (file, "Streams", "Count", preferences->stream_count);

	g_object_thaw_notify (G_OBJECT (preferences));

	g_key_file_free(file);
	g_signal_emit (preferences, stream_update_signal, 0);
}

void
xfmpc_preferences_edit_stream (XfmpcPreferences *preferences, guint id, const gchar* name, const gchar* url)
{
	GKeyFile      *file;
	gchar*       key_name;
	gchar*       key_url;

	file = xfmpc_preferences_get_keyfile (preferences);
	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to load the preferences");
		return;
	}

	g_object_freeze_notify (G_OBJECT (preferences));

	key_url = g_strdup_printf ("StreamUrl%i", id);
	key_name = g_strdup_printf ("StreamName%i", id);

	g_key_file_set_string (file, "Streams", key_url, url);
	g_key_file_set_string (file, "Streams", key_name, name);

	g_free (key_url);
	g_free (key_name);

	g_object_thaw_notify (G_OBJECT (preferences));

	g_key_file_free(file);
	g_signal_emit (preferences, stream_update_signal, 0);
}

void
xfmpc_preferences_delete_stream (XfmpcPreferences *preferences, guint id)
{
	GKeyFile    *file;
	GError		*gerror = NULL;
	guint        n;
	gchar*       key_name;
	gchar*       key_url;
	gchar*       key_name2;
	gchar*       key_url2;

	file = xfmpc_preferences_get_keyfile (preferences);
	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to load the preferences");
		return;
	}

	g_object_freeze_notify (G_OBJECT (preferences));

	--preferences->stream_count;

	key_url = g_strdup_printf ("StreamUrl%i", id);
	key_name = g_strdup_printf ("StreamName%i", id);
	for (n = id; n<preferences->stream_count; ++n)
	{
		key_url2 = g_strdup_printf ("StreamUrl%i", n+1);
		key_name2 = g_strdup_printf ("StreamName%i", n+1);

		g_key_file_set_string (file, "Streams", key_url,
			g_key_file_get_string(file, "Streams", key_url2, &gerror));
		g_key_file_set_string (file, "Streams", key_name,
				g_key_file_get_string(file, "Streams", key_name2, &gerror));

		g_free (key_url);
		g_free (key_name);
		key_url = key_url2;
		key_name = key_name2;
	}
	g_key_file_remove_key (file, "Streams", key_url, &gerror);
	g_key_file_remove_key (file, "Streams", key_name, &gerror);
	g_free (key_url);
	g_free (key_name);

	g_key_file_set_integer(file, "Streams", "Count", preferences->stream_count);

	g_object_thaw_notify (G_OBJECT (preferences));

	g_key_file_free(file);
	g_signal_emit (preferences, stream_update_signal, 0);
}

void
xfmpc_preferences_stream_get (XfmpcPreferences *preferences, guint id, gchar** name, gchar** url)
{
	GKeyFile	*file;
	GError		*gerror = NULL;
	gchar*       key_name;
	gchar*       key_url;

	file = xfmpc_preferences_get_keyfile (preferences);
	if (G_UNLIKELY (NULL == file))
	{
		g_warning ("Failed to load the preferences");
		return;
	}

	g_object_freeze_notify (G_OBJECT (preferences));

	//xfce_rc_set_group (rc, "Streams");

	key_url = g_strdup_printf ("StreamUrl%i", id);
	key_name = g_strdup_printf ("StreamName%i", id);

	*url = g_strdup_printf (g_key_file_get_string (file, "Streams", key_url, &gerror));
	*name = g_strdup_printf (g_key_file_get_string (file, "Streams", key_name, &gerror));

	g_free (key_url);
	g_free (key_name);

	g_object_thaw_notify (G_OBJECT (preferences));

	g_key_file_free(file);
	g_signal_emit (preferences, stream_update_signal, 0);
}
