/*
 *  Copyright (c) 2009 Vincent Legout <vincent@legout.info>
 *  Copyright (c) 2009 Mike Massonnet <mmassonnet@xfce.org>
 *  Copyright (c) 2009 Peter Savichev <psavichev@gmail.com>
 *
 *  Based on ThunarPreferencesDialog:
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

#include "etc.h"

#include <gtk/gtk.h>

#include "mpdclient.h"
#include "preferences-dialog.h"
#include "preferences.h"
#include "statusbar.h"

#define GET_PRIVATE(o) \
		(G_TYPE_INSTANCE_GET_PRIVATE ((o), XFMPC_TYPE_PREFERENCES_DIALOG, XfmpcPreferencesDialogPrivate))



static void               xfmpc_preferences_dialog_class_init (XfmpcPreferencesDialogClass *klass);
static void               xfmpc_preferences_dialog_init       (XfmpcPreferencesDialog *dialog);
static void               xfmpc_preferences_dialog_finalize   (GObject *object);

static void               xfmpc_preferences_dialog_response   (GtkDialog *dialog,
																															gint response);
static void               cb_use_defaults_toggled             (GtkToggleButton *button,
																															GtkWidget *widget);
static void               cb_update_mpd                       (GtkButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_show_statusbar_toggled           (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_format_combo_changed             (GtkComboBox *combo,
																															XfmpcPreferencesDialog *dialog);
static void               cb_show_notify_toggled              (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_close_to_tray_toggled            (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_show_tray_toggled                (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_show_combobox_toggled            (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_show_tabs_toggled                (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);
static void               cb_db_elem_bold_toggled             (GtkToggleButton *button,
																															XfmpcPreferencesDialog *dialog);

struct _XfmpcPreferencesDialogClass
{
	GtkDialogClass             parent_class;
};

struct _XfmpcPreferencesDialog
{
	GtkDialog                  parent;
	XfmpcPreferences                 *preferences;
	/*<private>*/
	XfmpcPreferencesDialogPrivate    *priv;
};

struct _XfmpcPreferencesDialogPrivate
{
	GtkWidget                        *entry_use_defaults;
	GtkWidget                        *entry_host;
	GtkWidget                        *entry_port;
	GtkWidget                        *entry_passwd;
	GtkWidget                        *statusbar_button;
	GtkWidget                        *entry_format;
	GtkWidget                        *combo_format;
	GtkWidget                        *fchooser_libdir;
	GtkWidget                        *entry_cover;
	GtkWidget                        *tray_show_button;
	GtkWidget                        *tray_close_button;
	GtkWidget                        *notify_button;
	GtkWidget                        *combox_show_button;
	GtkWidget                        *tab_show_button;
	GtkWidget                        *db_elem_bold_button;

	guint                             format_timeout;
	gboolean                          is_format;
};

static GObjectClass *parent_class = NULL;

GType
xfmpc_preferences_dialog_get_type (void)
{
	static GType xfmpc_preferences_dialog_type = G_TYPE_INVALID;

	if (G_UNLIKELY (xfmpc_preferences_dialog_type == G_TYPE_INVALID))
		{
			static const GTypeInfo xfmpc_preferences_dialog_info =
			{
				sizeof (XfmpcPreferencesDialogClass),
				NULL,
				NULL,
				(GClassInitFunc) xfmpc_preferences_dialog_class_init,
				NULL,
				NULL,
				sizeof (XfmpcPreferencesDialog),
				0,
				(GInstanceInitFunc) xfmpc_preferences_dialog_init,
				NULL,
			};

			xfmpc_preferences_dialog_type = g_type_register_static (GTK_TYPE_DIALOG, "XfmpcPreferencesDialog", &xfmpc_preferences_dialog_info, 0);
		}

	return xfmpc_preferences_dialog_type;
}



static void
xfmpc_preferences_dialog_class_init (XfmpcPreferencesDialogClass *klass)
{
	GtkDialogClass *gtkdialog_class;
	GObjectClass   *gobject_class;

	g_type_class_add_private (klass, sizeof (XfmpcPreferencesDialogPrivate));

	parent_class = g_type_class_peek_parent (klass);

	gobject_class = G_OBJECT_CLASS (klass);
	gobject_class->finalize = xfmpc_preferences_dialog_finalize;

	gtkdialog_class = GTK_DIALOG_CLASS (klass);
	gtkdialog_class->response = xfmpc_preferences_dialog_response;
}

