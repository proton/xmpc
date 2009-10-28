/*
 *  Copyright (c) 2009 Vincent Legout <vincent@legout.info>
 *  Copyright (c) 2009 Mike Massonnet <mmassonnet@xfce.org>
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

#include "main-window.h"
#include "preferences.h"
#include "interface.h"
#include "main-ui.h"
#include "extended-interface.h"
#include "statusbar.h"
#include "menu.h"

#define BORDER 4

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_MAIN_WINDOW, XfmpcMainWindowPrivate))



static void     xfmpc_main_window_class_init               (XfmpcMainWindowClass *klass);
static void     xfmpc_main_window_init                     (XfmpcMainWindow *main_window);
static void     xfmpc_main_window_finalize                 (GObject *object);

static gboolean cb_window_state_event                      (GtkWidget *window,
																														GdkEventWindowState *event);
static gboolean cb_window_closed                           (GtkWidget *window,
																														GdkEvent *event);
static void     cb_window_hided                            (GtkWidget *window,
																														GdkEvent *event);
static void     cb_window_showed                           (GtkWidget *window,
																														GdkEvent *event);
static void     action_close                               (GtkAction *action,
																														GtkWidget *window);
static void     action_previous                            (GtkAction *action,
																														GtkWidget *window);
static void     action_pp                                  (GtkAction *action,
																														GtkWidget *window);
static void     action_stop                                (GtkAction *action,
																														GtkWidget *window);
static void     action_next                                (GtkAction *action,
																														GtkWidget *window);
static void     action_volume                              (GtkAction *action,
																														GtkWidget *window);

static void     action_statusbar                           (GtkToggleAction *action,
																														XfmpcMainWindow *main_window);

static void     xfmpc_main_window_update_statusbar         (XfmpcMainWindow *main_window);

static void     cb_playlist_changed                        (XfmpcMainWindow *main_window);
static void     cb_show_statusbar_changed                  (XfmpcMainWindow *main_window,
																														GParamSpec *pspec);

static gboolean         cb_button_pressed                  (XfmpcMainWindow *main_window,
																														GdkEventButton *event);
static void             cb_popup_menu                      (XfmpcMainWindow *main_window);

static void             cb_random                          (XfmpcMainWindow *main_window,
																														gboolean random);
static void             cb_repeat                          (XfmpcMainWindow *main_window,
																														gboolean repeat);
static void             cb_repeat_switch                   (XfmpcMainWindow *main_window);
static void             cb_random_switch                   (XfmpcMainWindow *main_window);

static void             cb_close_to_tray_changed           (XfmpcMainWindow *main_window);

static const GtkToggleActionEntry toggle_action_entries[] =
{
	{ "view-statusbar", NULL, "", NULL, NULL, G_CALLBACK (action_statusbar), FALSE, },
};



static const GtkActionEntry action_entries[] =
{
	{ "quit", NULL, "", "<control>q", NULL, G_CALLBACK (action_close), },

	{ "previous", NULL, "", "<control>b", NULL, G_CALLBACK (action_previous), },
	{ "pp", NULL, "", "<control>p", NULL, G_CALLBACK (action_pp), },
	{ "stop", NULL, "", "<control>s", NULL, G_CALLBACK (action_stop), },
	{ "next", NULL, "", "<control>f", NULL, G_CALLBACK (action_next), },
	{ "volume", NULL, "", "<control>v", NULL, G_CALLBACK (action_volume), },
};



struct _XfmpcMainWindowClass
{
	GtkWindowClass          parent;
};

struct _XfmpcMainWindow
{
	GtkWindow               parent;
	XfmpcPreferences       *preferences;
	XfmpcMpdclient         *mpdclient;
	/*<private>*/
	XfmpcMainWindowPrivate *priv;
};

