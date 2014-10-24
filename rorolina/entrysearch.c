#include <glib.h>
#include <glib/gstdio.h>
#include <string.h>

typedef struct {
    gchar *name;
    gchar *exec;
    gchar *icon;
} DesktopEntry;

static DesktopEntry* read_desktop_entry(const gchar* path) {
    const char *IDENTIFY = "[Desktop Entry]";
    const int LINE_LENGTH = 1024;
    FILE *fp;
    char line[LINE_LENGTH];
    
    fp = g_fopen(path, "r");
    if (fp == NULL) {
        g_fprintf(stderr, "Open Error: %s\n", path);
        return NULL;
    }

    //ヘッダチェック
    if (fgets(line, LINE_LENGTH, fp) != NULL) {
        if (strcmp(path, line) == 0) {
            //TODO: ここから先でname, exec, iconを読み込んでいく
            g_fprintf(stderr, "Identify OK: %s\n", path);
            
            while (fgets(line, LINE_LENGTH, fp) != NULL) {
                g_fprintf(stderr, "%s\n", line);
            }
        } else {
            g_fprintf(stderr, "Identify Error: %s\n", path);
        }
    } else {
        g_fprintf(stderr, "Read Error: %s\n", path);
    }

    fclose(fp);
}

GList* get_application_list() {
    return NULL;
}

int main(int argc, char* argv[]) {
    read_desktop_entry("/usr/share/applications/chromium.desktop");
    return 0;
}
