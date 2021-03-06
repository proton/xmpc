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

#include "etc.h"
#include "preferences-dialog.h"

#include <gtk/gtk.h>

#include "menu.h"
#include "preferences.h"
#include "main-window.h"

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_MENU, XfmpcMenuPrivate))

static void             xfmpc_menu_class_init              (XfmpcMenuClass *klass);
static void             xfmpc_menu_init                    (XfmpcMenu *menu);
static void             xfmpc_menu_finalize                (GObject *object);

static void             xfmpc_menu_settings                (XfmpcMenu *menu);
static void             xfmpc_menu_about                   (XfmpcMenu *menu);
static void             xfmpc_menu_quit                    (XfmpcMenu *menu);

struct _XfmpcMenuClass
{
	GObjectClass              parent_class;
};

struct _XfmpcMenu
{
	GObject                   parent;
	//XfmpcMainWindow          *window;
	/*<private>*/
	XfmpcMenuPrivate    *priv;
};

struct _XfmpcMenuPrivate
{

};

static GObjectClass *parent_class = NULL;

GType
xfmpc_menu_get_type ()
{
	static GType xfmpc_menu_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_menu_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_menu_info =
				{
					sizeof (XfmpcMenuClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_menu_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcMenu),
					0,
					(GInstanceInitFunc) xfmpc_menu_init,
					NULL
				};
			xfmpc_menu_type = g_type_register_static (G_TYPE_OBJECT, "XfmpcMenu", &xfmpc_menu_info, 0);
		}

	return xfmpc_menu_type;
}



static void
xfmpc_menu_class_init (XfmpcMenuClass *klass)
{
	GObjectClass *gobject_class;

	//g_type_class_add_private (klass, sizeof (XfmpcMenuPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = xfmpc_menu_finalize;
}

static void
xfmpc_menu_init (XfmpcMenu *menu)
{
	//XfmpcMenuPrivate *priv = menu->priv = GET_PRIVATE (menu);
	//menu->window = xfmpc_main_window_get ();
}

static void
xfmpc_menu_finalize (GObject *object)
{
	//XfmpcMenu *menu = XFMPC_MENU (object);
	//XfmpcMenuPrivate *priv = XFMPC_MENU (menu)->priv;
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}



XfmpcMenu *
xfmpc_menu_get ()
{
	static XfmpcMenu *menu = NULL;

	if (G_UNLIKELY (NULL == menu))
		{
			menu = g_object_new (XFMPC_TYPE_MENU, NULL);
			g_object_add_weak_pointer (G_OBJECT (menu), (gpointer)&menu);
		}
	else
		g_object_ref (G_OBJECT (menu));

	return menu;
}

void
xfmpc_menu_add (XfmpcMenu *menu, GtkWidget *pmenu)
{
	GtkWidget *mi = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (pmenu), mi);
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_PREFERENCES, NULL);
	g_signal_connect_swapped (mi, "activate", G_CALLBACK (xfmpc_menu_settings), menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (pmenu), mi);
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, NULL);
	g_signal_connect_swapped (mi, "activate", G_CALLBACK (xfmpc_menu_about), menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (pmenu), mi);
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, NULL);
	g_signal_connect_swapped (mi, "activate", G_CALLBACK (xfmpc_menu_quit), menu);
	gtk_menu_shell_append (GTK_MENU_SHELL (pmenu), mi);
}

static void
xfmpc_menu_settings (XfmpcMenu *menu)
{
	GtkWidget *dialog = xfmpc_preferences_dialog_new (NULL);
	gtk_widget_show (dialog);
}

static void
xfmpc_menu_about (XfmpcMenu *menu)
{
	static const gchar *artists[] = { NULL };
	static const gchar *authors[] = {
		"Mike Massonnet <mmassonnet@xfce.org>",
		"Vincent Legout <vincent@xfce.org>",
		"Peter Savichev <psavichev@gmail.com>",
		NULL,
	};
	static const gchar *documenters[] = { NULL };

	gtk_show_about_dialog (NULL,
												"artists", artists,
												"authors", authors,
												"comments", _("MPD client written in GTK+ for Xfce"),
												"copyright", "Copyright \302\251 2008-2009 Mike Massonnet, Vincent Legout, Peter Savichev",
												"documenters", documenters,
												//"license", XFCE_LICENSE_GPL,
												"translator-credits", _("translator-credits"),
												//"version", PACKAGE_VERSION,
												"website", "http://goodies.xfce.org/projects/applications/xfmpc",
												NULL);
}

static void
xfmpc_menu_quit (XfmpcMenu *menu)
{
	gtk_main_quit();
}