struct _XfmpcMainWindowPrivate
{
	GtkActionGroup         *action_group;
	GtkWidget              *statusbar;
	GtkWidget              *vbox;
	GtkWidget              *menu;
	GtkWidget              *menu_item_random;
	GtkWidget              *menu_item_repeat;
	gboolean                close_to_tray;
	gulong                  close_to_tray_signal;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_main_window_get_type (void)
{
	static GType xfmpc_main_window_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_main_window_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_main_window_info =
			{
				sizeof (XfmpcMainWindowClass),
				(GBaseInitFunc) NULL,
				(GBaseFinalizeFunc) NULL,
				(GClassInitFunc) xfmpc_main_window_class_init,
				(GClassFinalizeFunc) NULL,
				NULL,
				sizeof (XfmpcMainWindow),
				0,
				(GInstanceInitFunc) xfmpc_main_window_init,
				NULL
			};

			xfmpc_main_window_type = g_type_register_static (GTK_TYPE_WINDOW, "XfmpcMainWindow", &xfmpc_main_window_info, 0);
		}

	return xfmpc_main_window_type;
}

static void
xfmpc_main_window_class_init (XfmpcMainWindowClass *klass)
{
	GObjectClass *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcMainWindowPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = xfmpc_main_window_finalize;
}

static void
xfmpc_main_window_init (XfmpcMainWindow *window)
{
	XfmpcMainWindowPrivate *priv = window->priv = GET_PRIVATE (window);

	GtkAction *action;
	gboolean active;
	gint posx, posy;
	gint width, height;
	gboolean sticky;

	window->mpdclient = xfmpc_mpdclient_get ();
	window->preferences = xfmpc_preferences_get ();

	g_object_get (G_OBJECT (window->preferences),
								"last-window-posx", &posx,
								"last-window-posy", &posy,
								"last-window-width", &width,
								"last-window-height", &height,
								"last-window-state-sticky", &sticky,
								"close-to-tray", &priv->close_to_tray,
								NULL);

	/* Window */
	gtk_window_set_default_icon_name ("xfmpc");
	gtk_window_set_icon_name (GTK_WINDOW (window), "stock_volume");
	//gtk_window_set_title (GTK_WINDOW (window), PACKAGE_NAME); //TODO:uncomment
	gtk_window_set_default_size (GTK_WINDOW (window), 330, 330);
	priv->close_to_tray_signal = (priv->close_to_tray)?
		g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL) :
		g_signal_connect (G_OBJECT (window), "delete-event", G_CALLBACK (cb_window_closed), NULL);
	g_signal_connect (G_OBJECT (window), "window-state-event", G_CALLBACK (cb_window_state_event), NULL);
	g_signal_connect (G_OBJECT (window), "hide", G_CALLBACK (cb_window_hided), NULL);
	g_signal_connect (G_OBJECT (window), "show", G_CALLBACK (cb_window_showed), NULL);

	priv->vbox = gtk_vbox_new (FALSE, 0);
	GtkWidget *event_box = gtk_event_box_new ();
	gtk_container_add (GTK_CONTAINER (window), event_box);
	gtk_container_add (GTK_CONTAINER (event_box), priv->vbox);

	if (G_LIKELY (posx != -1 && posy != -1))
		gtk_window_move (GTK_WINDOW (window), posx, posy);
	if (G_LIKELY (width != -1 && height != -1))
		gtk_window_set_default_size (GTK_WINDOW (window), width, height);
	if (sticky == TRUE)
		gtk_window_stick (GTK_WINDOW (window));

	/* Interface */
	GtkWidget *interface = xfmpc_interface_new ();
	g_object_set_data (G_OBJECT (window), "XfmpcInterface", interface);
	gtk_box_pack_start (GTK_BOX (priv->vbox), interface, FALSE, FALSE, BORDER);

	/* ExtendedInterface */
	GtkWidget *extended_interface = xfmpc_extended_interface_new ();
	gtk_box_pack_start (GTK_BOX (priv->vbox), extended_interface, TRUE, TRUE, 0);

	/* Accelerators */
	GtkUIManager *ui_manager = gtk_ui_manager_new ();

	/* Action group */
	priv->action_group = gtk_action_group_new ("XfmpcMainWindow");
	gtk_action_group_add_actions (priv->action_group, action_entries,
																G_N_ELEMENTS (action_entries),
																GTK_WIDGET (window));
	gtk_action_group_add_toggle_actions (priv->action_group, toggle_action_entries,
																			G_N_ELEMENTS (toggle_action_entries),
																			GTK_WIDGET (window));
	gtk_ui_manager_insert_action_group (ui_manager, priv->action_group, 0);
	gtk_ui_manager_add_ui_from_string (ui_manager, main_ui, main_ui_length, NULL);

	/* Accel group */
	GtkAccelGroup *accel_group = gtk_ui_manager_get_accel_group (ui_manager);
	gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

	/* show-statusbar action */
	action = gtk_action_group_get_action (priv->action_group, "view-statusbar");
	g_object_get (G_OBJECT (window->preferences), "show-statusbar", &active, NULL);
	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);

	priv->menu = gtk_menu_new ();
	priv->menu_item_repeat = gtk_check_menu_item_new_with_label (_("Repeat"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->menu_item_repeat);
	g_signal_connect_swapped (priv->menu_item_repeat, "activate",
														G_CALLBACK (cb_repeat_switch), window);

	priv->menu_item_random = gtk_check_menu_item_new_with_label (_("Random"));
	gtk_menu_shell_append (GTK_MENU_SHELL (priv->menu), priv->menu_item_random);
	g_signal_connect_swapped (priv->menu_item_random, "activate",
														G_CALLBACK (cb_random_switch), window);
	xfmpc_menu_add(xfmpc_menu_get(), priv->menu);
	gtk_widget_show_all (priv->menu);

	/* === Signals === */
	g_signal_connect_swapped (window->mpdclient, "playlist-changed",
														G_CALLBACK (cb_playlist_changed), window);

	g_signal_connect_swapped (window->preferences, "notify::show-statusbar",
														G_CALLBACK (cb_show_statusbar_changed), window);

	g_signal_connect_swapped (event_box, "button-press-event",
														G_CALLBACK (cb_button_pressed), window);
	g_signal_connect_swapped (event_box, "popup-menu",
														G_CALLBACK (cb_popup_menu), window);
	g_signal_connect_swapped (window->mpdclient, "repeat",
														G_CALLBACK (cb_repeat), window);
	g_signal_connect_swapped (window->mpdclient, "random",
														G_CALLBACK (cb_random), window);
	/* Preferences */
	g_signal_connect_swapped (window->preferences, "notify::close-to-tray",
														G_CALLBACK (cb_close_to_tray_changed), window);
}

