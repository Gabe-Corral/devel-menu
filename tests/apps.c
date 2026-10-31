#include "../src/apps.h"

#include <glib.h>

static void case_insensitive_search(void) {
    MenuApp app = { .search_name = g_utf8_casefold("Firefox", -1) };
    g_assert_true(menu_app_matches(&app, "FIRE"));
    g_assert_true(menu_app_matches(&app, "fox"));
    g_assert_true(menu_app_matches(&app, ""));
    g_assert_false(menu_app_matches(&app, "terminal"));
    g_free(app.search_name);
}

static void discover_desktop_entries(void) {
    GPtrArray *apps = menu_apps_load();
    for (guint i = 0; i < apps->len; ++i) {
        MenuApp *app = g_ptr_array_index(apps, i);
        g_assert_nonnull(app->info);
        g_assert_nonnull(app->name);
        if (i > 0) {
            MenuApp *previous = g_ptr_array_index(apps, i - 1);
            g_assert_cmpint(g_utf8_collate(previous->name, app->name), <=, 0);
        }
    }
    g_ptr_array_unref(apps);
}

int main(int argc, char **argv) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/apps/search", case_insensitive_search);
    g_test_add_func("/apps/discovery", discover_desktop_entries);
    return g_test_run();
}
