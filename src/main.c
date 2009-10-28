/*
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
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

#include <string.h>
#include <stdlib.h>
#include <libintl.h>

//#include <config.h>

#include <gtk/gtk.h>

#include "main-window.h"
#include "tray.h"

static void
transform_string_to_int (const GValue *src,
												GValue *dst)
{
	g_value_set_int (dst, (gint) strtol (g_value_get_string (src), NULL, 10));
}

static void
transform_string_to_boolean (const GValue *src,
														GValue *dst)
{
	g_value_set_boolean (dst, (gboolean) strcmp (g_value_get_string (src), "FALSE") != 0);
}

static void
transform_string_to_enum (const GValue *src,
													GValue *dst)
{
	GEnumClass *klass;
	gint        value = 0;
	guint       n;

	klass = g_type_class_ref (G_VALUE_TYPE (dst));
	for (n = 0; n < klass->n_values; ++n)
		{
			value = klass->values[n].value;
			if (!g_ascii_strcasecmp (klass->values[n].value_name, g_value_get_string (src)))
				break;
		}
	g_type_class_unref (klass);

	g_value_set_enum (dst, value);
}



int
main (int argc, char *argv[])
{
	//xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");
	//setlocale (LC_ALL, "");
	//bindtextdomain (PACKAGE, LOCALEDIR);
	//textdomain (PACKAGE);

	gtk_init (&argc, &argv);

	g_value_register_transform_func (G_TYPE_STRING, G_TYPE_INT, transform_string_to_int);
	g_value_register_transform_func (G_TYPE_STRING, G_TYPE_BOOLEAN, transform_string_to_boolean);
	g_value_register_transform_func (G_TYPE_STRING, G_TYPE_ENUM, transform_string_to_enum);

	XfmpcMainWindow *window = xfmpc_main_window_get ();
	//GtkStatusIcon *tray =
	xfmpc_tray_new();
	gtk_widget_show_all (GTK_WIDGET (window));

	gtk_main ();

	return 0;
}

