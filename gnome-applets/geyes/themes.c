/*
 * Copyright (C) 1999 Dave Camp <dave@davec.dhs.org>
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
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 *  
 */

#include "config.h"

#include <string.h>
#include <limits.h>
#include <ctype.h>
#include <glib/gi18n-lib.h>
#include <gtk/gtk.h>

#include "geyes-applet-private.h"

#define NUM_THEME_DIRECTORIES 2
#define HIG_IDENTATION  "    "

static char *theme_directories[NUM_THEME_DIRECTORIES];

enum {
	COL_THEME_DIR = 0,
	COL_THEME_NAME,
	TOTAL_COLS
};

void
theme_dirs_create (void)
{
	static gboolean themes_created = FALSE;
	
	if (themes_created == TRUE)
		return;

	theme_directories[0] = g_strdup (GEYES_THEMES_DIR);
	theme_directories[1] = g_strdup_printf ("%s/.gnome2/geyes-themes/", g_get_home_dir());

	themes_created = TRUE;
}

static gboolean
parse_theme_file (EyesApplet *eyes_applet, FILE *theme_file)
{
  gchar line_buf [512]; /* prolly overkill */
  gchar *token;

  if (fgets (line_buf, 512, theme_file) == NULL)
    return FALSE;

  while (!feof (theme_file)) {
    token = strtok (line_buf, "=");

    if (strncmp (token, "wall-thickness", strlen ("wall-thickness")) == 0) {
      token += strlen ("wall-thickness");

      while (!isdigit (*token))
        token++;

      sscanf (token, "%d", &eyes_applet->wall_thickness);
    } else if (strncmp (token, "num-eyes", strlen ("num-eyes")) == 0) {
      token += strlen ("num-eyes");

      while (!isdigit (*token))
        token++;

      sscanf (token, "%d", &eyes_applet->num_eyes);

      if (eyes_applet->num_eyes > MAX_EYES)
        eyes_applet->num_eyes = MAX_EYES;
    } else if (strncmp (token, "eye-pixmap", strlen ("eye-pixmap")) == 0) {
      token = strtok (NULL, "\"");
      token = strtok (NULL, "\"");

      if (eyes_applet->eye_filename != NULL)
        g_free (eyes_applet->eye_filename);

      eyes_applet->eye_filename = g_strdup_printf ("%s%s",
                                                   eyes_applet->theme_dir,
                                                   token);
    } else if (strncmp (token, "pupil-pixmap", strlen ("pupil-pixmap")) == 0) {
      token = strtok (NULL, "\"");
      token = strtok (NULL, "\"");

      if (eyes_applet->pupil_filename != NULL)
        g_free (eyes_applet->pupil_filename);

      eyes_applet->pupil_filename = g_strdup_printf ("%s%s",
                                                     eyes_applet->theme_dir,
                                                     token);
    }

    if (fgets (line_buf, 512, theme_file) == NULL && !feof (theme_file))
      return FALSE;
  }

  return TRUE;
}

int
load_theme (EyesApplet *eyes_applet, const gchar *theme_dir)
{
	GtkWidget *dialog;

	FILE* theme_file;
        gchar *file_name;

        eyes_applet->theme_dir = g_strdup_printf ("%s/", theme_dir);

        file_name = g_strdup_printf("%s%s",theme_dir,"/config");
        theme_file = fopen (file_name, "r");
        g_free (file_name);

        if (theme_file == NULL) {
        	g_free (eyes_applet->theme_dir);
        	eyes_applet->theme_dir = g_strdup_printf (GEYES_THEMES_DIR "Default-tiny/");

                file_name = g_strdup (GEYES_THEMES_DIR "Default-tiny/config");
                theme_file = fopen (file_name, "r");
                g_free (file_name);
        }

	/* if it's still NULL we've got a major problem */
	if (theme_file == NULL || parse_theme_file (eyes_applet, theme_file) == FALSE) {
		if (theme_file)
			fclose (theme_file);
		dialog = gtk_message_dialog_new_with_markup (NULL,
				GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_ERROR,
				GTK_BUTTONS_OK,
				"<b>%s</b>\n\n%s",
				_("Can not launch the eyes applet."),
				_("There was a fatal error while trying to load the theme."));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		gtk_widget_destroy (GTK_WIDGET (eyes_applet));

		return FALSE;
	}

        eyes_applet->theme_name = g_strdup (theme_dir);
       
        if (eyes_applet->eye_image)
        	g_object_unref (eyes_applet->eye_image);
        eyes_applet->eye_image = gdk_pixbuf_new_from_file (eyes_applet->eye_filename, NULL);
        if (eyes_applet->pupil_image)
        	g_object_unref (eyes_applet->pupil_image);
        eyes_applet->pupil_image = gdk_pixbuf_new_from_file (eyes_applet->pupil_filename, NULL);

	eyes_applet->eye_height = gdk_pixbuf_get_height (eyes_applet->eye_image);
        eyes_applet->eye_width = gdk_pixbuf_get_width (eyes_applet->eye_image);
        eyes_applet->pupil_height = gdk_pixbuf_get_height (eyes_applet->pupil_image);
        eyes_applet->pupil_width = gdk_pixbuf_get_width (eyes_applet->pupil_image);

        fclose (theme_file);

	return TRUE;
}

