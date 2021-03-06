/*
 *      etc.c
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

#include "etc.h"

void
gtk_widget_set_visible (GtkWidget *widget, gboolean condition)
{
  switch(condition)
  {
    case TRUE:  gtk_widget_show (widget); break;
    case FALSE: gtk_widget_hide (widget); break;
  }
}
