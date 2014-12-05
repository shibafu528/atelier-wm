#include <glib.h>
#include <stdio.h>
#include "desktopentry.h"

int main(int argc, char* argv[]) {
    GList *entry;
    entry = get_application_list();
    if (entry != NULL) {
        while (entry != NULL) {
            DesktopEntry *de = (DesktopEntry*)entry->data;
            printf("Name:%s\nExec Command:%s\nIcon:%s\n\n", de->name, de->exec, de->icon);
            entry = g_list_next(entry);
        }
        free_desktop_entries(entry);
    }
    return 0;
}
