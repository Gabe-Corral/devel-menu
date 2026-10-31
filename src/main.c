#include "apps.h"

#include <gdk/gdkkeysyms.h>
#include <glib-unix.h>
#include <gtk/gtk.h>
#include <gtk-layer-shell.h>
#include <signal.h>

#define MAX_RESULTS 8

typedef struct {
    GtkWidget *window;
    GtkWidget *entry;
    GtkWidget *results;
    GtkWidget *message;
    GPtrArray *apps;
} Menu;

static gboolean toggle_close(gpointer data) {
    (void)data;
    gtk_main_quit();
    return G_SOURCE_REMOVE;
}

static void launch_selected(Menu *menu) {
    GtkListBoxRow *row = gtk_list_box_get_selected_row(GTK_LIST_BOX(menu->results));
    if (row == NULL) return;
    MenuApp *app = g_object_get_data(G_OBJECT(row), "menu-app");
    GdkAppLaunchContext *context = gdk_display_get_app_launch_context(gdk_display_get_default());
    GError *error = NULL;
    gboolean launched = g_app_info_launch(app->info, NULL, G_APP_LAUNCH_CONTEXT(context), &error);
    g_object_unref(context);
    if (launched) {
        gtk_main_quit();
    } else {
        gtk_label_set_text(GTK_LABEL(menu->message), error->message);
        gtk_widget_show(menu->message);
        g_clear_error(&error);
    }
}

static void row_activated(GtkListBox *list, GtkListBoxRow *row, gpointer data) {
    (void)list;
    Menu *menu = data;
    gtk_list_box_select_row(GTK_LIST_BOX(menu->results), row);
    launch_selected(menu);
}

static void refresh_results(Menu *menu) {
    GList *children = gtk_container_get_children(GTK_CONTAINER(menu->results));
    for (GList *item = children; item != NULL; item = item->next)
        gtk_widget_destroy(GTK_WIDGET(item->data));
    g_list_free(children);

    const char *query = gtk_entry_get_text(GTK_ENTRY(menu->entry));
    unsigned count = 0;
    for (guint i = 0; i < menu->apps->len && count < MAX_RESULTS; ++i) {
        MenuApp *app = g_ptr_array_index(menu->apps, i);
        if (!menu_app_matches(app, query)) continue;
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(app->name);
        gtk_widget_set_halign(label, GTK_ALIGN_START);
        gtk_widget_set_margin_start(label, 12);
        gtk_widget_set_margin_end(label, 12);
        gtk_widget_set_margin_top(label, 9);
        gtk_widget_set_margin_bottom(label, 9);
        gtk_container_add(GTK_CONTAINER(row), label);
        g_object_set_data(G_OBJECT(row), "menu-app", app);
        gtk_container_add(GTK_CONTAINER(menu->results), row);
        gtk_widget_show_all(row);
        ++count;
    }
    gtk_widget_set_visible(menu->message, count == 0);
    if (count == 0) gtk_label_set_text(GTK_LABEL(menu->message), "No applications found");
    if (count > 0)
        gtk_list_box_select_row(GTK_LIST_BOX(menu->results),
                                gtk_list_box_get_row_at_index(GTK_LIST_BOX(menu->results), 0));
}

static void search_changed(GtkEditable *entry, gpointer data) {
    (void)entry;
    refresh_results(data);
}

static gboolean search_key(GtkWidget *entry, GdkEventKey *event, gpointer data) {
    (void)entry;
    Menu *menu = data;
    GtkListBox *list = GTK_LIST_BOX(menu->results);
    GtkListBoxRow *selected = gtk_list_box_get_selected_row(list);
    switch (event->keyval) {
    case GDK_KEY_Escape:
        gtk_main_quit();
        return TRUE;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:
        launch_selected(menu);
        return TRUE;
    case GDK_KEY_Down:
    case GDK_KEY_Up: {
        int index = selected != NULL ? gtk_list_box_row_get_index(selected) : 0;
        int next = index + (event->keyval == GDK_KEY_Down ? 1 : -1);
        GtkListBoxRow *row = gtk_list_box_get_row_at_index(list, next);
        if (row != NULL) gtk_list_box_select_row(list, row);
        return TRUE;
    }
    default:
        return FALSE;
    }
}

