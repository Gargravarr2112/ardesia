/* 
 * Ardesia -- a program for painting on the screen
 * with this program you can play, draw, learn and teach
 * This program has been written such as a freedom sonet
 * We believe in the freedom and in the freedom of education
 *
 * Copyright (C) 2009 Pilolli Pietro <pilolli@fbk.eu>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */
 
/* This is the file where put the callback of the events generated by the ardesia interface
 * If you are trying to add some functionality and you have already add a button in interface
 * you are in the right place. In any case good reading!
 * Pay attentioni; all the things are delegated to the annotate commandline tool
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

#include "interface.h"

#include "stdlib.h"
#include "unistd.h"
#include <string.h> 

#include "stdio.h"
#include "time.h"
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <gconf/gconf-client.h>

#include <math.h>
#include <pngutils.h>

#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>


/* annotation is visible */
gboolean     visible = TRUE;

/* pencil is selected */
gboolean     pencil = TRUE;

/* selected color in RGB format without the unusefull # prefix */
gchar*        color="FF0000";

/* old picked color in RGB format without the unusefull # prefix */
gchar*        picked_color=NULL;

/* selected line width */
int          tickness=15;

/* pid of the ffmpeg process for the recording */
int ffmpegpid = -1;

/* pid of the current running instance of the annotate client */
int annotateclientpid = -1;

/* arrow=0 mean no arrow, arrow=1 mean normal arrow, arrow=2 mean double arrow */
char* arrow = "0";

/* Preference dialog */
GtkBuilder *dialogGtkBuilder;

/* 0 no background, 1 background color, 2 png background, */
int background = 0;

/* Default folder where store images and videos */
gchar* workspace_dir = NULL;

/* 
 * Create a annotate client process the annotate
 * that talk with the server process 
 */
int 
callAnnotate(char *arg1, char* arg2, char* arg3, char* arrow)
{

  if (annotateclientpid != -1)
    {
      kill(annotateclientpid,9);
    }
  pid_t pid;

  pid = fork();

  if (pid < 0)
    {
      return -1;
    }
  if (pid == 0)
    {
      char* annotate="annotate";
      char* annotatebin = (char*) malloc(160*sizeof(char));
      sprintf(annotatebin,"%s/../bin/%s", PACKAGE_DATA_DIR, annotate);
      execl(annotatebin, annotate, arg1, arg2, arg3, arrow, NULL);
      free(annotatebin);
      _exit(127);
    }
  annotateclientpid = pid;
  return pid;

}

/* Return if the recording is active */
gboolean is_recording()
{

  if (ffmpegpid==-1)
    {
      return FALSE;
    }
  return TRUE;

}

/* Called when close the program */
gboolean  quit()
{

  extern int annotatepid;
  gboolean ret=FALSE;
  if(is_recording())
    {
      kill(ffmpegpid,9);
    }       
  if (annotateclientpid != -1)
    {
      kill(annotateclientpid,9);
    }
  kill(annotatepid,9);
  remove_background();
  /* Disalloc */
  g_object_unref ( G_OBJECT(gtkBuilder) ); 

  gtk_main_quit();; 
  exit(ret);

}

/* Start to paint calling annotate */
void paint()
{

  pencil=TRUE;
  char* ticknessa = (char*) malloc(16*sizeof(char));
  sprintf(ticknessa,"%d",tickness);

  callAnnotate("--toggle", color, ticknessa, arrow);   
 
  free(ticknessa);

}


/* Start to erase calling annotate */
void erase()
{

  pencil=FALSE;
  char* ticknessa = (char*) malloc(16*sizeof(char));
  sprintf(ticknessa,"%d",tickness);

  callAnnotate("--eraser", ticknessa, NULL, NULL); 
  
  free(ticknessa);
   
}

/* 
 * Create a annotate client process the annotate
 * that talk with the server process 
 */
int startFFmpeg(char *argv[])
{

  pid_t pid;

  pid = fork();

  if (pid < 0)
    {
      return -1;
    }
  if (pid == 0)
    {
      execvp(argv[0], argv);
    }
  return pid;

}

/*
 * Get the desktop folder;
 * Now this function use gconf to found the folder,
 * this means that this rutine works properly only
 * with the gnome desktop environment
 * We can investigate how-to do this
 * in a desktop environment independant way
 */
