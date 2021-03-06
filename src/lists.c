/*
 *  Copyright (c) 2008-2009 Mike Massonnet <mmassonnet@xfce.org>
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
#include <gdk/gdkkeysyms.h>

#include "lists.h"
#include "preferences.h"
#include "mpdclient.h"
#include "menu.h"

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_LISTS, XfmpcListsPrivate))



static void             xfmpc_lists_class_init	(XfmpcListsClass *klass);
static void             xfmpc_lists_init        (XfmpcLists *lists);
static void             xfmpc_lists_dispose		(GObject *object);
static void             xfmpc_lists_finalize	(GObject *object);

static void             xfmpc_lists_create	(XfmpcLists *lists);
static void             xfmpc_lists_edit	(XfmpcLists *lists);
static void             xfmpc_lists_delete	(XfmpcLists *lists);

//static void             cb_row_activated	(XfmpcLists *lists,	GtkTreePath *path, GtkTreeViewColumn *column);
static gboolean         cb_key_pressed		(XfmpcLists *lists, GdkEventKey *event);
static gboolean         cb_button_pressed	(XfmpcLists *lists, GdkEventButton *event);
static gboolean         cb_popup_menu		(XfmpcLists *lists);
static void             popup_menu			(XfmpcLists *lists);

/* List store identifiers */
enum
{
	COLUMN_ID,
	COLUMN_PIXBUF,
	COLUMN_NAME,
	COLUMN_IS_DIR,
	N_COLUMNS,
};

struct _XfmpcListsClass
{
	GtkVBoxClass              parent_class;
};

struct _XfmpcLists
{
	GtkVBox                   parent;
	XfmpcPreferences         *preferences;
	XfmpcMpdclient           *mpdclient;
	/*<private>*/
	XfmpcListsPrivate    *priv;
};

struct _XfmpcListsPrivate
{
	GtkWidget                *treeview;
	GtkListStore             *store;
	GtkWidget                *menu;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_lists_get_type ()
{
	static GType xfmpc_lists_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_lists_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_lists_info =
				{
					sizeof (XfmpcListsClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_lists_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcLists),
					0,
					(GInstanceInitFunc) xfmpc_lists_init,
					NULL
				};
			xfmpc_lists_type = g_type_register_static (GTK_TYPE_VBOX, "XfmpcLists", &xfmpc_lists_info, 0);
		}

	return xfmpc_lists_type;
}



static void
xfmpc_lists_class_init (XfmpcListsClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcListsPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = xfmpc_lists_dispose;
	gobject_class->finalize = xfmpc_lists_finalize;
}

static void
xfmpc_lists_init (XfmpcLists *lists)
{
	XfmpcListsPrivate *priv = lists->priv = GET_PRIVATE (lists);

	lists->preferences = xfmpc_preferences_get ();
	lists->mpdclient = xfmpc_mpdclient_get ();

	/* === Tree model === */
	priv->store = gtk_list_store_new (N_COLUMNS,
																		G_TYPE_INT,
																		GDK_TYPE_PIXBUF,
																		G_TYPE_STRING,
																		G_TYPE_STRING,
																		G_TYPE_BOOLEAN,
																		G_TYPE_INT);

	/* === Tree view === */
	priv->treeview = gtk_tree_view_new ();
	gtk_tree_selection_set_mode (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)), GTK_SELECTION_MULTIPLE);
	gtk_tree_view_set_rubber_banding (GTK_TREE_VIEW (priv->treeview), TRUE);
	gtk_tree_view_set_enable_search (GTK_TREE_VIEW (priv->treeview), TRUE);
	gtk_tree_view_set_search_column (GTK_TREE_VIEW (priv->treeview), COLUMN_NAME);
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
	g_object_set (G_OBJECT (cell),
								"ellipsize", PANGO_ELLIPSIZE_END,
								NULL);
	gtk_tree_view_insert_column_with_attributes (GTK_TREE_VIEW (priv->treeview),
																							-1, "Filename", cell,
																							"text", COLUMN_NAME,
																							NULL);

	/* Scrolled window */
	GtkWidget *scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
																	GTK_POLICY_AUTOMATIC,
																	GTK_POLICY_ALWAYS);

	/* Menu */
	priv->menu = gtk_menu_new ();

	/* Menu -> Add */
	GtkWidget *mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_ADD, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_lists_add_playlist), lists);

	/* Menu -> Replace */
	mi = gtk_image_menu_item_new_with_mnemonic (_("Replace"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_lists_replace_playlist), lists);
	GtkWidget *image = gtk_image_new_from_stock (GTK_STOCK_UNDO, GTK_ICON_SIZE_MENU);
	gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), image);

	/* Menu -> Separator */
	mi = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);

	/* Menu -> Create */
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_NEW, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_lists_create), lists);

	/* Menu -> Edit */
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_EDIT, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_lists_edit), lists);

	/* Menu -> Remove */
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_DELETE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_lists_delete), lists);

	xfmpc_menu_add(xfmpc_menu_get(), priv->menu);

	gtk_widget_show_all (priv->menu);

	/* === Containers === */
	gtk_container_add (GTK_CONTAINER (scrolled), priv->treeview);
	gtk_box_pack_start (GTK_BOX (lists), scrolled, TRUE, TRUE, 0);

	/* === Signals === */
	g_signal_connect_swapped (lists->mpdclient, "connected",
														G_CALLBACK (xfmpc_lists_reload), lists);
	/* Tree view */
	g_signal_connect_swapped (priv->treeview, "row-activated",
														G_CALLBACK (xfmpc_lists_add_playlist), lists);
	g_signal_connect_swapped (priv->treeview, "key-press-event",
														G_CALLBACK (cb_key_pressed), lists);
	g_signal_connect_swapped (priv->treeview, "button-press-event",
														G_CALLBACK (cb_button_pressed), lists);
	g_signal_connect_swapped (priv->treeview, "popup-menu",
														G_CALLBACK (cb_popup_menu), lists);
}

