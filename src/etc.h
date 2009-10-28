#include <gtk/gtk.h>
#include <string.h>
#include <stdlib.h>
#include <libintl.h>

#define _(string) gettext (string)

void        gtk_widget_set_visible       (GtkWidget *widget, gboolean condition);