static void
xfmpc_main_window_finalize (GObject *object)
{
	XfmpcMainWindow *window = XFMPC_MAIN_WINDOW (object);
	g_object_unref (G_OBJECT (window->mpdclient));
	g_object_unref (G_OBJECT (window->preferences));
	G_OBJECT_CLASS (parent_class)->finalize (object);
}

XfmpcMainWindow *
xfmpc_main_window_get ()
{
	static XfmpcMainWindow *window = NULL;

	if (G_UNLIKELY (NULL == window))
	{
		window = g_object_new (XFMPC_TYPE_MAIN_WINDOW, NULL);
		g_object_add_weak_pointer (G_OBJECT (window), (gpointer)&window);
	}
	else g_object_ref (G_OBJECT (window));

	return window;
}

static gboolean
cb_window_state_event (GtkWidget *window,
											GdkEventWindowState *event)
{
	if (G_UNLIKELY (event->type != GDK_WINDOW_STATE))
		return FALSE;

	/**
	 * Hiding the top level window will unstick it too, and send a
	 * window-state-event signal, so here we take the value only if
	 * the window is visible
	 **/
	if (event->changed_mask & GDK_WINDOW_STATE_STICKY && GTK_WIDGET_VISIBLE (window))
		{
			gboolean sticky = ((gboolean) event->new_window_state & GDK_WINDOW_STATE_STICKY) == FALSE ? FALSE : TRUE;
			XfmpcPreferences *preferences = xfmpc_preferences_get ();
			g_object_set (G_OBJECT (preferences),
										"last-window-state-sticky", sticky,
										NULL);
			g_object_unref (preferences);
		}

	return FALSE;
}

