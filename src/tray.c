/*
 *  Copyright 2009 Peter Savichev <psavichev@gmail.com>
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
#include <libnotify/notify.h>

#include "tray.h"
#include "preferences.h"
#include "mpdclient.h"
#include "cover.h"
#include "menu.h"
#include "main-window.h"
#include "etc.h"

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_TRAY, XfmpcTrayPrivate))

static void             xfmpc_tray_class_init               (XfmpcTrayClass *klass);
static void             xfmpc_tray_init                     (XfmpcTray *tray);
static void             xfmpc_tray_dispose                  (GObject *object);
static void             xfmpc_tray_finalize                 (GObject *object);

static void             cb_song_changed                     (XfmpcTray *tray);
static void             cb_pp_changed                       (XfmpcTray *tray,
																														gboolean is_playing);
static void             cb_stopped                          (XfmpcTray *tray);
static void             cb_popup_menu                       (XfmpcTray *tray);
static void             scroll                              (XfmpcTray *tray,
																														GdkEventScroll *event);
static void             xfmpc_tray_activate                 (XfmpcTray *tray);
static void             xfmpc_tray_hide                     (XfmpcTray *tray);
static void             xfmpc_tray_play                     (XfmpcTray *tray);
static void             xfmpc_tray_pause                    (XfmpcTray *tray);
static void             xfmpc_tray_stop                     (XfmpcTray *tray);
static void             xfmpc_tray_next                     (XfmpcTray *tray);
static void             xfmpc_tray_prev                     (XfmpcTray *tray);

static void             cb_win_hide                         (XfmpcTray *tray);
static void             cb_win_show                         (XfmpcTray *tray);

static void             cb_notify_show_changed              (XfmpcTray *tray);
static void             cb_tray_show_changed              (XfmpcTray *tray);

struct _XfmpcTrayClass
{
	GtkStatusIconClass        parent_class;
};

struct _XfmpcTray
{
	GtkStatusIcon             parent;
	XfmpcPreferences         *preferences;
	XfmpcMpdclient           *mpdclient;
	XfmpcSsignal             *ssignal;
	XfmpcMainWindow          *window;
	/*<private>*/
	XfmpcTrayPrivate     *priv;
};

struct _XfmpcTrayPrivate
{
	GtkWidget                *menu;
	GtkWidget                *menu_item_play;
	GtkWidget                *menu_item_pause;
	GtkWidget                *menu_item_hide;
	NotifyNotification       *notification;
	gboolean                  show_notification;
	gboolean                  show_tray;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_tray_get_type ()
{
	static GType xfmpc_tray_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_tray_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_tray_info =
				{
					sizeof (XfmpcTrayClass),
					(GBaseInitFunc) NULL,
					(GBaseFinalizeFunc) NULL,
					(GClassInitFunc) xfmpc_tray_class_init,
					(GClassFinalizeFunc) NULL,
					NULL,
					sizeof (XfmpcTray),
					0,
					(GInstanceInitFunc) xfmpc_tray_init,
					NULL
				};
			xfmpc_tray_type = g_type_register_static (GTK_TYPE_STATUS_ICON, "XfmpcTray", &xfmpc_tray_info, 0);
		}

	return xfmpc_tray_type;
}

static void
xfmpc_tray_class_init (XfmpcTrayClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcTrayPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->dispose = xfmpc_tray_dispose;
	gobject_class->finalize = xfmpc_tray_finalize;
}

gboolean button_press (XfmpcTray* tray, GdkEventButton * event, GdkWindowEdge edge)
{
	if (event->type == GDK_BUTTON_PRESS)
	{
		if (event->button == 2) {xfmpc_mpdclient_pp(tray->mpdclient);}
	}

	return FALSE;
}


