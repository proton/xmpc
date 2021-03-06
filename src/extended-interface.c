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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>

#include "etc.h"

#include "extended-interface.h"
#include "playlist.h"
#include "dbbrowser.h"
#include "statusbar.h"
#include "streams.h"
#include "xfce-arrow-button.h"
#include "preferences.h"
#include "lists.h"
#include "songinfo.h"

#define BORDER 4

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_EXTENDED_INTERFACE, XfmpcExtendedInterfacePrivate))



/* List store identifiers */
enum
{
	COLUMN_STRING,
	COLUMN_POINTER,
	N_COLUMNS,
};



static void xfmpc_extended_interface_class_init               (XfmpcExtendedInterfaceClass *klass);
static void xfmpc_extended_interface_init                     (XfmpcExtendedInterface *extended_interface);
static void xfmpc_extended_interface_dispose                  (GObject *object);
static void xfmpc_extended_interface_finalize                 (GObject *object);

static void cb_interface_changed                              (GtkComboBox *widget,
																															XfmpcExtendedInterface *extended_interface);

static void cb_show_combo_changed                             (XfmpcExtendedInterface *extended_interface);
static void cb_show_tabs_changed                              (XfmpcExtendedInterface *extended_interface);



struct _XfmpcExtendedInterfaceClass
{
	GtkVBoxClass                      parent_class;
};

struct _XfmpcExtendedInterface
{
	GtkVBox                           parent;
	XfmpcPreferences                 *preferences;
	/*<private>*/
	XfmpcExtendedInterfacePrivate    *priv;
};

struct _XfmpcExtendedInterfacePrivate
{
	GtkWidget                        *notebook;
	GtkWidget                        *context_button;
	GtkWidget                        *context_menu;
	GtkListStore                     *list_store;
	GtkWidget                        *combobox;
	GtkWidget                        *combo_hbox;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_extended_interface_get_type ()
{
	static GType xfmpc_extended_interface_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_extended_interface_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_extended_interface_info =
				{
					sizeof (XfmpcExtendedInterfaceClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_extended_interface_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcExtendedInterface),
					0,
					(GInstanceInitFunc) xfmpc_extended_interface_init,
					NULL
				};
			xfmpc_extended_interface_type = g_type_register_static (GTK_TYPE_VBOX, "XfmpcExtendedInterface", &xfmpc_extended_interface_info, 0);
		}

	return xfmpc_extended_interface_type;
}

static void
xfmpc_extended_interface_class_init (XfmpcExtendedInterfaceClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcExtendedInterfacePrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = xfmpc_extended_interface_dispose;
	gobject_class->finalize = xfmpc_extended_interface_finalize;
}

static void
xfmpc_extended_interface_init (XfmpcExtendedInterface *extended_interface)
{
	gboolean show_combo, show_tabs;

	XfmpcExtendedInterfacePrivate *priv = extended_interface->priv = GET_PRIVATE (extended_interface);

	extended_interface->preferences = xfmpc_preferences_get ();

	g_object_get (G_OBJECT (extended_interface->preferences),
								"combobox", &show_combo,
								"tab-headers", &show_tabs,
								NULL);

	/* Hbox  */
	priv->combo_hbox = gtk_hbox_new (FALSE, 2);
	gtk_box_pack_start (GTK_BOX (extended_interface), priv->combo_hbox, FALSE, FALSE, 0);

	/* Combo box */

	priv->list_store = gtk_list_store_new (N_COLUMNS,
																				G_TYPE_STRING,
																				G_TYPE_POINTER);

	priv->combobox = gtk_combo_box_new_with_model (GTK_TREE_MODEL (priv->list_store));
	g_signal_connect (priv->combobox, "changed",
										G_CALLBACK (cb_interface_changed), extended_interface);

	GtkCellRenderer *cell = gtk_cell_renderer_text_new ();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (priv->combobox), cell, TRUE);
	gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (priv->combobox),
																	cell, "text", COLUMN_STRING,
																	NULL);
	g_object_ref(G_OBJECT(priv->combobox));
	if(show_combo)
	{
		gtk_container_add (GTK_CONTAINER(priv->combo_hbox), priv->combobox);
	}

	/* Notebook */
	priv->notebook = gtk_notebook_new ();
	gtk_box_pack_start (GTK_BOX (extended_interface), priv->notebook, TRUE, TRUE, 0);

	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), show_tabs);

	/* Extended interface widgets ; added in the same order as the
	 * XfmpcExtendedInterfaceWidget enum */
	GtkWidget *playlist = xfmpc_playlist_new ();
	xfmpc_extended_interface_append_child (extended_interface, playlist, _("Playlist"), GTK_STOCK_CDROM);

	GtkWidget *dbbrowser = xfmpc_dbbrowser_new ();
	xfmpc_extended_interface_append_child (extended_interface, dbbrowser, _("Database"), GTK_STOCK_HARDDISK);

	GtkWidget *playlists = xfmpc_lists_new ();
	xfmpc_extended_interface_append_child (extended_interface, playlists, _("Lists"), GTK_STOCK_JUSTIFY_CENTER);

	//TODO:
//	GtkWidget *streams = xfmpc_streams_new ();
//	xfmpc_extended_interface_append_child (extended_interface, streams, _("Streams"), GTK_STOCK_NETWORK);