const gchar * get_desktop_dir (void)
{

  GConfClient *gconf_client = NULL;
  gboolean desktop_is_home_dir = FALSE;
  const gchar* desktop_dir = NULL;

  gconf_client = gconf_client_get_default ();
  desktop_is_home_dir = gconf_client_get_bool (gconf_client,
                                               "/apps/nautilus/preferences/desktop_is_home_dir",
                                               NULL);
  if (desktop_is_home_dir)
    {
      desktop_dir = g_get_home_dir ();
    }
  else
    {
      desktop_dir = g_get_user_special_dir (G_USER_DIRECTORY_DESKTOP);
    }

  g_object_unref (gconf_client);

  return desktop_dir;

}


/* Get the current date and format in a printable format */
char* get_date()
{

  struct tm* t;
  time_t now;
  time( &now );
  t = localtime( &now );

  char* date = malloc(64*sizeof(char));
  sprintf(date, "%d-%d-%d_%d:%d:%d", t->tm_mday,t->tm_mon+1,t->tm_year+1900,t->tm_hour,t->tm_min,t->tm_sec);
  return date;

}


/* 
 * This function is called when the thick property is changed;
 * start paint with pen or eraser depending on the selected tool
 */
void thick()
{

  if (pencil)
    {
      paint();
    }
  else
    {
      erase();
    }

}


/* Hide/Unhide the annotation depending on the status of the screen */
void hide_unhide()
{

  callAnnotate("--visibility", NULL, NULL, NULL );

}


/* Take aGdkColor and return the RGB tring */
char *
gdkcolor_to_rgb(GdkColor* gdkcolor)
{

  char*   ret= malloc(7*sizeof(char));;
  /* transform in the  RGB format e.g. FF0000 */ 
  sprintf(ret,"%02x%02x%02x", gdkcolor->red/257, gdkcolor->green/257, gdkcolor->blue/257);
  return ret;

}

/* Return if a file exists */
gboolean file_exists(char* filename, char* desktop_dir)
{

  char* afterslash = strrchr(filename, '/');
  if (afterslash==0)
    {
      /* relative path */
      filename = strcat(filename,desktop_dir);
    }
  struct stat statbuf;
  if(stat(filename, &statbuf) < 0) {
    if(errno == ENOENT) {
      return FALSE;
    } else {
      perror("");
      exit(0);
    }
  }
  printf("filename %s exists \n", filename);
  return TRUE;

}

/*
 * Start the dialog that ask to the user where save the video
 * containing the screencast
 * This function take as input the recor toolbutton in ardesia bar
 */
void start_save_video_dialog(GtkToolButton   *toolbutton)
{

   
  char * date = get_date();
  if (workspace_dir==NULL)
    {
      workspace_dir = (char *) get_desktop_dir();
    }	

  /* I hide the annotation to not disturb the user */
  hide_unhide();
  GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save video as mpeg", NULL , GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  gtk_window_stick((GtkWindow*)chooser);
  
  gtk_window_set_title (GTK_WINDOW (chooser), "Select a file");
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), workspace_dir);
  gchar* filename =  malloc(256*sizeof(char));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {
      char* screen_dimension=malloc(16*sizeof(char));
      gint screen_height = gdk_screen_height ();
      gint screen_width = gdk_screen_width ();
      sprintf(screen_dimension,"%dx%d", screen_width, screen_height);

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));

      char* extension = strrchr(filename, '.');
      if ((extension==0) || (strcmp(extension, ".mpeg") != 0))
	{
	  filename = (gchar *) realloc(filename,  (strlen(filename) + 5 + 1) * sizeof(gchar));
	  (void) strcat((gchar *)filename, ".mpeg");
	}
      free(extension);
 
      if (file_exists(filename, (char *) workspace_dir) == TRUE)
	{
	  GtkWidget *msg_dialog; 
                   
	  msg_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,  GTK_BUTTONS_YES_NO, "File Exists. Overwrite");

	  gtk_window_stick((GtkWindow*)msg_dialog);
                 
          gint result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
          if (msg_dialog!=NULL)
            {
	      gtk_widget_destroy(msg_dialog);
            }
	  if ( result  == GTK_RESPONSE_NO)
	    { 
	      g_free(filename);
	      return; 
	    } 
	}
      char* argv[11];
      argv[0] = "ffmpeg";
      argv[1] = "-f";
      argv[2] = "x11grab";
      argv[3] = "-r";
      argv[4] = "25";
      argv[5] = "-s";
      argv[6] = screen_dimension;
      argv[7] = "-i";
      argv[8] = ":0.0";
      argv[9] = filename;
      argv[10] = (char*) NULL ;
      ffmpegpid = startFFmpeg(argv);
      g_free(screen_dimension);
      /* set stop tooltip */ 
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Stop");
      /* put icon to stop */
      gtk_tool_button_set_stock_id (toolbutton, "gtk-media-stop");
    }
    if (chooser!=NULL)
      { 
        gtk_widget_destroy (chooser);
      } 
      
  g_free(filename);
  g_free(date);
 
} 


