# devel-menu

A standalone Rust/GTK 3 Wayland application launcher for devel-wm. It searches installed `.desktop` apps; type to filter, press Enter to launch, or Escape to close. It does not execute arbitrary typed shell commands.

## Planned

Planned directions include richer application matching, stdin-driven menus, and devel-wm command providers, while keeping the launcher separate from the compositor. These are not implemented yet; typed input currently filters installed applications.

## Run

Install GTK 3 and gtk-layer-shell development packages, then build with `cargo build --locked` and run `target/debug/devel-menu` in a Wayland session supporting wlr-layer-shell. devel-wm's `scripts/run-native-with-bar.sh` builds and exposes it through Alt+p automatically. For other startup methods, put `devel-menu` on `PATH` or set `DEVEL_MENU_BIN` to its absolute path before starting devel-wm.