static void
xfmpc_tray_init (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = tray->priv = GET_PRIVATE (tray);

	tray->preferences = xfmpc_preferences_get ();
	tray->mpdclient = xfmpc_mpdclient_get ();
	tray->ssignal = xfmpc_ssignal_get ();
	tray->window = xfmpc_main_window_get ();

	g_object_get (G_OBJECT (tray->preferences),
								"show-notify", &priv->show_notification,
								NULL);
	g_object_get (G_OBJECT (tray->preferences),
								"tray-icon", &priv->show_tray,
								NULL);

	notify_init("Xfmpc");

	/* Menu */
	priv->menu = gtk_menu_new ();

	priv->menu_item_hide = gtk_check_menu_item_new_with_label (_("Hide"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->menu_item_hide);
	g_signal_connect_swapped (priv->menu_item_hide, "activate",
														G_CALLBACK (xfmpc_tray_hide), tray);

	GtkWidget  *mi = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);

	priv->menu_item_play = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PLAY, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->menu_item_play);
	g_signal_connect_swapped (priv->menu_item_play, "activate",
														G_CALLBACK (xfmpc_tray_play), tray);
	priv->menu_item_pause = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PAUSE, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->menu_item_pause);
	g_signal_connect_swapped (priv->menu_item_pause, "activate",
														G_CALLBACK (xfmpc_tray_pause), tray);
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_STOP, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_tray_stop), tray);
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_tray_prev), tray);
	mi = gtk_image_menu_item_new_from_stock (GTK_STOCK_MEDIA_NEXT, NULL);
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), mi);
	g_signal_connect_swapped (mi, "activate",
														G_CALLBACK (xfmpc_tray_next), tray);

	xfmpc_menu_add(xfmpc_menu_get(), priv->menu);

	gtk_widget_show_all (priv->menu);

	/* Icon */
	gtk_status_icon_set_from_icon_name(&tray->parent, GTK_STOCK_MEDIA_STOP);
	gtk_status_icon_set_visible(&tray->parent, priv->show_tray);

	g_signal_connect(G_OBJECT(&tray->parent), "activate", G_CALLBACK(xfmpc_tray_activate), tray);
	g_signal_connect(G_OBJECT(&tray->parent), "popup-menu", G_CALLBACK(cb_popup_menu), tray);
	g_signal_connect(G_OBJECT(&tray->parent), "scroll-event", G_CALLBACK(scroll), tray);
	g_signal_connect(G_OBJECT(&tray->parent), "button-press-event", G_CALLBACK(button_press), NULL);
	//
	g_signal_connect_swapped (tray->mpdclient, "song-changed",
														G_CALLBACK (cb_song_changed), tray);
	g_signal_connect_swapped (tray->mpdclient, "pp-changed",
														G_CALLBACK (cb_pp_changed), tray);
	g_signal_connect_swapped (tray->mpdclient, "stopped",
														G_CALLBACK (cb_stopped), tray);
	g_signal_connect_swapped (tray->window, "hide",
														G_CALLBACK (cb_win_hide), tray);
	g_signal_connect_swapped (tray->window, "show",
														G_CALLBACK (cb_win_show), tray);
	/* Preferences */
	g_signal_connect_swapped (tray->preferences, "notify::show-notify",
														G_CALLBACK (cb_notify_show_changed), tray);
	g_signal_connect_swapped (tray->preferences, "notify::tray-icon",
														G_CALLBACK (cb_tray_show_changed), tray);
}

static void
xfmpc_tray_dispose (GObject *object)
{
	(*G_OBJECT_CLASS (parent_class)->dispose) (object);
}

static void
xfmpc_tray_finalize (GObject *object)
{
	XfmpcTray *tray = XFMPC_TRAY (object);
	g_object_unref (G_OBJECT (tray->mpdclient));
	g_object_unref (G_OBJECT (tray->preferences));
	(*G_OBJECT_CLASS (parent_class)->finalize) (object);
}

GtkStatusIcon*
xfmpc_tray_new ()
{
	return g_object_new (XFMPC_TYPE_TRAY, NULL);
}

static void
cb_song_changed (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	if(!priv->show_notification) return;
	if(priv->notification) notify_notification_close(priv->notification, NULL);
	gchar *title = g_strdup(xfmpc_mpdclient_get_title (tray->mpdclient));
	gchar *text = g_strdup_printf (_("<b>Artist:</b> %s\n<b>Album:</b> %s\n<b>Year:</b> %s"),
																xfmpc_mpdclient_get_artist (tray->mpdclient),
																xfmpc_mpdclient_get_album (tray->mpdclient),
																xfmpc_mpdclient_get_date (tray->mpdclient));

	priv->notification = notify_notification_new (title, text, NULL, NULL);
	if(priv->show_tray) notify_notification_attach_to_status_icon(priv->notification, &tray->parent);
	GdkPixbuf* cover = xfmpc_cover_get_picture (tray->ssignal, 77);
	notify_notification_set_icon_from_pixbuf (priv->notification, cover);
	//notify_notification_set_urgency (priv->notification, NOTIFY_URGENCY_NORMAL);
	//notify_notification_add_action (priv->notification, "Previous", _("Previous"), xfmpc_tray_prev, NULL, NULL);
	//notify_notification_add_action (priv->notification, "Pause", _("Pause"), xfmpc_tray_next, NULL, NULL);
	//notify_notification_add_action (priv->notification, "Next", _("Next"), xfmpc_tray_next, NULL, NULL);
	//notify_notification_attach_to_widget (priv->notification, GTK_WIDGET(&tray->parent));
	//notify_notification_set_timeout (priv->notification, 5);
	notify_notification_show (priv->notification, NULL);
	g_free (title);
	g_free (text);
}

