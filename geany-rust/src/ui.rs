#[derive(Debug, PartialEq, Clone, Copy)]
pub enum TabOrientation {
    Horizontal,
    Vertical,
}

pub struct TabManager {
    orientation: TabOrientation,
    tabs: Vec<String>,
}

impl TabManager {
    pub fn new() -> Self {
        Self {
            orientation: TabOrientation::Horizontal,
            tabs: Vec::new(),
        }
    }

    pub fn set_orientation(&mut self, orientation: TabOrientation) {
        self.orientation = orientation;
        println!("[Rust-TabManager] Setting orientation to {:?}", orientation);
    }

    pub fn get_orientation(&self) -> TabOrientation {
        self.orientation
    }

    pub fn add_tab(&mut self, title: String) -> usize {
        self.tabs.push(title.clone());
        println!("[Rust-TabManager] Added tab: {}", title);
        self.tabs.len() - 1
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_tab_manager() {
        let mut mgr = TabManager::new();

        mgr.set_orientation(TabOrientation::Vertical);
        assert_eq!(mgr.get_orientation(), TabOrientation::Vertical);

        let idx = mgr.add_tab("My Tab".to_string());
        assert_eq!(idx, 0);
    }
}
