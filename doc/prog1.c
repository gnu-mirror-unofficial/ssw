#include <gtk/gtk.h>
#include <ssw-sheet.h>

int
main (int argc, char **argv)
{
  GtkWidget *window;
  GtkWidget *sheet;
  gtk_init (&argc, &argv);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

  sheet = ssw_sheet_new ();

  gtk_container_add (GTK_CONTAINER (window), sheet);
  
  gtk_window_maximize (GTK_WINDOW (window));

  gtk_widget_show_all (window);

  gtk_main ();

  return 0;
}
