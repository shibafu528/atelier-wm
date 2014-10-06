#include <gtk/gtk.h>

static typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
} DesktopEntry;

GList* get_application_list() {
    return NULL;
}