static void
destroy_theme (EyesApplet *eyes_applet)
{
	/* Dunno about this - to unref or not to unref? */
	if (eyes_applet->eye_image != NULL) {
        	g_object_unref (eyes_applet->eye_image); 
        	eyes_applet->eye_image = NULL;
        }
        if (eyes_applet->pupil_image != NULL) {
        	g_object_unref (eyes_applet->pupil_image); 
        	eyes_applet->pupil_image = NULL;
	}
	
        g_free (eyes_applet->theme_dir);
        g_free (eyes_applet->theme_name);
}

static void
theme_selected_cb (GtkTreeSelection *selection, gpointer data)
{
	EyesApplet *eyes_applet = data;
	GtkTreeModel *model;
	GtkTreeIter iter;
	gchar *theme;
	gchar *theme_dir;
	
	if (!gtk_tree_selection_get_selected (selection, &model, &iter))
		return;
	
	gtk_tree_model_get (model, &iter, COL_THEME_DIR, &theme, -1);
	
	g_return_if_fail (theme);
	
	theme_dir = g_strdup_printf ("%s/", theme);
	if (!g_ascii_strncasecmp (theme_dir, eyes_applet->theme_dir, strlen (theme_dir))) {
		g_free (theme_dir);
		return;
	}
	g_free (theme_dir);
		
	destroy_eyes (eyes_applet);
        destroy_theme (eyes_applet);
        load_theme (eyes_applet, theme);
        setup_eyes (eyes_applet);
	
	g_settings_set_string (eyes_applet->settings, KEY_THEME_PATH, theme);
	
	g_free (theme);
}

static void
presponse_cb (GtkDialog *dialog, gint id, gpointer data)
{
	EyesApplet *eyes_applet = data;
	if(id == GTK_RESPONSE_HELP){
		gp_applet_show_help (GP_APPLET (eyes_applet), "geyes-settings");
		return;
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));

	eyes_applet->prop_box.pbox = NULL;
}