//	GtkWidget *songinfo = xfmpc_songinfo_new ();
//	xfmpc_extended_interface_append_child (extended_interface, songinfo, _("Info"), GTK_STOCK_INFO);
//
//	g_object_set_data (G_OBJECT (playlist), "XfmpcDbbrowser", dbbrowser);
//	g_object_set_data (G_OBJECT (playlist), "XfmpcExtendedInterface", extended_interface);

	/* Preferences */
	g_signal_connect_swapped (extended_interface->preferences, "notify::combobox",
														G_CALLBACK (cb_show_combo_changed), extended_interface);
	g_signal_connect_swapped (extended_interface->preferences, "notify::tab-headers",
														G_CALLBACK (cb_show_tabs_changed), extended_interface);
}

static void
xfmpc_extended_interface_dispose (GObject *object)
{
	XfmpcExtendedInterfacePrivate *priv = XFMPC_EXTENDED_INTERFACE (object)->priv;

	if (GTK_IS_MENU (priv->context_menu))
		{
			gtk_menu_detach (GTK_MENU (priv->context_menu));
			priv->context_menu = NULL;
		}

	if (GTK_IS_WIDGET (priv->context_button))
		{
			gtk_widget_destroy (priv->context_button);
			priv->context_button = NULL;
		}

	(*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

static void
xfmpc_extended_interface_finalize (GObject *object)
{
	//XfmpcExtendedInterface *extended_interface = XFMPC_EXTENDED_INTERFACE (object);
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

GtkWidget*
xfmpc_extended_interface_new ()
{
	return g_object_new (XFMPC_TYPE_EXTENDED_INTERFACE, NULL);
}

void
xfmpc_extended_interface_append_child (XfmpcExtendedInterface *extended_interface,
																			GtkWidget *child,
																			const gchar *title,
																			const gchar *icon)
{
	XfmpcExtendedInterfacePrivate *priv = XFMPC_EXTENDED_INTERFACE (extended_interface)->priv;
	GtkTreeIter iter;

	gtk_list_store_append (priv->list_store, &iter);
	gtk_list_store_set (priv->list_store, &iter,
											COLUMN_STRING, title,
											COLUMN_POINTER, child,
											-1);

	if (gtk_combo_box_get_active(GTK_COMBO_BOX (priv->combobox)) == -1)
		gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combobox), 0);

	GtkWidget *hbox = gtk_hbox_new (0, FALSE);
	GtkWidget *label = gtk_label_new (title);
	GtkWidget *image = gtk_image_new_from_stock (icon, GTK_ICON_SIZE_MENU);
	gtk_box_pack_start (GTK_BOX(hbox),image, TRUE, TRUE, 2);
	gtk_box_pack_start (GTK_BOX(hbox),label, TRUE, TRUE, 2);
	gtk_widget_show_all (hbox);
	gtk_notebook_append_page (GTK_NOTEBOOK (priv->notebook), child, hbox);
	gtk_notebook_set_tab_label_packing (GTK_NOTEBOOK (priv->notebook), child, FALSE, TRUE, GTK_PACK_START);
	gtk_notebook_set_tab_reorderable (GTK_NOTEBOOK (priv->notebook), child, TRUE);
}

void
xfmpc_extended_interface_set_active (XfmpcExtendedInterface *extended_interface,
																		XfmpcExtendedInterfaceWidget active_widget)
{
	//XfmpcExtendedInterfacePrivate *priv = extended_interface->priv;
}

static void
cb_interface_changed (GtkComboBox *widget,
											XfmpcExtendedInterface *extended_interface)
{
	XfmpcExtendedInterfacePrivate *priv = XFMPC_EXTENDED_INTERFACE (extended_interface)->priv;
	GtkWidget            *child;
	GtkTreeIter           iter;
	gint                  i;

	if (gtk_combo_box_get_active_iter (widget, &iter) == FALSE)
		return;

	gtk_tree_model_get (GTK_TREE_MODEL (priv->list_store), &iter,
											COLUMN_POINTER, &child,
											-1);
	g_return_if_fail (G_LIKELY (GTK_IS_WIDGET (child)));

	i = gtk_notebook_page_num (GTK_NOTEBOOK (priv->notebook), child);
	gtk_notebook_set_current_page (GTK_NOTEBOOK (priv->notebook), i);
}

static void
cb_show_combo_changed (XfmpcExtendedInterface *extended_interface)
{
	gboolean show_combo;

	XfmpcExtendedInterfacePrivate *priv = XFMPC_EXTENDED_INTERFACE (extended_interface)->priv;

	g_object_get (G_OBJECT (extended_interface->preferences),
								"combobox", &show_combo,
								NULL);
	if(show_combo)
	{
		gtk_container_add (GTK_CONTAINER(priv->combo_hbox), priv->combobox);
		gtk_widget_show_all (priv->combo_hbox);
	}
	else
	{
		gtk_container_remove (GTK_CONTAINER(priv->combo_hbox), priv->combobox);
	}
}

static void
cb_show_tabs_changed (XfmpcExtendedInterface *extended_interface)
{
	gboolean show_tabs;

	XfmpcExtendedInterfacePrivate *priv = XFMPC_EXTENDED_INTERFACE (extended_interface)->priv;

	g_object_get (G_OBJECT (extended_interface->preferences),
								"tab-headers", &show_tabs,
								NULL);
	gtk_notebook_set_show_tabs (GTK_NOTEBOOK (priv->notebook), show_tabs);
}
