/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * Ardesia is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * Ardesia is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */


#include <gdk/gdkinput.h>

#include <gtk/gtk.h>


/* Expose event: this occurs when the windows is show */
G_MODULE_EXPORT gboolean
event_expose (GtkWidget *widget, 
              GdkEventExpose *event, 
              gpointer func_data);


/* This is called when the button is pushed */
G_MODULE_EXPORT gboolean
paint (GtkWidget *win,
       GdkEventButton *ev, 
       gpointer func_data);


/* This shots when the ponter is moving */
G_MODULE_EXPORT gboolean
paintto (GtkWidget *win, 
         GdkEventMotion *ev, 
         gpointer func_data);


/* This shots when the button is realeased */
G_MODULE_EXPORT gboolean
paintend (GtkWidget *win, GdkEventButton *ev, gpointer func_data);


/* Device touch */
G_MODULE_EXPORT gboolean
proximity_in (GtkWidget *win,
              GdkEventProximity *ev, 
              gpointer func_data);


/* Device lease */
G_MODULE_EXPORT gboolean
proximity_out (GtkWidget *win, 
               GdkEventProximity *ev,
               gpointer func_data);



