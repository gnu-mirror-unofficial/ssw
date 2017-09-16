#include <gtk/gtk.h>
#include <ssw-sheet.h>

GtkTreeModel *
create_data_model (void)
{
  GtkListStore *model = gtk_list_store_new (3,
					    G_TYPE_INT,
					    G_TYPE_STRING,
					    G_TYPE_DOUBLE);
  for (int i = 0; i < 4; ++i)
    {
      GtkTreeIter iter;
      gtk_list_store_append (model, &iter);
      gtk_list_store_set (model, &iter,
			  0, i,
			  1, (i % 2) ? "odd" : "even",
			  2, i / 3.0,
			  -1);
    }

  return GTK_TREE_MODEL (model);
}

int
main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  GtkWidget *sheet = ssw_sheet_new ();
  
  GtkTreeModel *data = create_data_model ();
  g_object_set (sheet, "data-model", data, NULL);
  g_object_set (sheet, "editable", TRUE, NULL);

  gtk_container_add (GTK_CONTAINER (window), sheet);
  gtk_window_maximize (GTK_WINDOW (window));
  gtk_widget_show_all (window);
  gtk_main ();

  return 0;
}