static void
xfmpc_lists_dispose (GObject *object)
{
	(*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

static void
xfmpc_lists_finalize (GObject *object)
{
	XfmpcLists *lists = XFMPC_LISTS (object);
	//XfmpcListsPrivate *priv = XFMPC_LISTS (lists)->priv;

	g_object_unref (G_OBJECT (lists->preferences));
	g_object_unref (G_OBJECT (lists->mpdclient));
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}



GtkWidget*
xfmpc_lists_new ()
{
	return g_object_new (XFMPC_TYPE_LISTS, NULL);
}

void
xfmpc_lists_clear (XfmpcLists *lists)
{
	XfmpcListsPrivate *priv = XFMPC_LISTS (lists)->priv;
	gtk_list_store_clear (priv->store);
}

void
xfmpc_lists_append (XfmpcLists *lists,
												gchar *name)
{
	XfmpcListsPrivate    *priv = XFMPC_LISTS (lists)->priv;
	GdkPixbuf                *pixbuf;
	GtkTreeIter               iter;

	pixbuf = gtk_widget_render_icon (priv->treeview,
																	GTK_STOCK_JUSTIFY_FILL,
																	GTK_ICON_SIZE_MENU,
																	NULL);

	gtk_list_store_append (priv->store, &iter);
	gtk_list_store_set (priv->store, &iter,
											COLUMN_PIXBUF, pixbuf,
											COLUMN_NAME, name,
											-1);
}

void
xfmpc_lists_add_playlist (XfmpcLists *lists)
{
	XfmpcListsPrivate *priv = XFMPC_LISTS (lists)->priv;
	GtkTreeModel         *store = GTK_TREE_MODEL (priv->store);
	GtkTreeIter           iter;
	GList                *l, *list;
	gchar                *name, *filename;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)), &store);

	for (l = list; l != NULL; l = l->next)
	{
		if (gtk_tree_model_get_iter (store, &iter, l->data))
		{
			gtk_tree_model_get (store, &iter, COLUMN_NAME, &name, -1);
			while (xfmpc_mpdclient_database_playlist_read_contents (lists->mpdclient, name, &filename))
			{
				xfmpc_mpdclient_queue_add (lists->mpdclient, filename);
				g_free (filename);
			}
			g_free (name);
		}
	}

	xfmpc_mpdclient_queue_commit (lists->mpdclient);

	g_list_foreach (list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free (list);
}

void
xfmpc_lists_replace_playlist (XfmpcLists *lists)
{
	xfmpc_mpdclient_queue_clear (lists->mpdclient);
	xfmpc_lists_add_playlist (lists);
}

void
xfmpc_lists_reload (XfmpcLists *lists)
{
	//XfmpcListsPrivate    *priv = XFMPC_LISTS (lists)->priv;
	gchar                    *name;

	if (G_UNLIKELY (!xfmpc_mpdclient_is_connected (lists->mpdclient))) return;

	xfmpc_lists_clear (lists);

	while (xfmpc_mpdclient_database_playlist_read (lists->mpdclient, &name))
	{
		xfmpc_lists_append (lists, name);
		g_free (name);
	}
}

