#ifndef H_DESKTOPENTRY
#define H_DESKTOPENTRY

typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
} DesktopEntry;

static inline DesktopEntry* new_desktop_entry() {
    const int CHAR_LENGTH = 1024;
    DesktopEntry *entry;
    entry = (DesktopEntry*)malloc(sizeof(DesktopEntry));
    entry->name = (gchar*)calloc(sizeof(gchar), CHAR_LENGTH);
    entry->exec = (gchar*)calloc(sizeof(gchar), CHAR_LENGTH);
    entry->icon = (gchar*)calloc(sizeof(gchar), CHAR_LENGTH);
    return entry;
}

static inline void free_desktop_entry(DesktopEntry *entry) {
    free(entry->name);
    free(entry->exec);
    free(entry->icon);
    free(entry);
}

#endif
