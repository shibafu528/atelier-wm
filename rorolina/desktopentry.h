#ifndef H_DESKTOPENTRY
#define H_DESKTOPENTRY

#include <glib.h>
#include <stdlib.h>

#define DENTRY_CHAR_LENGTH 1024

typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
} DesktopEntry;

static inline DesktopEntry* new_desktop_entry() {
    DesktopEntry *entry;
    entry = (DesktopEntry*)malloc(sizeof(DesktopEntry));
    entry->name = (gchar*)calloc(sizeof(gchar), DENTRY_CHAR_LENGTH);
    entry->exec = (gchar*)calloc(sizeof(gchar), DENTRY_CHAR_LENGTH);
    entry->icon = (gchar*)calloc(sizeof(gchar), DENTRY_CHAR_LENGTH);
    return entry;
}

static inline void free_desktop_entry(DesktopEntry *entry) {
    free(entry->name);
    free(entry->exec);
    free(entry->icon);
    free(entry);
}

static inline void free_desktop_entries(GList* list) {
    list = g_list_first(list);
    while (list != NULL) {
        if (list->data != NULL) free_desktop_entry((DesktopEntry*)list->data);
        list = g_list_next(list);
    }
}

static inline DesktopEntry* get_desktop_entry(GList *list) {
    return (DesktopEntry*)list->data;
}

GList* get_application_list();

#endif
