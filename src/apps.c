#include "apps.h"

#include <gio/gdesktopappinfo.h>

static void menu_app_free(gpointer data) {
    MenuApp *app = data;
    g_object_unref(app->info);
    g_free(app->name);
    g_free(app->search_name);
    g_free(app);
}

static gint compare_apps(gconstpointer left, gconstpointer right) {
    const MenuApp *a = *(MenuApp * const *)left;
    const MenuApp *b = *(MenuApp * const *)right;
    return g_utf8_collate(a->name, b->name);
}

GPtrArray *menu_apps_load(void) {
    GPtrArray *apps = g_ptr_array_new_with_free_func(menu_app_free);
    GList *all = g_app_info_get_all();
    for (GList *item = all; item != NULL; item = item->next) {
        GAppInfo *info = item->data;
        const char *name = g_app_info_get_display_name(info);
        if (!g_app_info_should_show(info) || name == NULL || *name == '\0') {
            g_object_unref(info);
            continue;
        }
        MenuApp *app = g_new0(MenuApp, 1);
        app->info = info;
        app->name = g_strdup(name);
        app->search_name = g_utf8_casefold(name, -1);
        g_ptr_array_add(apps, app);
    }
    g_list_free(all);
    g_ptr_array_sort(apps, compare_apps);
    return apps;
}

gboolean menu_app_matches(const MenuApp *app, const char *query) {
    if (*query == '\0') return TRUE;
    char *needle = g_utf8_casefold(query, -1);
    gboolean match = g_strstr_len(app->search_name, -1, needle) != NULL;
    g_free(needle);
    return match;
}
