#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>
#include <unistd.h>

static void cb_entry(GtkEntry *entry, gpointer data);
static void cb_execute(GtkButton *button, gpointer data);
static gboolean cb_esc(GtkWidget *widget, GdkEventKey *event, gpointer data) {
    if (event->keyval == GDK_KEY_Escape) {
        gtk_main_quit();
    }
    return FALSE;
}

int main(int argc, char* argv[]) {
    GtkWidget *window;

    gtk_init(&argc, &argv);

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Application Launcher");
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE);
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_DIALOG);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    gtk_widget_set_size_request(window, 300, 70);
    {
        GtkWidget *vbox;
        vbox = gtk_vbox_new(TRUE, 2);
        {
            GtkWidget *entry;
            GtkWidget *hbox;

            entry = gtk_entry_new();
            g_signal_connect(entry, "activate", G_CALLBACK(cb_entry), NULL);
            g_signal_connect_after(entry, "key-press-event", G_CALLBACK(cb_esc), NULL);
            gtk_container_add(GTK_CONTAINER(vbox), entry);
            
            hbox = gtk_hbox_new(TRUE, 2);
            {
                GtkWidget *button;
                button = gtk_button_new_from_stock(GTK_STOCK_EXECUTE);
                g_signal_connect(button, "clicked", G_CALLBACK(cb_execute), (gpointer)entry);
                gtk_container_add(GTK_CONTAINER(hbox), button);

                button = gtk_button_new_from_stock(GTK_STOCK_CLOSE);
                g_signal_connect(button, "clicked", G_CALLBACK(gtk_main_quit), NULL);
                gtk_container_add(GTK_CONTAINER(hbox), button);
            }
            gtk_container_add(GTK_CONTAINER(vbox), hbox); 
        }
        gtk_container_add(GTK_CONTAINER(window), vbox);
    }
    gtk_widget_show_all(window);

    gtk_main();

    return 0;
}

gboolean exist_command(const gchar *command) {
    gboolean state = FALSE;
    // whichコマンドの返り値を使ってコマンドの存在判定を行う
    GString *which = g_string_new("which ");
    g_string_append(which, command);
    g_string_append(which, " > /dev/null");
    state = system(which->str) == 0 ? TRUE : FALSE;
    g_string_free(which, TRUE);
    return state;
}

void execute_command(const gchar *command) {
    gboolean result = FALSE;
    gchar *cmd;
    gchar **argv;

    g_print("%s\n", command);
    if (strcmp(command, "") != 0) {
        argv = g_strsplit(command, " ", 0);
        if (argv[0] != NULL) {
            cmd = g_strdup(argv[0]);
        }

        if (cmd != NULL && exist_command(cmd)) {
            int pid = fork();
            switch (pid) {
            case 0:
                execvp(cmd, argv);
                break;
            case -1:
                g_printf("Execute Failed - fork error\n");
                break;
            default:
                result = TRUE;
                break;
            }
        }

        g_free(cmd);
        g_strfreev(argv);
    }

    if (result) {
        gtk_main_quit();
    }    
}

static void cb_entry(GtkEntry *entry, gpointer data) {
    execute_command(gtk_entry_get_text(entry));
}

static void cb_execute(GtkButton *button, gpointer data) {
    execute_command(gtk_entry_get_text(GTK_ENTRY(data)));
}
