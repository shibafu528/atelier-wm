#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>
#include <stdlib.h>
#include "desktopentry.h"

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
    DesktopEntry *entry;
    
    fp = g_fopen(path, "r");
    if (fp == NULL) {
        g_fprintf(stderr, "Open Error: %s\n", path);
        return NULL;
    }

    //ヘッダチェック
    if (fgets(line, LINE_LENGTH, fp) != NULL) {
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
                        sscanf(&line[strlen(IDS[i])], "%s\n", ptr);
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
    return NULL;
}

int main(int argc, char* argv[]) {
    DesktopEntry *ent;
    ent = read_desktop_entry("/usr/share/applications/chromium.desktop");
    if (ent != NULL) {
        printf("Name:%s\nExec Command:%s\nIcon:%s\n", ent->name, ent->exec, ent->icon);
        free_desktop_entry(ent);
    }
    return 0;
}
