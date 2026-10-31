#pragma once

#include <gio/gio.h>

typedef struct {
    GAppInfo *info;
    char *name;
    char *search_name;
} MenuApp;

/* Each item (and its GAppInfo reference) belongs to the returned array. */
GPtrArray *menu_apps_load(void);
gboolean menu_app_matches(const MenuApp *app, const char *query);