int main(int argc, char **argv) {
    /* Register before GTK startup so a second Alt+p cannot miss the toggle. */
    g_unix_signal_add(SIGUSR1, toggle_close, NULL);
    gtk_init(&argc, &argv);
    if (!gtk_layer_is_supported()) {
        g_printerr("devel-menu needs a Wayland compositor with wlr-layer-shell support\n");
        return 1;
    }

    Menu menu = {0};
    menu.apps = menu_apps_load();
    menu.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(menu.window), "devel-menu");
    gtk_window_set_decorated(GTK_WINDOW(menu.window), FALSE);
    gtk_window_set_resizable(GTK_WINDOW(menu.window), FALSE);
    gtk_widget_set_size_request(menu.window, 560, -1);
    gtk_layer_init_for_window(GTK_WINDOW(menu.window));
    gtk_layer_set_namespace(GTK_WINDOW(menu.window), "devel-menu");
    gtk_layer_set_layer(GTK_WINDOW(menu.window), GTK_LAYER_SHELL_LAYER_OVERLAY);
    gtk_layer_set_keyboard_mode(GTK_WINDOW(menu.window), GTK_LAYER_SHELL_KEYBOARD_MODE_EXCLUSIVE);
    /* No anchors: layer-shell centers the requested-size surface on its output. */
    for (GtkLayerShellEdge edge = GTK_LAYER_SHELL_EDGE_LEFT;
         edge <= GTK_LAYER_SHELL_EDGE_BOTTOM; ++edge)
        gtk_layer_set_anchor(GTK_WINDOW(menu.window), edge, FALSE);
    gtk_layer_set_exclusive_zone(GTK_WINDOW(menu.window), 0);

    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(box), 12);
    gtk_container_add(GTK_CONTAINER(menu.window), box);
    menu.entry = gtk_search_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(menu.entry), "Search applications");
    gtk_box_pack_start(GTK_BOX(box), menu.entry, FALSE, FALSE, 0);
    menu.results = gtk_list_box_new();
    gtk_list_box_set_activate_on_single_click(GTK_LIST_BOX(menu.results), TRUE);
    gtk_box_pack_start(GTK_BOX(box), menu.results, FALSE, FALSE, 0);
    menu.message = gtk_label_new("No applications found");
    gtk_box_pack_start(GTK_BOX(box), menu.message, FALSE, FALSE, 0);

    GtkCssProvider *style = gtk_css_provider_new();
    gtk_css_provider_load_from_data(style,
        "window { background: #282a36; color: #f8f8f2; border: 2px solid #bd93f9; }"
        "entry { background: #44475a; color: #f8f8f2; min-height: 30px; }"
        "list row:selected { background: #6272a4; }"
        "list row:hover { background: #44475a; }", -1, NULL);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),
        GTK_STYLE_PROVIDER(style), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(style);

    g_signal_connect(menu.window, "destroy", G_CALLBACK(gtk_main_quit), NULL);
    g_signal_connect(menu.entry, "changed", G_CALLBACK(search_changed), &menu);
    g_signal_connect(menu.entry, "key-press-event", G_CALLBACK(search_key), &menu);
    g_signal_connect(menu.results, "row-activated", G_CALLBACK(row_activated), &menu);
    refresh_results(&menu);
    gtk_widget_show_all(menu.window);
    if (menu.apps->len > 0) gtk_widget_hide(menu.message);
    gtk_widget_grab_focus(menu.entry);
    gtk_main();
    g_ptr_array_unref(menu.apps);
    return 0;
}
