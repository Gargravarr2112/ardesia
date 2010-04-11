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

/** Widget for text insertion */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <gdk/gdkinput.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <ctype.h>
#include <text_widget.h>
#include <utils.h>
#include <annotate.h>

#include <cairo.h>

#if defined(_WIN32)
	#include <cairo-win32.h>
#else
	#include <cairo-xlib.h>
#endif

#define TEXT_MOUSE_EVENTS        ( GDK_PROXIMITY_IN_MASK |      \
				   GDK_PROXIMITY_OUT_MASK |	\
				   GDK_POINTER_MOTION_MASK |	\
				   GDK_BUTTON_PRESS_MASK |      \
				   GDK_BUTTON_RELEASE_MASK      \
				   )

typedef struct
{
  int x;
  int y;
  int x_bearing;
  int y_bearing;
} CharInfo;


typedef struct
{
  int x;
  int y;
} Pos;

Pos* pos = NULL;

GdkDisplay* display = NULL;
GdkScreen* screen = NULL;
GtkWidget* text_window = NULL;
GdkCursor* text_cursor;

GSList *letterlist = NULL;

cairo_t *text_cr = NULL;
cairo_text_extents_t extents;

int text_pen_width = 15;
char* text_color = NULL;

int screen_width;

int max_font_height;

gboolean is_visible_cursor = FALSE;

int yoffset;


/* Creation of the text window */
void create_text_window(GtkWindow *parent)
{
  text_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_maximize(GTK_WINDOW(text_window));
  gtk_window_set_skip_taskbar_hint(GTK_WINDOW(text_window), TRUE);
  gtk_window_set_skip_pager_hint(GTK_WINDOW(text_window), TRUE);
  #ifndef _WIN32 
    gtk_window_set_opacity(GTK_WINDOW(text_window), 1); 
  #else
    // TODO In windows I am not able to use an rgba transparent  
    // windows and then I use a sort of semi transparency
    gtk_window_set_opacity(GTK_WINDOW(text_window), 0.5); 
  #endif  
  gtk_window_stick(GTK_WINDOW(text_window));
  gtk_window_set_decorated(GTK_WINDOW(text_window), FALSE);
  gtk_widget_set_app_paintable(text_window, TRUE);
  gtk_widget_set_double_buffered(text_window, FALSE);
  gtk_window_set_transient_for(GTK_WINDOW(text_window), get_annotation_window());
}


/* Move the pen cursor */
void move_editor_cursor()
{
  if (text_cr)
    {
       cairo_move_to(text_cr, pos->x, pos->y);
    }
}


/** Delete the last character printed */
void delete_character()
{
  CharInfo *charInfo = (CharInfo *) g_slist_nth_data (letterlist, 0);

  if (charInfo)
    {  

      cairo_set_operator (text_cr, CAIRO_OPERATOR_CLEAR);
 
      cairo_rectangle(text_cr, charInfo->x + charInfo->x_bearing, charInfo->y + charInfo->y_bearing, screen_width,  max_font_height + text_pen_width+3);
      cairo_fill(text_cr);
      cairo_stroke(text_cr);
      cairo_set_operator (text_cr, CAIRO_OPERATOR_SOURCE);
      pos->x = charInfo->x;
      pos->y = charInfo->y;
      move_editor_cursor();
      letterlist = g_slist_remove(letterlist,charInfo); 
    }
}


/* Press keyboard event */
G_MODULE_EXPORT gboolean
key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)  
{
 
  if ((event->keyval == GDK_BackSpace)
      || (event->keyval == GDK_Delete))
    {
      // undo
      delete_character();
      return FALSE;
    }
  /* is finished the line */
  if ((pos->x + extents.x_advance >= screen_width))
    {
      pos->x = 0;
      pos->y +=  max_font_height;

      /* go to the new line */
      move_editor_cursor();  
    }
  /* is pressed enter */
  else if ((event->keyval == GDK_Return) ||
	   (event->keyval == GDK_ISO_Enter) || 	
	   (event->keyval == GDK_KP_Enter))
    {
      stop_text_widget();
    } 
  else if (isprint(event->keyval))
    {
      /* The character is printable */
      char *utf8 = g_malloc(2) ;
      sprintf(utf8,"%c", event->keyval);
      
      CharInfo *charInfo = g_malloc (sizeof (CharInfo));
      charInfo->x = pos->x;
      charInfo->y = pos->y; 
      
      cairo_text_extents (text_cr, utf8, &extents);
      cairo_show_text (text_cr, utf8); 
      cairo_stroke(text_cr);
 
      charInfo->x_bearing = extents.x_bearing;
      charInfo->y_bearing = extents.y_bearing;
     
      letterlist = g_slist_prepend (letterlist, charInfo);
      
      /* move cursor to the x step */
      pos->x +=  extents.x_advance;
      move_editor_cursor();  
      
      g_free(utf8);
    }
  return TRUE;
}


