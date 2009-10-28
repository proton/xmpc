/*
 *      streams.c
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

#include "streams.h"
#include "streams-dialog.h"
#include "mpdclient.h"
#include "preferences.h"
#include "menu.h"

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_STREAMS, XfmpcStreamsPrivate))

static void             xfmpc_streams_class_init             (XfmpcStreamsClass *klass);
static void             xfmpc_streams_init                   (XfmpcStreams *streams);
static void             xfmpc_streams_dispose                (GObject *object);
static void             xfmpc_streams_finalize               (GObject *object);

static void             cb_row_activated                    (XfmpcStreams *streams,
																														GtkTreePath *path,
																														GtkTreeViewColumn *column);
static gboolean         cb_button_pressed                   (XfmpcStreams *streams,
																														GdkEventButton *event);
static void             cb_popup_menu                       (XfmpcStreams *streams);
static void             xfmpc_streams_add                   (XfmpcStreams *streams);
static void             xfmpc_streams_replace               (XfmpcStreams *streams);
static void             xfmpc_streams_create                (XfmpcStreams *streams);
static void             xfmpc_streams_edit                  (XfmpcStreams *streams);
static void             xfmpc_streams_delete                (XfmpcStreams *streams);
static void             xfmpc_streams_load                  (XfmpcStreams *streams);

/* List store identifiers */
enum
{
	COLUMN_ID,
	COLUMN_PIXBUF,
	COLUMN_STREAMNAME,
	COLUMN_STREAMURI,
	N_COLUMNS,
};

struct _XfmpcStreamsClass
{
	GtkVBoxClass          parent_class;
};

struct _XfmpcStreams
{
	GtkVBox                   parent;
	XfmpcPreferences         *preferences;
	XfmpcMpdclient           *mpdclient;
	/*<private>*/
	XfmpcStreamsPrivate     *priv;
};

struct _XfmpcStreamsPrivate
{
	GtkWidget                *treeview;
	GtkListStore             *store;
	GtkWidget                *menu;
};

static GObjectClass *parent_class = NULL;

GtkWidget*
xfmpc_streams_new ()
{
	return g_object_new (XFMPC_TYPE_STREAMS, NULL);
}

GType
xfmpc_streams_get_type ()
{
	static GType xfmpc_streams_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_streams_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_streams_info =
				{
					sizeof (XfmpcStreamsClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_streams_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcStreams),
					0,
					(GInstanceInitFunc) xfmpc_streams_init,
					NULL
				};
			xfmpc_streams_type = g_type_register_static (GTK_TYPE_VBOX, "XfmpcStreams", &xfmpc_streams_info, 0);
		}

	return xfmpc_streams_type;
}

static void
xfmpc_streams_class_init (XfmpcStreamsClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcStreamsPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = xfmpc_streams_dispose;
	gobject_class->finalize = xfmpc_streams_finalize;
}

static void
xfmpc_streams_init (XfmpcStreams *streams)
{
	XfmpcStreamsPrivate *priv = streams->priv = GET_PRIVATE (streams);

	streams->mpdclient = xfmpc_mpdclient_get ();
	streams->preferences = xfmpc_preferences_get ();

	/* === Tree model === */
	priv->store = gtk_list_store_new (N_COLUMNS,
																		G_TYPE_INT,
																		GDK_TYPE_PIXBUF,
																		G_TYPE_STRING,
																		G_TYPE_STRING);

	/* === Tree view === */
	priv->treeview = gtk_tree_view_new ();
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (priv->treeview), TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (priv->treeview), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (priv->treeview), COLUMN_STREAMNAME);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (priv->treeview), FALSE);
	gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (priv->treeview), TRUE);
	gtk_tree_view_set_model (GTK_TREE_VIEW (priv->treeview), GTK_TREE_MODEL (priv->store));
	g_object_unref (priv->store);

	/* Columns */
	GtkCellRenderer *cell = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview),
																							-1, "", cell,
																							"pixbuf", COLUMN_PIXBUF,
																							NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview),
																							-1, "Name", cell,
																							"text", COLUMN_STREAMNAME,
																							NULL);

	cell = gtk_cell_renderer_text_new ();
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview),
																							-1, "URI", cell,
																							"text", COLUMN_STREAMURI,
																							NULL);

	/* Scrolled window */
	GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
																	GTK_POLICY_AUTOMATIC,
																	GTK_POLICY_ALWAYS);
	/* Streams */
	xfmpc_streams_load (streams);

	/* Menu */
	priv->menu = gtk_menu_new ();

	/* Menu -> Add */
	GtkWidget *mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_streams_add), streams);

	/* Menu -> Replace */
	mi = gtk_image_menu_item_new_with_mnemonic (_("Replace"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_streams_replace), streams);
	GtkWidget *image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);

	/* Menu -> Separator */
	mi = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);

	/* Menu -> Create */
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_streams_create), streams);

	/* Menu -> Edit */
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_EDIT, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_streams_edit), streams);

	/* Menu -> Remove */
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_streams_delete), streams);

	xfmpc_menu_add(xfmpc_menu_get(), priv->menu);

	gtk_widget_show_all (priv->menu);

	/* === Containers === */
	gtk_container_add (GTK_CONTAINER (scrolled), priv->treeview);
	gtk_box_pack_start (GTK_BOX (streams), scrolled, TRUE, TRUE, 0);

	/* === Signals === */
	g_signal_connect_swapped (priv->treeview, "row-activated",
														G_CALLBACK (cb_row_activated), streams);
	g_signal_connect_swapped (priv->treeview, "button-press-event",
														G_CALLBACK (cb_button_pressed), streams);
	g_signal_connect_swapped (priv->treeview, "popup-menu",
														G_CALLBACK (cb_popup_menu), streams);
	g_signal_connect_swapped (streams->preferences, "stream-update",
														G_CALLBACK (xfmpc_streams_load), streams);
}

