#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include "desktopentry.h"

static void chomp(gchar* str) {
    gchar *p = str;
    for (int i = 0; p != NULL && *p != '\0'; i++, p = &str[i]) {
        if (*p == '\n') {
            *p = '\0';
            break;
        }
    }
}

static DesktopEntry* read_desktop_entry(const gchar* path) {
    enum IDENTIFY {
        ID_NAME,
        ID_EXEC,
        ID_ICON,
        IDS_NUM
    };
    const char *IDS[] = {
        "Name=",
        "Exec=",
        "Icon="
    };
    const char *IDENTIFY = "[Desktop Entry]\n";
    const int LINE_LENGTH = 1024;
    FILE *fp;
    char line[LINE_LENGTH];
    DesktopEntry *entry = NULL;
    
    fp = g_fopen(path, "r");
    if (fp == NULL) {
        g_fprintf(stderr, "Open Error: %s\n", path);
        return NULL;
    }

    //ヘッダチェック
    if (fgets(line, LINE_LENGTH, fp) != NULL) {
        while (*line == '#') fgets(line, LINE_LENGTH, fp);
        if (strcmp(IDENTIFY, line) == 0) {
            g_fprintf(stderr, "Identify OK: %s\n", path);
            entry = new_desktop_entry();
            
            while (fgets(line, LINE_LENGTH, fp) != NULL) {
                for (int i = 0; i < IDS_NUM; i++) {
                    gchar *result = strstr(line, IDS[i]);
                    if (result != NULL && line[strlen(IDS[i])-1] == '=') {
                        gchar *ptr;
                        switch (i) {
                        case ID_NAME:
                            ptr = entry->name;
                            break;
                        case ID_EXEC:
                            ptr = entry->exec;
                            break;
                        case ID_ICON:
                            ptr = entry->icon;
                            break;
                        }
                        g_strlcpy(ptr, &line[strlen(IDS[i])], DENTRY_CHAR_LENGTH);
                        chomp(ptr);
                    }
                }
            }            
        } else {
            g_fprintf(stderr, "Identify Error: %s\n", path);
        }
    } else {
        g_fprintf(stderr, "Read Error: %s\n", path);
    }

    fclose(fp);
    
    return entry;
}

GList* get_application_list() {
    const gchar *APPDIRS[] = {
        "/usr/share/applications",
        g_build_filename(getenv("HOME"), ".local/share/applications", NULL)
    };
    const int APPDIRS_NUM = 2;

    GList *list = NULL;
    GDir *dir;
    
    for (int i = 0; i < APPDIRS_NUM; i++) {
        dir = g_dir_open(APPDIRS[i], 0, NULL);
        if (dir) {
            const gchar *name;
            while (name = g_dir_read_name(dir)) {
                gchar *path;
                gchar *ext;
                path = g_build_filename(APPDIRS[i], name, NULL);
                ext = strstr(path, ".");
                if (!g_file_test(path, G_FILE_TEST_IS_DIR) &&
                    ext != NULL &&
                    g_strcmp0(ext, ".desktop") == 0) {
                    DesktopEntry *entry;
                    entry = read_desktop_entry(path);
                    if (entry != NULL) {
                        list = g_list_append(list, entry);
                    }
                }
                g_free(path);
            }
            g_dir_close(dir);
        }
    }
    return g_list_first(list);
}