static void
xfmpc_preferences_dialog_init (XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv = GET_PRIVATE (dialog);

	GtkWidget *notebook;
	GtkWidget *vbox, *vbox2, *hbox;
	GtkWidget *mpd_vbox;
	GtkWidget *frame;
	GtkWidget *label;
	GtkWidget *button;
	GtkWidget *table;

	gchar *host, *passwd;
	guint port;
	gboolean use_defaults;
	gboolean statusbar;
	XfmpcSongFormat song_format;
	gchar *format_custom;
	gchar *music_dir;
	gchar *cover_name;
	gboolean show_tray;
	gboolean close_to_tray;
	gboolean show_notify;
	gboolean show_combobox;
	gboolean show_tabs;
	gboolean db_bold;

	dialog->preferences = xfmpc_preferences_get ();

	g_object_get (dialog->preferences,
								"mpd-hostname", &host,
								"mpd-port", &port,
								"mpd-password", &passwd,
								"mpd-use-defaults", &use_defaults,
								"show-statusbar", &statusbar,
								"song-format", &song_format,
								"song-format-custom", &format_custom,
								"library-dir", &music_dir,
								"cover-filename", &cover_name,
								"tray-icon", &show_tray,
								"close-to-tray", &close_to_tray,
								"show-notify", &show_notify,
								"combobox", &show_combobox,
								"tab-headers", &show_tabs,
								"db-bold", &db_bold,
								NULL);

	//GtkWidget                        *tray_show_button;
	//GtkWidget                        *tray_close_button;
	//GtkWidget                        *notify_button;

	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_position (GTK_WINDOW (dialog), GTK_WIN_POS_CENTER_ON_PARENT);
	gtk_window_set_skip_taskbar_hint (GTK_WINDOW (dialog), TRUE);
	gtk_window_set_icon_name (GTK_WINDOW (dialog), "stock_volume");
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_window_set_title (GTK_WINDOW (dialog), _("Xfmpc Preferences"));

	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_OK, GTK_STOCK_OK, NULL);
	gtk_dialog_add_buttons (GTK_DIALOG (dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);

	notebook = gtk_notebook_new ();
	gtk_container_set_border_width (GTK_CONTAINER (notebook), 6);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), notebook, TRUE, TRUE, 0);
	gtk_widget_show (notebook);

	/*
		MPD settings
	 */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	label = gtk_label_new (_("MPD"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	vbox2 = gtk_vbox_new (FALSE, 8);
	frame = gtk_frame_new(_("Connection"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
	//frame = xfce_create_framebox_with_content (_("Connection"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	priv->entry_use_defaults = gtk_check_button_new_with_mnemonic (_("Use _default system settings"));
	gtk_widget_set_tooltip_text (priv->entry_use_defaults,
															_("If checked, Xfmpc will try to read the environment "
																"variables MPD_HOST and MPD_PORT otherwise it will "
																"use localhost"));
	gtk_container_add (GTK_CONTAINER (vbox2), priv->entry_use_defaults);

	GtkWidget *alignment = gtk_alignment_new (0., 0., 1., 1.);
	gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 8, 8, 16, 0);
	gtk_container_add (GTK_CONTAINER (vbox2), alignment);

	mpd_vbox = gtk_vbox_new (FALSE, 8);
	gtk_container_add (GTK_CONTAINER (alignment), mpd_vbox);

	g_signal_connect (priv->entry_use_defaults, "toggled",
										G_CALLBACK (cb_use_defaults_toggled), mpd_vbox);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->entry_use_defaults), use_defaults);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (mpd_vbox), hbox);

	label = gtk_label_new (_("Hostname:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	priv->entry_host = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->entry_host), 15);
	gtk_entry_set_text (GTK_ENTRY (priv->entry_host), host);
	gtk_box_pack_start (GTK_BOX (hbox), priv->entry_host, TRUE, TRUE, 0);

	label = gtk_label_new (_("Port:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	priv->entry_port = gtk_spin_button_new_with_range (0, 65536, 1);
	gtk_spin_button_set_digits (GTK_SPIN_BUTTON (priv->entry_port), 0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (priv->entry_port), port);
	gtk_box_pack_start (GTK_BOX (hbox), priv->entry_port, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (mpd_vbox), hbox);

	label = gtk_label_new (_("Password:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	priv->entry_passwd = gtk_entry_new ();
	gtk_entry_set_visibility (GTK_ENTRY (priv->entry_passwd), FALSE);
	if (passwd != NULL)
		gtk_entry_set_text (GTK_ENTRY (priv->entry_passwd), passwd);
	gtk_box_pack_start (GTK_BOX (hbox), priv->entry_passwd, TRUE, TRUE, 0);

	button = gtk_button_new_from_stock (GTK_STOCK_APPLY);
	g_signal_connect (button, "clicked",
										G_CALLBACK (cb_update_mpd), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), button);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Library"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Library"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (vbox2), hbox);
	label = gtk_label_new (_("Music directory:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	priv->fchooser_libdir = gtk_file_chooser_button_new (_("Select a music directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
	gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (priv->fchooser_libdir), music_dir);
	gtk_box_pack_start (GTK_BOX (hbox), priv->fchooser_libdir, FALSE, FALSE, 0);

	/*
		Interface
	 */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	label = gtk_label_new (_("Interface"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Tab's change"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Tab's change"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	priv->combox_show_button = gtk_check_button_new_with_label (_("Show the combobox"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->combox_show_button), show_combobox);
	g_signal_connect (priv->combox_show_button, "toggled",
										G_CALLBACK (cb_show_combobox_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->combox_show_button);

	priv->tab_show_button = gtk_check_button_new_with_label (_("Show the tab's headers"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->tab_show_button), show_tabs);
	g_signal_connect (priv->tab_show_button, "toggled",
										G_CALLBACK (cb_show_tabs_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->tab_show_button);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Tray icon"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Tray icon"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	priv->tray_show_button = gtk_check_button_new_with_label (_("Show the tray icon"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->tray_show_button), show_tray);
	g_signal_connect (priv->tray_show_button, "toggled",
										G_CALLBACK (cb_show_tray_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->tray_show_button);

	priv->tray_close_button = gtk_check_button_new_with_label (_("Close to tray"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->tray_close_button), close_to_tray);
	g_signal_connect (priv->tray_close_button, "toggled",
										G_CALLBACK (cb_close_to_tray_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->tray_close_button);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Notifications"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Notifications"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	priv->notify_button = gtk_check_button_new_with_label (_("Show notifications"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->notify_button), show_notify);
	g_signal_connect (priv->notify_button, "toggled",
										G_CALLBACK (cb_show_notify_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->notify_button);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Statusbar"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Statusbar"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	priv->statusbar_button = gtk_check_button_new_with_mnemonic (_("Show _statusbar"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->statusbar_button), statusbar);
	g_signal_connect (priv->statusbar_button, "toggled",
										G_CALLBACK (cb_show_statusbar_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->statusbar_button);

	/*
		Display
	 */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 6);
	label = gtk_label_new (_("Appearance"));
	gtk_notebook_append_page (GTK_NOTEBOOK (notebook), vbox, label);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Song Format"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Song Format"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 2);

	label = gtk_label_new (_("Song format:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	priv->combo_format = gtk_combo_box_new_text ();
	gtk_box_pack_start (GTK_BOX (hbox), priv->combo_format, TRUE, TRUE, 0);

	gint i;
	GEnumClass *klass = g_type_class_ref (XFMPC_TYPE_SONG_FORMAT);
	for (i = 0; i < klass->n_values; i++)
		{
			gtk_combo_box_append_text (GTK_COMBO_BOX (priv->combo_format),
																_(klass->values[i].value_nick));
		}
	gtk_combo_box_set_active (GTK_COMBO_BOX (priv->combo_format), song_format);
	g_type_class_unref (klass);

	gtk_container_add (GTK_CONTAINER (vbox2), hbox);

	hbox = gtk_hbox_new (FALSE, 2);

	label = gtk_label_new (_("Custom format:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	priv->entry_format = gtk_entry_new ();
	gtk_entry_set_width_chars (GTK_ENTRY (priv->entry_format), 15);
	gtk_entry_set_max_length (GTK_ENTRY (priv->entry_format), 30);
	gtk_entry_set_text (GTK_ENTRY (priv->entry_format), format_custom);
	gtk_box_pack_start (GTK_BOX (hbox), priv->entry_format, TRUE, TRUE, 0);
	g_signal_connect (priv->combo_format, "changed",
										G_CALLBACK (cb_format_combo_changed), dialog);
	gtk_widget_set_sensitive (priv->entry_format,
														song_format == XFMPC_SONG_FORMAT_CUSTOM);

	gtk_container_add (GTK_CONTAINER (vbox2), hbox);

	label = gtk_label_new (_("Available parameters:"));
	gtk_container_add (GTK_CONTAINER (vbox2), label);

	table = gtk_table_new (4, 6, TRUE);

	PangoAttrList *attrs = pango_attr_list_new ();
	PangoAttribute *attr = pango_attr_scale_new (PANGO_SCALE_SMALL);
	pango_attr_list_insert (attrs, attr);

	label = gtk_label_new (_("%a: Artist"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, 0, 1);
	label = gtk_label_new (_("%A: Album"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 6, 0, 1);

	label = gtk_label_new (_("%d: Date"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, 1, 2);
	label = gtk_label_new (_("%D: Disc"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 6, 1, 2);

	label = gtk_label_new (_("%f: File"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, 2, 3);
	label = gtk_label_new (_("%g: Genre"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 6, 2, 3);

	label = gtk_label_new (_("%t: Title"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 1, 3, 3, 4);
	label = gtk_label_new (_("%T: Track"));
	gtk_label_set_attributes (GTK_LABEL (label), attrs);
	gtk_misc_set_alignment (GTK_MISC (label), 0., 0.5);
	gtk_table_attach_defaults (GTK_TABLE (table), label, 4, 6, 3, 4);

	pango_attr_list_unref (attrs);

	gtk_container_add (GTK_CONTAINER (vbox2), table);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Cover"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Cover"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);
	hbox = gtk_hbox_new (FALSE, 2);
	gtk_container_add (GTK_CONTAINER (vbox2), hbox);
	label = gtk_label_new (_("Cover filename:"));
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	priv->entry_cover = gtk_entry_new ();
	gtk_entry_set_text (GTK_ENTRY (priv->entry_cover), cover_name);
	gtk_box_pack_start (GTK_BOX (hbox), priv->entry_cover, FALSE, FALSE, 0);

	vbox2 = gtk_vbox_new (FALSE, 6);
	frame = gtk_frame_new(_("Database Browser"));
	gtk_container_add (GTK_CONTAINER (frame), vbox2);
		//frame = xfce_create_framebox_with_content (_("Database Browser"), vbox2);
	gtk_box_pack_start (GTK_BOX (vbox), frame, FALSE, FALSE, 0);

	priv->db_elem_bold_button = gtk_check_button_new_with_label (_("Highlight played songs in dbbrowser"));
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->db_elem_bold_button), db_bold);
	gtk_widget_set_tooltip_text (priv->db_elem_bold_button, _("Warning: Slow, when your playlist is big!"));
	g_signal_connect (priv->db_elem_bold_button, "toggled",
										G_CALLBACK (cb_db_elem_bold_toggled), dialog);
	gtk_container_add (GTK_CONTAINER (vbox2), priv->db_elem_bold_button);

	gtk_widget_show_all (GTK_DIALOG (dialog)->vbox);

	g_free (host);
	g_free (passwd);
	g_free (format_custom);
	g_free (music_dir);
	g_free (cover_name);
}

static void
xfmpc_preferences_dialog_finalize (GObject *object)
{
	XfmpcPreferencesDialog *dialog = XFMPC_PREFERENCES_DIALOG (object);
	g_object_unref (G_OBJECT (dialog->preferences));
	G_OBJECT_CLASS (parent_class)->finalize (object);
}



static void
xfmpc_preferences_dialog_response (GtkDialog *dialog,
																	gint       response)
{
	XfmpcPreferencesDialogPrivate *priv = XFMPC_PREFERENCES_DIALOG (dialog)->priv;
	XfmpcPreferences *preferences = XFMPC_PREFERENCES_DIALOG (dialog)->preferences;
	switch(response)
	{
		case GTK_RESPONSE_OK:
			g_object_set (G_OBJECT (preferences),
							"song-format", gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo_format)),
							"song-format-custom", gtk_entry_get_text (GTK_ENTRY (priv->entry_format)),
							"cover-filename", gtk_entry_get_text (GTK_ENTRY (priv->entry_cover)),
							"mpd-use-defaults", gtk_file_chooser_get_current_folder (GTK_FILE_CHOOSER (priv->fchooser_libdir)),
							NULL);
			break;
		default: break;
	}
	gtk_widget_destroy (GTK_WIDGET (dialog));
}



GtkWidget*
xfmpc_preferences_dialog_new ()
{
	return g_object_new (XFMPC_TYPE_PREFERENCES_DIALOG, NULL);
}

static void
cb_use_defaults_toggled (GtkToggleButton *button,
												GtkWidget *widget)
{
	gboolean sensitive;
	sensitive = gtk_toggle_button_get_active (button);
	gtk_widget_set_sensitive (widget, !sensitive);
}

static void
cb_update_mpd (GtkButton *button,
							XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;
	XfmpcMpdclient *mpdclient = xfmpc_mpdclient_get ();

	g_object_set (G_OBJECT (dialog->preferences),
								"mpd-hostname", gtk_entry_get_text (GTK_ENTRY (priv->entry_host)),
								"mpd-port", gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (priv->entry_port)),
								"mpd-password", gtk_entry_get_text (GTK_ENTRY (priv->entry_passwd)),
								"mpd-use-defaults", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->entry_use_defaults)),
								NULL);

	xfmpc_mpdclient_disconnect (mpdclient);
	xfmpc_mpdclient_connect (mpdclient);
}

static void
cb_show_statusbar_toggled (GtkToggleButton *button,
													XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	g_object_set (G_OBJECT (dialog->preferences),
								"show-statusbar", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->statusbar_button)),
								NULL);
}

static void
cb_show_tray_toggled (GtkToggleButton *button,
													XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	gboolean tray_show = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->tray_show_button));
	gtk_widget_set_sensitive (priv->tray_close_button, tray_show);
	if(!tray_show) gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (priv->tray_close_button), FALSE);
	g_object_set (G_OBJECT (dialog->preferences), "tray-icon", tray_show, NULL);
}

static void
cb_close_to_tray_toggled (GtkToggleButton *button,
													XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	g_object_set (G_OBJECT (dialog->preferences),
								"close-to-tray", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->tray_close_button)),
								NULL);
}

static void
cb_show_notify_toggled (GtkToggleButton *button,
													XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	g_object_set (G_OBJECT (dialog->preferences),
								"show-notify", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->notify_button)),
								NULL);
}

static void
cb_format_combo_changed (GtkComboBox *combo, XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = XFMPC_PREFERENCES_DIALOG (dialog)->priv;
	XfmpcSongFormat song_format;
	song_format = gtk_combo_box_get_active (GTK_COMBO_BOX (priv->combo_format));
	gtk_widget_set_sensitive (priv->entry_format, song_format == XFMPC_SONG_FORMAT_CUSTOM);
}

static void
cb_show_combobox_toggled (GtkToggleButton *button,
													XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	g_object_set (G_OBJECT (dialog->preferences),
								"combobox", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->combox_show_button)),
								NULL);
}

static void
cb_show_tabs_toggled (GtkToggleButton *button,
											XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	g_object_set (G_OBJECT (dialog->preferences),
								"tab-headers", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->tab_show_button)),
								NULL);
}

static void
cb_db_elem_bold_toggled (GtkToggleButton *button,
											XfmpcPreferencesDialog *dialog)
{
	XfmpcPreferencesDialogPrivate *priv = dialog->priv;

	g_object_set (G_OBJECT (dialog->preferences),
								"db-bold", gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->db_elem_bold_button)),
								NULL);
}