/* Start event handler section */


gboolean
on_quit                                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  return quit();

}


gboolean
on_winMain_delete_event                (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{

  return quit();

}


void
on_toolsEraser_activate                (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  erase();

}


void
on_toolsArrow_activate               (GtkToolButton   *toolbutton,
				      gpointer         user_data)
{

  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if single arrow is active release it */
      GtkToggleToolButton* doubleArrowToolButton = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(gtkBuilder,"buttonDoubleArrow"));
      if (gtk_toggle_tool_button_get_active(doubleArrowToolButton))
        {
	  gtk_toggle_tool_button_set_active(doubleArrowToolButton, FALSE); 
        }
      arrow="1";
    }
  else
    {
      arrow="0";
    }
  if (pencil)
    {
      paint();
    }
  else
    {
      erase();
    }

}

void
on_toolsDoubleArrow_activate               (GtkToolButton   *toolbutton,
					    gpointer         user_data)
{

  if (gtk_toggle_tool_button_get_active(GTK_TOGGLE_TOOL_BUTTON(toolbutton)))
    {
      /* if single arrow is active release it */
      GtkToggleToolButton* arrowToolButton = GTK_TOGGLE_TOOL_BUTTON(gtk_builder_get_object(gtkBuilder,"buttonArrow"));
      if (gtk_toggle_tool_button_get_active(arrowToolButton))
        {
	  gtk_toggle_tool_button_set_active(arrowToolButton, FALSE); 
        }
      arrow="2";
    }
  else
    {
      arrow="0";
    }
  if (pencil)
    {
      paint();
    }
  else
    {
      erase();
    }

}


void
on_toolsVisible_activate               (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
  hide_unhide();
  if (visible)
    {
      visible=FALSE;
      /* set tooltip to unhide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Unhide");
    }
  else
    {
      visible=TRUE;
      /* set tooltip to hide */
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Hide");
    }
}


void
on_toolsScreenShot_activate	       (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  char * date = get_date();
  if (workspace_dir==NULL)
    {
      workspace_dir = (char *) get_desktop_dir();
    }	

  /* I hide the annotation to not disturb the user */
  hide_unhide();
  GtkWidget *chooser = gtk_file_chooser_dialog_new ("Save image as image", NULL, GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE_AS, GTK_RESPONSE_ACCEPT,
						    NULL);
  /* with this apperar in all the workspaces */  
  gtk_window_stick((GtkWindow*) chooser);
 
  gtk_window_set_title (GTK_WINDOW (chooser), "Select a file");
  gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), workspace_dir);
  
  gchar* filename =  malloc(256*sizeof(char));
  sprintf(filename,"ardesia_%s", date);
  gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER(chooser), filename);
  gboolean screenshot = FALSE;
 
  if (gtk_dialog_run (GTK_DIALOG (chooser)) == GTK_RESPONSE_ACCEPT)
    {

      filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (chooser));
      workspace_dir = gtk_file_chooser_get_current_folder(GTK_FILE_CHOOSER(chooser));
      
      char* extension = strrchr(filename, '.');
      if ((extension==0) || (strcmp(extension, ".png") != 0))
        {
          filename = (gchar *) realloc(filename,  (strlen(filename) + 4 + 1) * sizeof(gchar)); 
          (void) strcat((gchar *)filename, ".png");
        }           
      free(extension);

      if (file_exists(filename,(char *) workspace_dir))
        {
	  GtkWidget *msg_dialog; 
          msg_dialog = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,  GTK_BUTTONS_YES_NO, "File Exists. Overwrite");
          gtk_window_stick((GtkWindow*)msg_dialog);
 
          int result = gtk_dialog_run(GTK_DIALOG(msg_dialog));
          if (msg_dialog!=NULL)
            { 
	      gtk_widget_destroy(msg_dialog);
            }
	  if ( result == GTK_RESPONSE_NO)
            { 
	      g_free(filename);
	      return; 
	    } 
	}
      screenshot = TRUE;
    }
  if (chooser!=NULL)
    {
      gtk_widget_destroy (chooser);
    }
  paint();
  if (screenshot)
    {
      make_screenshot(filename);
    }
  free(date);
  g_free(filename);

}