static void
cb_window_hided (GtkWidget *window,
									GdkEvent *event)
{
	/*gint posx, posy;
	gtk_window_get_position (GTK_WINDOW (window), &posx, &posy);

	XfmpcPreferences *preferences = xfmpc_preferences_get ();
	g_object_set (G_OBJECT (preferences),
								"last-window-posx", posx,
								"last-window-posy", posy,
								NULL);*/
}

static void
cb_window_showed (GtkWidget *window,
									GdkEvent *event)
{
	/*gint posx, posy;

	XfmpcPreferences *preferences = xfmpc_preferences_get ();
	g_object_get (G_OBJECT (preferences),
								"last-window-posx", &posx,
								"last-window-posy", &posy,
								NULL);

	if (G_LIKELY (posx != -1 && posy != -1))
		gtk_window_move (GTK_WINDOW (window), posx, posy);*/
}

static gboolean
cb_window_closed (GtkWidget *window,
									GdkEvent *event)
{
	gint posx, posy;
	gint width, height;
	gtk_window_get_position (GTK_WINDOW (window), &posx, &posy);
	gtk_window_get_size (GTK_WINDOW (window), &width, &height);

	XfmpcPreferences *preferences = xfmpc_preferences_get ();
	g_object_set (G_OBJECT (preferences),
								"last-window-posx", posx,
								"last-window-posy", posy,
								"last-window-width", width,
								"last-window-height", height,
								NULL);
	g_object_unref (preferences);

	gtk_main_quit ();
	return FALSE;
}


static void
action_close (GtkAction *action,
							GtkWidget *window)
{
	cb_window_closed (window, NULL);
}

static void
action_previous (GtkAction *action,
								GtkWidget *window)
{
	XfmpcInterface *interface = g_object_get_data (G_OBJECT (window), "XfmpcInterface");
	xfmpc_mpdclient_previous (interface->mpdclient);
}

static void
action_pp (GtkAction *action,
					GtkWidget *window)
{
	XfmpcInterface *interface = g_object_get_data (G_OBJECT (window), "XfmpcInterface");
	xfmpc_interface_pp_clicked (interface);
}

static void
action_stop (GtkAction *action,
						GtkWidget *window)
{
	XfmpcInterface *interface = g_object_get_data (G_OBJECT (window), "XfmpcInterface");
	xfmpc_mpdclient_stop (interface->mpdclient);
}

static void
action_next (GtkAction *action,
						GtkWidget *window)
{
	XfmpcInterface *interface = g_object_get_data (G_OBJECT (window), "XfmpcInterface");
	xfmpc_mpdclient_next (interface->mpdclient);
}

static void
action_volume (GtkAction *action,
							GtkWidget *window)
{
	XfmpcInterface *interface = g_object_get_data (G_OBJECT (window), "XfmpcInterface");
	xfmpc_interface_popup_volume (interface);
}

static void
action_statusbar (GtkToggleAction *action,
									XfmpcMainWindow *window)
{
	XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;

	gboolean active;

	active = gtk_toggle_action_get_active (action);

	if (!active && priv->statusbar != NULL)
		{
			gtk_widget_destroy (priv->statusbar);
			priv->statusbar = NULL;
		}
	else if (active && priv->statusbar == NULL)
		{
			priv->statusbar = xfmpc_statusbar_new ();
			gtk_widget_show (priv->statusbar);
			gtk_box_pack_start (GTK_BOX (priv->vbox), priv->statusbar, FALSE, FALSE, 0);
		}
}