static gboolean
cb_key_pressed (XfmpcLists *lists, GdkEventKey *event)
{
	XfmpcListsPrivate    *priv = XFMPC_LISTS (lists)->priv;
	GtkTreeSelection         *selection;
	//gchar                    *filename;

	if (event->type != GDK_KEY_PRESS) return FALSE;

	switch (event->keyval)
	{
	case GDK_Return:
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
		if (gtk_tree_selection_count_selected_rows (selection) > 1)
			xfmpc_lists_add_playlist (lists);
		else return FALSE;
	break;
	default: return FALSE;
	}

	return TRUE;
}

static gboolean
cb_button_pressed (XfmpcLists *lists,
									GdkEventButton *event)
{
	XfmpcListsPrivate    *priv = XFMPC_LISTS (lists)->priv;
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

	popup_menu (lists);

	return TRUE;
}

static gboolean
cb_popup_menu (XfmpcLists *lists)
{
	popup_menu (lists);
	return TRUE;
}

static void
popup_menu (XfmpcLists *lists)
{
	XfmpcListsPrivate *priv = XFMPC_LISTS (lists)->priv;
	GtkTreeSelection      *selection;
	gint                   count;

	selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview));
	count = gtk_tree_selection_count_selected_rows (selection);

	gtk_menu_popup (GTK_MENU (priv->menu),
									NULL, NULL,
									NULL, NULL,
									0,
									gtk_get_current_event_time ());
}


static void
xfmpc_lists_create (XfmpcLists *lists)
{
	GtkWidget *dialog;
	GtkWidget *entry;
	GtkWidget *label;
	gint response;

	dialog = gtk_dialog_new_with_buttons (_("Playlist creating"),
					GTK_WINDOW (NULL),
					GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
					GTK_STOCK_OK,
					GTK_RESPONSE_OK,
					GTK_STOCK_CANCEL,
					GTK_RESPONSE_CANCEL,
					NULL);

	label = gtk_label_new (_("Name of playlist:"));
	entry = gtk_entry_new ();

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, TRUE, FALSE, 0);

	gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);
	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_OK)
		xfmpc_mpdclient_database_playlist_create (lists->mpdclient, gtk_entry_get_text (GTK_ENTRY (entry)));

	gtk_widget_destroy (dialog);

	xfmpc_lists_reload (lists);
}

static void
xfmpc_lists_edit (XfmpcLists *lists)
{
	XfmpcListsPrivate *priv = XFMPC_LISTS (lists)->priv;
	GtkTreeModel         *store = GTK_TREE_MODEL (priv->store);
	GtkTreeIter           iter;
	GList                *list;
	gchar                *name;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)),
																							&store);

	if (NULL != list)
	if (gtk_tree_model_get_iter (store, &iter, list->data))
	{
		gtk_tree_model_get (store, &iter, COLUMN_NAME, &name, -1);
		GtkWidget *dialog;
		GtkWidget *entry;
		GtkWidget *label;
		gint response;

		dialog = gtk_dialog_new_with_buttons (_("Playlist renaming"),
						GTK_WINDOW (NULL),
						GTK_DIALOG_MODAL| GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_STOCK_OK,
						GTK_RESPONSE_OK,
						GTK_STOCK_CANCEL,
						GTK_RESPONSE_CANCEL,
						NULL);

		label = gtk_label_new (_("New name of the playlist:"));
		entry = gtk_entry_new ();
		gtk_entry_set_text (GTK_ENTRY (entry), name);

		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), label, TRUE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), entry, TRUE, FALSE, 0);

		gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);
		response = gtk_dialog_run (GTK_DIALOG (dialog));

		if (response == GTK_RESPONSE_OK)
			xfmpc_mpdclient_database_playlist_create (lists->mpdclient, gtk_entry_get_text (GTK_ENTRY (entry)));

		gtk_widget_destroy (dialog);
	}

	xfmpc_lists_reload (lists);
}

static void
xfmpc_lists_delete (XfmpcLists *lists)
{
	XfmpcListsPrivate *priv = XFMPC_LISTS (lists)->priv;
	GtkTreeModel         *store = GTK_TREE_MODEL (priv->store);
	GtkTreeIter           iter;
	GList                *list;
	gchar                *name;

	list = gtk_tree_selection_get_selected_rows (gtk_tree_view_get_selection (GTK_TREE_VIEW (priv->treeview)),
																							&store);

	if (NULL != list)
	if (gtk_tree_model_get_iter (store, &iter, list->data))
	{
		gtk_tree_model_get (store, &iter, COLUMN_NAME, &name, -1);
		GtkWidget *dialog = gtk_message_dialog_new (NULL,
						GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_INFO, GTK_BUTTONS_YES_NO,
						_("Do you realy want to delete this playlist?"));

		gint response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_YES)
		xfmpc_mpdclient_database_playlist_remove (lists->mpdclient, name);

		gtk_widget_destroy (dialog);
	}

	xfmpc_lists_reload (lists);
}
