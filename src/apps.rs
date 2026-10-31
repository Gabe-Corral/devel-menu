use gio::prelude::*;

pub struct MenuApp {
    pub info: gio::AppInfo,
    pub name: String,
    search_name: String,
}

impl MenuApp {
    fn new(info: gio::AppInfo) -> Option<Self> {
        let name = info.display_name().to_string();
        if !info.should_show() || name.is_empty() {
            return None;
        }
        let search_name = glib::casefold(&name).to_string();
        Some(Self {
            info,
            name,
            search_name,
        })
    }

    pub fn matches(&self, query: &str) -> bool {
        self.search_name.contains(glib::casefold(query).as_str())
    }
}

pub fn load() -> Vec<MenuApp> {
    let mut apps: Vec<_> = gio::AppInfo::all()
        .into_iter()
        .filter_map(MenuApp::new)
        .collect();
    apps.sort_by(|a, b| {
        glib::GString::from(a.name.as_str())
            .as_gstr()
            .collate(b.name.as_str())
    });
    apps
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn case_insensitive_search() {
        let info = gio::AppInfo::create_from_commandline(
            "/usr/bin/true",
            Some("Firefox"),
            gio::AppInfoCreateFlags::NONE,
        )
        .unwrap();
        let app = MenuApp {
            info,
            name: "Firefox".into(),
            search_name: "firefox".into(),
        };
        assert!(app.matches("FIRE"));
        assert!(app.matches("fox"));
        assert!(app.matches(""));
        assert!(!app.matches("terminal"));
        let unicode = MenuApp {
            search_name: glib::casefold("Straße").to_string(),
            ..app
        };
        assert!(unicode.matches("STRASSE"));
    }

    #[test]
    fn discover_desktop_entries() {
        let apps = load();
        assert!(
            apps.iter()
                .all(|app| app.info.should_show() && !app.name.is_empty())
        );
        assert!(apps.windows(2).all(|pair| {
            glib::GString::from(pair[0].name.as_str())
                .as_gstr()
                .collate(pair[1].name.as_str())
                .is_le()
        }));
    }
}
