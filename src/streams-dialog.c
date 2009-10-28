/*
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

#include "etc.h"

#include <gtk/gtk.h>

#include "streams-dialog.h"
#include "preferences.h"

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_STREAMS_DIALOG, XfmpcStreamsDialogPrivate))



static void               xfmpc_streams_dialog_class_init (XfmpcStreamsDialogClass *klass);
static void               xfmpc_streams_dialog_init       (XfmpcStreamsDialog *dialog);
static void               xfmpc_streams_dialog_finalize   (GObject *object);

static void               xfmpc_streams_dialog_response   (GtkDialog *dialog,
																															 gint response);

struct _XfmpcStreamsDialogClass
{
	GtkDialogClass             parent_class;
};

struct _XfmpcStreamsDialog
{
	GtkDialog                  parent;
	XfmpcPreferences                 *preferences;
	/*<private>*/
	XfmpcStreamsDialogPrivate    *priv;
};

struct _XfmpcStreamsDialogPrivate
{
	GtkWidget                        *entry_name;
	GtkWidget                        *entry_url;

	gint                              id;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_streams_dialog_get_type (void)
{
	static GType xfmpc_streams_dialog_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_streams_dialog_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_streams_dialog_info =
			{
				sizeof (XfmpcStreamsDialogClass),
				NULL,
				NULL,
				(GClassInitFunc) xfmpc_streams_dialog_class_init,
				NULL,
				NULL,
				sizeof (XfmpcStreamsDialog),
				0,
				(GInstanceInitFunc) xfmpc_streams_dialog_init,
				NULL,
			};

			xfmpc_streams_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XfmpcStreamsDialog", &xfmpc_streams_dialog_info, 0);
		}

	return xfmpc_streams_dialog_type;
}



static void
xfmpc_streams_dialog_class_init (XfmpcStreamsDialogClass *klass)
{
	GtkDialogClass *gtkdialog_class;
	GObjectClass   *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcStreamsDialogPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = xfmpc_streams_dialog_finalize;

	gtkdialog_class = GTK_DIALOG_CLASS (klass);
	gtkdialog_class->response = xfmpc_streams_dialog_response;
}

static void
xfmpc_streams_dialog_init (XfmpcStreamsDialog *dialog)
{
	XfmpcStreamsDialogPrivate *priv = dialog->priv = GET_PRIVATE (dialog);

	GtkWidget *label;
	GtkWidget *table;

	dialog->preferences = xfmpc_preferences_get ();

	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	table = gtk_table_new (2, 2, FALSE);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), table, TRUE, TRUE, 0);

	label = gtk_label_new (_("Stream name: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 0, 1);
	label = gtk_label_new (_("Stream URL: "));
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 0, 1, 1, 2);
	priv->entry_name = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), priv->entry_name, 1, 2, 0, 1);
	priv->entry_url = gtk_entry_new ();
	gtk_table_attach_defaults (GTK_TABLE (table), priv->entry_url, 1, 2, 1, 2);
	gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);
}

static void
xfmpc_streams_dialog_finalize (GObject *object)
{
	XfmpcStreamsDialog *dialog = XFMPC_STREAMS_DIALOG (object);
	g_object_unref (G_OBJECT (dialog->preferences));
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
xfmpc_streams_dialog_response (GtkDialog *dialog, gint response)
{
	XfmpcStreamsDialogPrivate *priv = XFMPC_STREAMS_DIALOG (dialog)->priv;
	XfmpcPreferences *preferences = XFMPC_STREAMS_DIALOG (dialog)->preferences;
	switch(response)
	{
		case GTK_RESPONSE_OK:
			if(priv->id==-1)
				xfmpc_preferences_add_stream(preferences,
					gtk_entry_get_text (GTK_ENTRY (priv->entry_name)),
					gtk_entry_get_text (GTK_ENTRY (priv->entry_url)));
			else
				xfmpc_preferences_edit_stream(preferences, priv->id,
					gtk_entry_get_text (GTK_ENTRY (priv->entry_name)),
					gtk_entry_get_text (GTK_ENTRY (priv->entry_url)));
			break;
		default: break;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}

GtkWidget*
xfmpc_streams_dialog_new (gint id)
{
	XfmpcStreamsDialog *dialog = g_object_new (XFMPC_TYPE_STREAMS_DIALOG, NULL);
	XfmpcStreamsDialogPrivate *priv = XFMPC_STREAMS_DIALOG (dialog)->priv;
	priv->id = id;
	if(id!=-1)
	{
		gchar* name;
		gchar* url;
		xfmpc_preferences_stream_get (dialog->preferences, id, &name, &url);
		gtk_entry_set_text (GTK_ENTRY (priv->entry_name), name);
		gtk_entry_set_text (GTK_ENTRY (priv->entry_url), url);
		g_free (name);
		g_free (url);
	}
	return dialog;
}