static void
xfmpc_main_window_update_statusbar (XfmpcMainWindow *window)
{
	XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;
	gchar    *text;
	gint      seconds, length;

	if (G_UNLIKELY (priv->statusbar == NULL))
		return;

	if (!xfmpc_mpdclient_is_connected (window->mpdclient))
		return;

	length = xfmpc_mpdclient_playlist_get_length (window->mpdclient);
	seconds = xfmpc_mpdclient_playlist_get_total_time (window->mpdclient);

	if (seconds / 3600 > 0)
		text = g_strdup_printf (_("%d songs, %d hours and %d minutes"), length, seconds / 3600, (seconds / 60) % 60);
	else
		text = g_strdup_printf (_("%d songs, %d minutes"), length, (seconds / 60) % 60);

	g_object_set (G_OBJECT (priv->statusbar), "text", text, NULL);
	g_free (text);
}

static void
cb_playlist_changed (XfmpcMainWindow *window)
{
	xfmpc_main_window_update_statusbar (window);
}

static void
cb_show_statusbar_changed (XfmpcMainWindow *window,
													GParamSpec *pspec)
{
	XfmpcMainWindowPrivate *priv = window->priv;
	gboolean active;
	GtkAction *action;

	action = gtk_action_group_get_action (priv->action_group, "view-statusbar");
	g_object_get (window->preferences, "show-statusbar", &active, NULL);

	gtk_toggle_action_set_active (GTK_TOGGLE_ACTION (action), active);
	xfmpc_main_window_update_statusbar (window);
}

static gboolean
cb_button_pressed (XfmpcMainWindow *window, GdkEventButton *event)
{
	//XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;
	if (event->type != GDK_BUTTON_PRESS || event->button != 3)
		return FALSE;

	cb_popup_menu (window);
	return TRUE;
}

static void
cb_popup_menu (XfmpcMainWindow *window)
{
	XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;
	gtk_menu_popup (GTK_MENU (priv->menu),
									NULL, NULL,
									NULL, NULL,
									0,
									gtk_get_current_event_time ());
}

static void
cb_random (XfmpcMainWindow *window, gboolean random)
{
	XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;
	GTK_CHECK_MENU_ITEM(priv->menu_item_random)->active = random;
}

static void
cb_repeat (XfmpcMainWindow *window, gboolean repeat)
{
	XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;
	GTK_CHECK_MENU_ITEM(priv->menu_item_repeat)->active = repeat;
}

static void
cb_repeat_switch (XfmpcMainWindow *window)
{
	xfmpc_mpdclient_set_repeat (window->mpdclient, !xfmpc_mpdclient_get_repeat (window->mpdclient));
}

static void
cb_random_switch (XfmpcMainWindow *window)
{
	xfmpc_mpdclient_set_random (window->mpdclient, !xfmpc_mpdclient_get_random (window->mpdclient));
}

static void
cb_close_to_tray_changed (XfmpcMainWindow *window)
{
	XfmpcMainWindowPrivate *priv = XFMPC_MAIN_WINDOW (window)->priv;
	g_object_get (G_OBJECT (window->preferences),
								"close-to-tray", &priv->close_to_tray,
								NULL);
	g_signal_handler_disconnect (G_OBJECT (window), priv->close_to_tray_signal);
	if(priv->close_to_tray)
		priv->close_to_tray_signal =
			g_signal_connect (G_OBJECT (window), "delete-event",
				G_CALLBACK (gtk_widget_hide_on_delete), NULL);
	else
		priv->close_to_tray_signal =
			g_signal_connect (G_OBJECT (window), "delete-event",
				G_CALLBACK (cb_window_closed), NULL);
}