void
properties_cb (GSimpleAction *action,
               GVariant      *parameter,
               gpointer       user_data)
{
	EyesApplet *eyes_applet = (EyesApplet *) user_data;
	GtkWidget *pbox, *hbox;
	GtkWidget *vbox, *indent;
	GtkWidget *categories_vbox;
	GtkWidget *category_vbox, *control_vbox;
        GtkWidget *tree;
	GtkWidget *scrolled;
        GtkWidget *label;
        GtkListStore *model;
        GtkTreeViewColumn *column;
        GtkCellRenderer *cell;
        GtkTreeSelection *selection;
        GtkTreeIter iter;
        int i;
        gchar *title;

	if (eyes_applet->prop_box.pbox) {
		gtk_window_set_screen (
			GTK_WINDOW (eyes_applet->prop_box.pbox),
			gtk_widget_get_screen (GTK_WIDGET (eyes_applet)));
		gtk_window_present (GTK_WINDOW (eyes_applet->prop_box.pbox));
		return;
	}

        pbox = gtk_dialog_new_with_buttons (_("Geyes Preferences"), NULL,
        				     GTK_DIALOG_DESTROY_WITH_PARENT,
					     _("_Close"), GTK_RESPONSE_CLOSE,
					     _("_Help"), GTK_RESPONSE_HELP,
					     NULL);

	gtk_window_set_screen (GTK_WINDOW (pbox),
			       gtk_widget_get_screen (GTK_WIDGET (eyes_applet)));
        
	gtk_widget_set_size_request (GTK_WIDGET (pbox), 300, 200);
        gtk_dialog_set_default_response(GTK_DIALOG (pbox), GTK_RESPONSE_CLOSE);
        gtk_container_set_border_width (GTK_CONTAINER (pbox), 5);
	gtk_box_set_spacing (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pbox))), 2);

        g_signal_connect (pbox, "response",
			  G_CALLBACK (presponse_cb),
			  eyes_applet);
	
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_widget_show (vbox);
	
	gtk_box_pack_start (GTK_BOX (gtk_dialog_get_content_area (GTK_DIALOG (pbox))), vbox,
			    TRUE, TRUE, 0);

	categories_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 18);
	gtk_box_pack_start (GTK_BOX (vbox), categories_vbox, TRUE, TRUE, 0);
	gtk_widget_show (categories_vbox);

	category_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_pack_start (GTK_BOX (categories_vbox), category_vbox, TRUE, TRUE, 0);
	gtk_widget_show (category_vbox);
	
	title = g_strconcat ("<span weight=\"bold\">", _("Themes"), "</span>", NULL);
	label = gtk_label_new (_(title));
	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_box_pack_start (GTK_BOX (category_vbox), label, FALSE, FALSE, 0);
	g_free (title);
	
	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_box_pack_start (GTK_BOX (category_vbox), hbox, TRUE, TRUE, 0);
	gtk_widget_show (hbox);
	
	indent = gtk_label_new (HIG_IDENTATION);
	gtk_label_set_justify (GTK_LABEL (indent), GTK_JUSTIFY_LEFT);
	gtk_box_pack_start (GTK_BOX (hbox), indent, FALSE, FALSE, 0);
	gtk_widget_show (indent);
	
	control_vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 6);
	gtk_box_pack_start (GTK_BOX (hbox), control_vbox, TRUE, TRUE, 0);
	gtk_widget_show (control_vbox);
	
	label = gtk_label_new_with_mnemonic (_("_Select a theme:"));
	gtk_label_set_xalign (GTK_LABEL (label), 0.0);
	gtk_box_pack_start (GTK_BOX (control_vbox), label, FALSE, FALSE, 0);

	scrolled = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);

	model = gtk_list_store_new (TOTAL_COLS, G_TYPE_STRING, G_TYPE_STRING);
	tree = gtk_tree_view_new_with_model (GTK_TREE_MODEL (model));
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (tree), FALSE);
	gtk_label_set_mnemonic_widget (GTK_LABEL (label), tree);

	gtk_container_add (GTK_CONTAINER (scrolled), tree);
	
	cell = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("not used", cell,
                                                           "text", COL_THEME_NAME, NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (tree), column);
                                                           
        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (tree));
	g_signal_connect (selection, "changed",
			  G_CALLBACK (theme_selected_cb),
			  eyes_applet);

	if (!g_settings_is_writable (eyes_applet->settings, KEY_THEME_PATH)) {
		gtk_widget_set_sensitive (tree, FALSE);
		gtk_widget_set_sensitive (label, FALSE);
	}

        for (i = 0; i < NUM_THEME_DIRECTORIES; i++) {
                GDir *dir;
                const char *name;

                dir = g_dir_open (theme_directories[i], 0, NULL);
                if (dir == NULL)
                        continue;

                while ((name = g_dir_read_name (dir)) != NULL) {
                        char *filename;
                        char *theme_dir;
                        char *theme_name;

                        filename = g_build_filename (theme_directories[i], name, NULL);
                        theme_dir = g_strdup_printf ("%s/", filename);
                        theme_name = g_path_get_basename (filename);

                        gtk_list_store_append (model, &iter);
                        gtk_list_store_set (model, &iter,
                                            COL_THEME_DIR, filename,
                                            COL_THEME_NAME, theme_name,
                                            -1);

                        if (!g_ascii_strncasecmp (eyes_applet->theme_dir,
                                                  theme_dir,
                                                  strlen (theme_dir))) {
                                GtkTreePath *path;

                                path = gtk_tree_model_get_path (GTK_TREE_MODEL (model),
                                                                &iter);

                                gtk_tree_view_set_cursor (GTK_TREE_VIEW (tree),
                                                          path,
                                                          NULL,
                                                          FALSE);

                                gtk_tree_path_free (path);
                        }

                        g_free (theme_dir);
                        g_free (theme_name);
                        g_free (filename);
                }

                g_dir_close (dir);
        }

        g_object_unref (model);

        gtk_box_pack_start (GTK_BOX (control_vbox), scrolled, TRUE, TRUE, 0);
        
        gtk_widget_show_all (pbox);
        
        eyes_applet->prop_box.pbox = pbox;
}
