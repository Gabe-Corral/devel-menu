mod apps;

use std::{cell::RefCell, rc::Rc};

use gdk::prelude::*;
use gtk::prelude::*;
use gtk_layer_shell::{Edge, KeyboardMode, Layer, LayerShell};

const MAX_RESULTS: usize = 8;

struct Menu {
    entry: gtk::SearchEntry,
    results: gtk::ListBox,
    message: gtk::Label,
    apps: Vec<apps::MenuApp>,
    shown: RefCell<Vec<usize>>,
}

impl Menu {
    fn launch_selected(&self) {
        let Some(row) = self.results.selected_row() else {
            return;
        };
        let Some(index) = self.shown.borrow().get(row.index() as usize).copied() else {
            return;
        };
        let Some(display) = gdk::Display::default() else {
            return;
        };
        let Some(context) = display.app_launch_context() else {
            return;
        };
        match self.apps[index].info.launch(&[], Some(&context)) {
            Ok(()) => gtk::main_quit(),
            Err(error) => {
                self.message.set_text(&error.to_string());
                self.message.show();
            }
        }
    }

    fn refresh(&self) {
        for child in self.results.children() {
            self.results.remove(&child);
        }
        let query = self.entry.text();
        let mut shown = self.shown.borrow_mut();
        shown.clear();
        for (index, app) in self.apps.iter().enumerate() {
            if !app.matches(&query) {
                continue;
            }
            let row = gtk::ListBoxRow::new();
            let label = gtk::Label::new(Some(&app.name));
            label.set_halign(gtk::Align::Start);
            label.set_margin_start(12);
            label.set_margin_end(12);
            label.set_margin_top(9);
            label.set_margin_bottom(9);
            row.add(&label);
            self.results.add(&row);
            row.show_all();
            shown.push(index);
            if shown.len() == MAX_RESULTS {
                break;
            }
        }
        self.message.set_visible(shown.is_empty());
        if shown.is_empty() {
            self.message.set_text("No applications found");
        } else if let Some(row) = self.results.row_at_index(0) {
            self.results.select_row(Some(&row));
        }
    }
}

fn run() -> Result<(), Box<dyn std::error::Error>> {
    // Register before GTK startup so a second Alt+p cannot miss the toggle.
    glib::source::unix_signal_add(libc::SIGUSR1, || {
        gtk::main_quit();
        glib::ControlFlow::Break
    });
    gtk::init()?;
    if !gtk_layer_shell::is_supported() {
        return Err("devel-menu needs a Wayland compositor with wlr-layer-shell support".into());
    }

    let window = gtk::Window::new(gtk::WindowType::Toplevel);
    window.set_title("devel-menu");
    window.set_decorated(false);
    window.set_resizable(false);
    window.set_size_request(560, -1);
    window.init_layer_shell();
    window.set_namespace("devel-menu");
    window.set_layer(Layer::Overlay);
    window.set_keyboard_mode(KeyboardMode::Exclusive);
    for edge in [Edge::Left, Edge::Right, Edge::Top, Edge::Bottom] {
        window.set_anchor(edge, false);
    }
    window.set_exclusive_zone(0);

    let container = gtk::Box::new(gtk::Orientation::Vertical, 5);
    container.set_border_width(12);
    window.add(&container);
    let entry = gtk::SearchEntry::new();
    entry.set_placeholder_text(Some("Search applications"));
    container.pack_start(&entry, false, false, 0);
    let results = gtk::ListBox::new();
    results.set_activate_on_single_click(true);
    container.pack_start(&results, false, false, 0);
    let message = gtk::Label::new(Some("No applications found"));
    container.pack_start(&message, false, false, 0);

    let style = gtk::CssProvider::new();
    style.load_from_data(
        b"window { background: #282a36; color: #f8f8f2; border: 2px solid #bd93f9; }
        entry { background: #44475a; color: #f8f8f2; min-height: 30px; }
        list row:selected { background: #6272a4; }
        list row:hover { background: #44475a; }",
    )?;
    let screen = gdk::Screen::default().ok_or("no display screen")?;
    gtk::StyleContext::add_provider_for_screen(
        &screen,
        &style,
        gtk::STYLE_PROVIDER_PRIORITY_APPLICATION,
    );

    let menu = Rc::new(Menu {
        entry,
        results,
        message,
        apps: apps::load(),
        shown: RefCell::new(Vec::new()),
    });
    window.connect_destroy(|_| gtk::main_quit());
    {
        let menu = Rc::clone(&menu);
        let entry = menu.entry.clone();
        entry.connect_changed(move |_| menu.refresh());
    }
    {
        let menu = Rc::clone(&menu);
        let entry = menu.entry.clone();
        entry.connect_key_press_event(move |_, event| {
            match event.keyval() {
                gdk::keys::constants::Escape => gtk::main_quit(),
                gdk::keys::constants::Return | gdk::keys::constants::KP_Enter => {
                    menu.launch_selected()
                }
                gdk::keys::constants::Up | gdk::keys::constants::Down => {
                    let current = menu.results.selected_row().map_or(0, |row| row.index());
                    let delta = if event.keyval() == gdk::keys::constants::Down {
                        1
                    } else {
                        -1
                    };
                    if let Some(row) = menu.results.row_at_index(current + delta) {
                        menu.results.select_row(Some(&row));
                    }
                }
                _ => return glib::Propagation::Proceed,
            }
            glib::Propagation::Stop
        });
    }
    {
        let menu = Rc::clone(&menu);
        let results = menu.results.clone();
        results.connect_row_activated(move |list, row| {
            list.select_row(Some(row));
            menu.launch_selected();
        });
    }
    menu.refresh();
    window.show_all();
    if !menu.shown.borrow().is_empty() {
        menu.message.hide();
    }
    menu.entry.grab_focus();
    gtk::main();
    Ok(())
}

fn main() {
    if let Err(error) = run() {
        eprintln!("devel-menu: {error}");
        std::process::exit(1);
    }
}