/* Set the eraser cursor */
gboolean set_text_pointer(GtkWidget * window)
{
  int height = max_font_height;
  int width = text_pen_width;
  
  #ifdef _WIN32
    height=5;
    width=5;  
  #endif

  GdkPixmap *pixmap = gdk_pixmap_new (NULL, width ,height, 1);

  cairo_t *text_pointer_cr = gdk_cairo_create(pixmap);
  
  if (text_pointer_cr)
    {
       clear_cairo_context(text_pointer_cr);
  
       cairo_set_line_cap (text_pointer_cr, CAIRO_LINE_CAP_ROUND);
       cairo_set_line_join(text_pointer_cr, CAIRO_LINE_JOIN_ROUND); 
       cairo_set_operator(text_pointer_cr, CAIRO_OPERATOR_SOURCE);
       cairo_set_line_width(text_pointer_cr, text_pen_width);
       cairo_set_source_color_from_string(text_pointer_cr, text_color);
       cairo_rectangle(text_pointer_cr, 0,0, 5, height);  

       cairo_stroke(text_pointer_cr);
     } 
 
  GdkColor *background_color_p = rgb_to_gdkcolor("FFFFFF");
  GdkColor *foreground_color_p = rgb_to_gdkcolor(text_color);
  if (text_cursor)
    {
      gdk_cursor_unref(text_cursor);
    }
  text_cursor = gdk_cursor_new_from_pixmap (pixmap, pixmap, foreground_color_p, background_color_p, 0, height);
  gdk_window_set_cursor (text_window->window, text_cursor);
  gdk_flush ();
  g_object_unref (pixmap);
  g_free(foreground_color_p);
  g_free(background_color_p);
  cairo_destroy(text_pointer_cr);
  return TRUE;
}


/* The windows has been configured  */
G_MODULE_EXPORT gboolean
on_window_text_configure_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  return TRUE;
}


/* Initialization routine */
void init(GtkWidget *win)
{
  text_cr = gdk_cairo_create(win->window);
  
  if (text_cr)
    {
      cairo_set_operator(text_cr,CAIRO_OPERATOR_SOURCE);
      cairo_set_transparent_color(text_cr);
      cairo_paint(text_cr);
      cairo_stroke(text_cr);   
 
      cairo_set_line_cap (text_cr, CAIRO_LINE_CAP_ROUND);
      cairo_set_line_join(text_cr, CAIRO_LINE_JOIN_ROUND); 
      cairo_set_operator(text_cr, CAIRO_OPERATOR_SOURCE);
      cairo_set_line_width(text_cr, text_pen_width);
      cairo_set_source_color_from_string(text_cr, text_color);
    }  

  pos = g_malloc (sizeof(Pos));
  pos->x = text_pen_width;
  pos->y = text_pen_width;
  move_editor_cursor();
}


/* The windows has been exposed */
G_MODULE_EXPORT gboolean
on_window_text_expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  if (!display)
    {
      display = gdk_display_get_default ();
      screen = gdk_display_get_default_screen (display);
      screen_width = gdk_screen_get_width (screen);
    }
  if (widget)
    {
      int width, height;
      gtk_window_get_size(GTK_WINDOW(widget), &width, &height);
      int x, y;
      gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
      if (screen_width==width)
        {
          yoffset = y;
          init(widget);
       
          if (text_cr)
            {
              /* This is a trick we must found the maximum height and text_pen_width of the font */
              cairo_set_font_size (text_cr, text_pen_width * 2);
              cairo_text_extents (text_cr, "|" , &extents);
              max_font_height = extents.height;
            }
          
          if (text_window)
            {
              grab_pointer(text_window, TEXT_MOUSE_EVENTS);
            }
  
          set_text_pointer(widget);
        }
    }
  return TRUE;
}


/* This is called when the button is pushed */
G_MODULE_EXPORT gboolean
release (GtkWidget *win,
         GdkEventButton *ev, 
         gpointer user_data)
{
  ungrab_pointer(display, win);
  pos->x = ev->x;
  pos->y = ev->y;
  move_editor_cursor();
  return TRUE;
}


/* This shots when the ponter is moving */
G_MODULE_EXPORT gboolean
on_window_text_motion_notify_event (GtkWidget *win, 
			            GdkEventMotion *ev, 
			            gpointer user_data)
{
  GtkWidget *bar = GTK_WIDGET(gtk_builder_get_object(gtkBuilder, "winMain"));
  if (bar)
    {
      int x, y, width, height;
      gtk_window_get_position(GTK_WINDOW(bar), &x, &y);
      gtk_window_get_size(GTK_WINDOW(bar), &width, &height);
      /* rectangle that contains the panel */
      if ((ev->y>=y)&&(ev->y<y+height))
        {
           if ((ev->x>=x)&&(ev->x<x+width))
	     {
	        stop_text_widget();
	     }
        }
    }

  
  return TRUE;;
}

/* Start the widget for the text insertion */
void start_text_widget(GtkWindow *parent, char* color, int tickness)
{ 
  stop_text_widget();
  text_pen_width = tickness;
  text_color = color;
  create_text_window(parent);
      
  gtk_widget_set_events (text_window, GDK_EXPOSURE_MASK
			 | GDK_BUTTON_PRESS_MASK);

  g_signal_connect(G_OBJECT(text_window), "configure-event", G_CALLBACK(on_window_text_configure_event), NULL);
  g_signal_connect(G_OBJECT(text_window), "expose-event", G_CALLBACK(on_window_text_expose_event), NULL);
  g_signal_connect (G_OBJECT(text_window), "motion_notify_event",G_CALLBACK (on_window_text_motion_notify_event), NULL);
  g_signal_connect (G_OBJECT(text_window), "button_release_event", G_CALLBACK(release), NULL);
  g_signal_connect (G_OBJECT(text_window), "key_press_event", G_CALLBACK (key_press), NULL);

  gtk_widget_show_all(text_window);
 
}


/* Stop the text insertion widget */
void stop_text_widget()
{
  if (text_cr)
    {
      if (letterlist)
        {
          g_slist_foreach(letterlist, (GFunc)g_free, NULL);
          g_slist_free(letterlist);
          letterlist = NULL;
          merge_context(text_cr, yoffset);
        } 
      cairo_destroy(text_cr);     
      text_cr = NULL;
    }
  if (text_window)
    {
      gtk_widget_destroy(text_window);
      text_window= NULL;
    }
  if (text_cursor)
    {
      gdk_cursor_unref(text_cursor);
      text_cursor = NULL;
    }
  if (pos)
    {
      g_free(pos);
      pos = NULL;
    }
}