void
on_toolsRecorder_activate              (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  if(is_recording())
    {
      kill(ffmpegpid,9);
      ffmpegpid=-1;
      /* set stop tooltip */ 
      gtk_tool_item_set_tooltip_text((GtkToolItem *) toolbutton,"Record");
      /* put icon to record */
      gtk_tool_button_set_stock_id (toolbutton, "gtk-media-record");
    }
  else
    {      
      /* the recording is not active */ 
      start_save_video_dialog(toolbutton);
    }
  paint();

}


void
on_thickScale_value_changed		(GtkHScale   *hScale,
					 gpointer         user_data)
{

  tickness=gtk_range_get_value(&hScale->scale.range);
  thick();

}


void
on_toolsPreferences_activate	        (GtkToolButton   *toolbutton,
					 gpointer         user_data)
{

  GtkWidget *preferenceDialog;

  /* Initialize the main window */
  dialogGtkBuilder= gtk_builder_new();

  /* Load the gtk builder file created with glade */
  gtk_builder_add_from_file(dialogGtkBuilder,PACKAGE_DATA_DIR G_DIR_SEPARATOR_S PACKAGE G_DIR_SEPARATOR_S "preferenceDialog.ui",NULL);
 
  /* Fill the window by the gtk builder xml */
  preferenceDialog = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"preferences"));
  gtk_window_stick((GtkWindow*)preferenceDialog);
  
  GtkFileChooser* chooser = GTK_FILE_CHOOSER(gtk_builder_get_object(dialogGtkBuilder,"imageChooserButton"));
  gchar* default_dir= PACKAGE_DATA_DIR "/" PACKAGE "/backgrounds";

  gtk_file_chooser_set_current_folder(chooser, default_dir);
  
  GtkFileFilter *filter = gtk_file_filter_new ();
  gtk_file_filter_set_name (filter, "PNG and JPEG");
  gtk_file_filter_add_mime_type (filter, "image/jpeg");
  gtk_file_filter_add_mime_type (filter, "image/png");
  gtk_file_chooser_add_filter (chooser, filter);
 
  /* Adding alpha */
  GtkWidget* color_button = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"backgroundColorButton"));
  gtk_color_button_set_use_alpha      (GTK_COLOR_BUTTON(color_button), TRUE);
 
  /* Connect all signals by reflection */
  gtk_builder_connect_signals ( dialogGtkBuilder, NULL );

  /* I hide the annotation to not disturb the user */
  hide_unhide();
  
  GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"file"));
  GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"color"));
  if (background==1)
  {
     gtk_toggle_button_set_active(colorToolButton,TRUE);
  }
  else if (background==2)
  {
     gtk_toggle_button_set_active(imageToolButton,TRUE);
  }

  gtk_dialog_run(GTK_DIALOG(preferenceDialog));
  if (preferenceDialog!=NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
  paint(); 

}


void
on_preferenceOkButton_clicked(GtkButton *buton, gpointer user_date)
{
 GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"color"));
  if (gtk_toggle_button_get_active(colorToolButton))
    {
      /* background color */
      GtkColorButton* backgroundColorButton = GTK_COLOR_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"backgroundColorButton"));
      GdkColor* gdkcolor = g_malloc (sizeof (GdkColor)); 
      gtk_color_button_get_color(backgroundColorButton,gdkcolor);
      char* background_color = gdkcolor_to_rgb(gdkcolor);
      change_background_color(background_color);
      g_free(gdkcolor);
      g_free(background_color);
      background = 1;  
    }
  else 
    {
      GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"file"));
      if (gtk_toggle_button_get_active(imageToolButton))
	{
	  /* background png from file */
	  GtkFileChooserButton* imageChooserButton = GTK_FILE_CHOOSER_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"imageChooserButton"));
	  gchar* file = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(imageChooserButton)); 
	  if (file!=NULL)
            {
              change_background_image(file);
              g_free(file);
              background = 2;  
            }
          else
            {
              /* no background */
	      remove_background();
              background = 0;  
            }
	}
      else
	{
	  /* none */
	  remove_background();  
          background = 0;  
	} 
    }        
  GtkWidget *preferenceDialog = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"preferences"));
  if (preferenceDialog!=NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
  paint();

}


void
on_imageChooserButton_file_set (GtkButton *buton, gpointer user_date)
{

 GtkToggleButton* imageToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"file"));
 gtk_toggle_button_set_active(imageToolButton,TRUE);

}