static void
cb_pp_changed (XfmpcTray *tray, gboolean is_playing)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	gtk_widget_set_visible(priv->menu_item_play, !is_playing);
	gtk_widget_set_visible(priv->menu_item_pause, is_playing);
	switch(is_playing)
	{
		case TRUE:
			gtk_status_icon_set_from_icon_name(&tray->parent, GTK_STOCK_MEDIA_PLAY);
			cb_song_changed (tray);
			break;
		case FALSE:
			gtk_status_icon_set_from_icon_name(&tray->parent, GTK_STOCK_MEDIA_PAUSE);
			break;
	}

}

static void
cb_stopped (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	gtk_status_icon_set_from_icon_name(&tray->parent, GTK_STOCK_MEDIA_STOP);
	gtk_widget_set_visible(priv->menu_item_play, TRUE);
	gtk_widget_set_visible(priv->menu_item_pause, FALSE);

}

static void
cb_popup_menu (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	gtk_menu_popup (GTK_MENU (priv->menu),
									NULL, NULL,
									NULL, NULL,
									0,
									gtk_get_current_event_time ());
}

static void
scroll (XfmpcTray *tray, GdkEventScroll *event)
{
	int lvl = xfmpc_mpdclient_get_volume(tray->mpdclient);
	if(event->direction==GDK_SCROLL_UP || event->direction==GDK_SCROLL_RIGHT) lvl+=3;
	else lvl-=3;
	if(lvl>100) lvl=100;
	else if(lvl<0) lvl=0;
	xfmpc_mpdclient_set_volume (tray->mpdclient, lvl);
}

static void
xfmpc_tray_activate (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = tray->priv = GET_PRIVATE (tray);
	gboolean hidden = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (priv->menu_item_hide));
	gtk_widget_set_visible(GTK_WIDGET(tray->window), hidden);
}

static void
xfmpc_tray_hide (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = tray->priv = GET_PRIVATE (tray);
	gboolean hidden = gtk_check_menu_item_get_active (GTK_CHECK_MENU_ITEM (priv->menu_item_hide));
	gtk_widget_set_visible(GTK_WIDGET(tray->window), !hidden);
}

static void
xfmpc_tray_play (XfmpcTray *tray)
{
	xfmpc_mpdclient_play(tray->mpdclient);
}

static void
xfmpc_tray_pause (XfmpcTray *tray)
{
	xfmpc_mpdclient_pause(tray->mpdclient);
}

static void
xfmpc_tray_stop (XfmpcTray *tray)
{
	xfmpc_mpdclient_stop(tray->mpdclient);
}

static void
xfmpc_tray_next (XfmpcTray *tray)
{
	xfmpc_mpdclient_next(tray->mpdclient);
}

static void
xfmpc_tray_prev (XfmpcTray *tray)
{
	xfmpc_mpdclient_previous(tray->mpdclient);
}

static void
cb_win_hide (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	GTK_CHECK_MENU_ITEM(priv->menu_item_hide)->active = TRUE;
}

static void
cb_win_show (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	GTK_CHECK_MENU_ITEM(priv->menu_item_hide)->active = FALSE;
}

static void
cb_notify_show_changed (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	g_object_get (G_OBJECT (tray->preferences),
								"show-notify", &priv->show_notification,
								NULL);
	if(priv->show_notification) cb_song_changed (tray);
}

static void
cb_tray_show_changed (XfmpcTray *tray)
{
	XfmpcTrayPrivate *priv = XFMPC_TRAY (tray)->priv;
	g_object_get (G_OBJECT (tray->preferences),
								"tray-icon", &priv->show_tray,
								NULL);
	gtk_status_icon_set_visible(&tray->parent, priv->show_tray);
	if(!priv->show_tray) gtk_widget_show(GTK_WIDGET(tray->window));
}