static void
xfmpc_streams_dispose (GObject *object)
{
	(*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

static void
xfmpc_streams_finalize (GObject *object)
{
	XfmpcStreams *streams = XFMPC_STREAMS (object);
	//XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;

	g_object_unref (G_OBJECT (streams->mpdclient));
	g_object_unref (G_OBJECT (streams->preferences));
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

void
xfmpc_streams_append (XfmpcStreams *streams, guint n, gchar *name, gchar *uri)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	GtkTreeIter          iter;
	GdkPixbuf           *pixbuf = gtk_widget_render_icon (priv->treeview,
																	GTK_STOCK_NETWORK,
																	GTK_ICON_SIZE_MENU,
																	NULL);
	gtk_list_store_append (priv->store, &iter);
	gtk_list_store_set (priv->store, &iter,
											COLUMN_ID, n,
											COLUMN_PIXBUF, pixbuf,
											COLUMN_STREAMNAME, name,
											COLUMN_STREAMURI, uri,
											-1);
}

static gboolean
cb_button_pressed (XfmpcStreams *streams,
									GdkEventButton *event)
{
	XfmpcStreamsPrivate    *priv = XFMPC_STREAMS (streams)->priv;
	GtkTreeSelection         *selection;
	GtkTreePath              *path;

	if (event->type != GDK_BUTTON_PRESS || event->button != 3)
		return FALSE;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
	if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (priv->treeview),
																		event->x, event->y,
																		&path, NULL, NULL, NULL))
		{
			if (!gtk_tree_selection_path_is_selected (selection, path))
				{
					gtk_tree_selection_unselect_all (selection);
					gtk_tree_selection_select_path (selection, path);
				}
			gtk_tree_path_free (path);
		}

	cb_popup_menu (streams);

	return TRUE;
}

static void
cb_popup_menu (XfmpcStreams *streams)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	gtk_menu_popup (GTK_MENU (priv->menu),
									NULL, NULL,
									NULL, NULL,
									0,
									gtk_get_current_event_time ());
}

static void
xfmpc_streams_load (XfmpcStreams *streams)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	gtk_list_store_clear (priv->store);
	struct XfmpcStreamInfo* streamlist;
	guint count = xfmpc_preferences_get_streams (streams->preferences, &streamlist);
	guint n;
	for (n = 0; n<count; ++n)
	{
		xfmpc_streams_append (streams, n, streamlist[n].name, streamlist[n].url);
		g_free (streamlist[n].url);
		g_free (streamlist[n].name);
	}
	g_free (streamlist);
}

static void
xfmpc_streams_add (XfmpcStreams *streams)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	GtkTreeModel        *store = GTK_TREE_MODEL (priv->store);
	GtkTreeIter          iter;
	GList               *l, *list;
	gchar               *filename;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)), &store);

	for (l = list; l != NULL; l = l->next)
	{
		if (gtk_tree_model_get_iter (store, &iter, l->data))
		{
			gtk_tree_model_get (store, &iter, COLUMN_STREAMURI, &filename, -1);
			xfmpc_mpdclient_queue_add (streams->mpdclient, filename);
			g_free (filename);
		}
	}
	xfmpc_mpdclient_queue_commit (streams->mpdclient);
	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
}

static void
xfmpc_streams_replace (XfmpcStreams *streams)
{
	xfmpc_mpdclient_queue_clear (streams->mpdclient);
	xfmpc_streams_add (streams);
}

static void
xfmpc_streams_create (XfmpcStreams *streams)
{
	GtkWidget *dialog = xfmpc_streams_dialog_new (-1);
	gtk_widget_show (dialog);
}

static void
xfmpc_streams_edit (XfmpcStreams *streams)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	GtkTreeModel         *store = GTK_TREE_MODEL (priv->store);
	GtkTreeIter           iter;
	GList                *list;
	gint                  id;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)),
																							&store);

	if (NULL != list)
	if (gtk_tree_model_get_iter (store, &iter, list->data))
	{
		gtk_tree_model_get (store, &iter, COLUMN_ID, &id, -1);
		GtkWidget *dialog = xfmpc_streams_dialog_new (id);
		gtk_widget_show (dialog);
	}
}

static void
xfmpc_streams_delete (XfmpcStreams *streams)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	GtkTreeModel         *store = GTK_TREE_MODEL (priv->store);
	GtkTreeIter           iter;
	GList                *list;
	gint                  id;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)),
																							&store);

	if (NULL != list)
	if (gtk_tree_model_get_iter (store, &iter, list->data))
	{
		gtk_tree_model_get (store, &iter, COLUMN_ID, &id, -1);
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO,
						_("Do you realy want to delete this stream?"));

		gint response = gtk_dialog_run (GTK_DIALOG (dialog));

		if (response == GTK_RESPONSE_YES) xfmpc_preferences_delete_stream (streams->preferences, id);

		gtk_widget_destroy (dialog);
	}
}

static void
cb_row_activated (XfmpcStreams *streams,
									GtkTreePath *path,
									GtkTreeViewColumn *column)
{
	XfmpcStreamsPrivate *priv = XFMPC_STREAMS (streams)->priv;
	GtkTreeIter               iter;
	gchar                    *filename;

	if (!gtk_tree_model_get_iter (GTK_TREE_MODEL (priv->store), &iter, path))
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (priv->store), &iter,
											COLUMN_STREAMURI, &filename,
											-1);
	xfmpc_mpdclient_queue_add (streams->mpdclient, filename);
	xfmpc_mpdclient_queue_commit (streams->mpdclient);
	g_free (filename);
}