void
on_backgroundColorButton_color_set (GtkButton *buton, gpointer user_date)
{

 GtkToggleButton* colorToolButton = GTK_TOGGLE_BUTTON(gtk_builder_get_object(dialogGtkBuilder,"color"));
 gtk_toggle_button_set_active(colorToolButton,TRUE);

}



void
on_preferenceCancelButton_clicked(GtkButton *buton, gpointer user_date)
{

  GtkWidget *preferenceDialog = GTK_WIDGET(gtk_builder_get_object(dialogGtkBuilder,"preferences"));
  if (preferenceDialog!=NULL)
    {
      gtk_widget_destroy(preferenceDialog);
    }
  paint();  

}


void
on_buttonClear_activate                (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
  
  callAnnotate("--clear", NULL, NULL, NULL);   

}


void
on_buttonPicker_activate	        (GtkToolButton   *toolbutton,
					 gpointer         user_data)
{

  GtkToggleToolButton *button = GTK_TOGGLE_TOOL_BUTTON(toolbutton);
  GtkColorSelection *colorsel;
  if (gtk_toggle_tool_button_get_active(button))
    {
      /* open color widget */
      GtkWidget* colorDialog = gtk_color_selection_dialog_new ("Changing color");

      gtk_window_stick((GtkWindow*)colorDialog);


      colorsel = GTK_COLOR_SELECTION ((GTK_COLOR_SELECTION_DIALOG (colorDialog))->colorsel);
    
      /* color initially selected */ 
      GdkColor* gdkcolor = g_malloc (sizeof (GdkColor));
      gchar    *ccolor;
      ccolor=malloc(strlen(color)+2);
      if (picked_color!=NULL)
        {
           strncpy(&ccolor[1],picked_color,strlen(color)+1);
        }
      else
        {
           strncpy(&ccolor[1],color,strlen(color)+1); 
        }
      ccolor[0]='#'; 
      gdk_color_parse (ccolor, gdkcolor);
      g_free(ccolor);

      gtk_color_selection_set_current_color(colorsel, gdkcolor);
      gtk_color_selection_set_previous_color(colorsel, gdkcolor);
      gtk_color_selection_set_has_palette(colorsel, TRUE);
      gtk_color_selection_set_has_opacity_control (colorsel, TRUE);


      if (annotateclientpid != -1)
	{
	  kill(annotateclientpid,9);
	}  
      /* I hide the annotation to not disturb the user */
      hide_unhide();
      gint result = gtk_dialog_run((GtkDialog *) colorDialog);

      /* Wait for user to select OK or Cancel */
      switch (result)
	{
	case GTK_RESPONSE_OK:
	  colorsel = GTK_COLOR_SELECTION ((GTK_COLOR_SELECTION_DIALOG (colorDialog))->colorsel);
          gtk_color_selection_set_has_palette(colorsel, TRUE);
          gtk_color_selection_get_current_color   (colorsel, gdkcolor);
          color = gdkcolor_to_rgb(gdkcolor);
          if (picked_color==NULL)
            {
	       picked_color = malloc(strlen(color));
            }
          strncpy(&picked_color[0],color,strlen(color));
          g_free(gdkcolor);
	  break;
      
	default:
	  /* If dialog did not return OK then it was canceled */
	  //gtk_toggle_tool_button_set_active(button, FALSE); 
	  break;
	}
      if (colorDialog!=NULL)
      {
        gtk_widget_destroy(colorDialog);
      }
      paint(); 
    }

}

/* Start color handlers */

void
on_colorBlack_activate                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color="000000";
  paint();

}


void
on_colorBlue_activate                  (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "3333CC";
  paint();

}


void
on_colorRed_activate                   (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "FF0000";
  paint();

}


void
on_colorGreen_activate                 (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "008000";
  paint();

}


void
on_colorLightBlue_activate             (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "00C0FF";
  paint();

}


void
on_colorLightGreen_activate            (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "00FF00";
  paint();

}


void
on_colorMagenta_activate               (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{
 
  color = "FF00FF";
  paint();

}


void
on_colorOrange_activate                (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "FF8000";
  paint();

}


void
on_colorYellow_activate                (GtkToolButton   *toolbutton,
                                        gpointer         user_data)
{

  color = "FFFF00";
  paint();

}


void
on_colorWhite_activate                (GtkToolButton   *toolbutton,
				       gpointer         user_data)
{

  color = "FFFFFF";
  paint();

}


