# devel-menu

A small standalone Rust/GTK 3 Wayland application launcher for devel-wm. It
lists installed, visible `.desktop` applications in a centered overlay. Type
part of an application name to filter, use Up/Down and Enter (or click a row)
to launch it, and Escape to close. It does not run arbitrary typed shell commands.

Build with `cargo build --locked` (requires GTK 3 and gtk-layer-shell development
packages). Run `target/debug/devel-menu` from a Wayland session supporting wlr-layer-shell.
The devel-wm `run-native-with-bar.sh` script builds the menu before entering
native mode and makes it available to Alt+p automatically (press it again to close). For other launch
methods, install the binary on `PATH` or set `DEVEL_MENU_BIN` to its absolute
path before starting devel-wm. `cargo test --locked` runs the non-graphical search and
desktop-entry tests.

The application list/search is separate from the UI so future iterations can
add stdin-driven menus, richer matching, and WM command providers without
moving launcher UI into the compositor.
