pub mod ui;
pub mod textfx;

use std::ffi::{CStr, CString};
use std::os::raw::c_char;
use std::sync::Mutex;
use ui::{TabManager, TabOrientation};

static TAB_MGR: Mutex<Option<TabManager>> = Mutex::new(None);

#[no_mangle]
pub extern "C" fn GeanyRust_Initialize() {
    println!("[Geany-Rust FFI] Initializing Rust backend...");
    let mut mgr = TAB_MGR.lock().unwrap();
    *mgr = Some(TabManager::new());
}

#[no_mangle]
pub extern "C" fn GeanyRust_Shutdown() {
    println!("[Geany-Rust FFI] Shutting down Rust backend...");
    let mut mgr = TAB_MGR.lock().unwrap();
    *mgr = None;
}

#[no_mangle]
pub extern "C" fn GeanyRust_UI_SetTabOrientation(vertical: bool) {
    if let Ok(mut lock) = TAB_MGR.lock() {
        if let Some(mgr) = lock.as_mut() {
            let orientation = if vertical {
                TabOrientation::Vertical
            } else {
                TabOrientation::Horizontal
            };
            mgr.set_orientation(orientation);
        }
    }
}

#[no_mangle]
pub extern "C" fn GeanyRust_FreeString(s: *mut c_char) {
    if s.is_null() {
        return;
    }
    unsafe {
        let _ = CString::from_raw(s);
    }
}

#[no_mangle]
pub extern "C" fn GeanyRust_TextFX_SortLines(
    c_text: *const c_char,
    ascending: bool,
    case_sensitive: bool,
) -> *mut c_char {
    if c_text.is_null() {
        return std::ptr::null_mut();
    }
    let text = unsafe { CStr::from_ptr(c_text).to_string_lossy() };
    let sorted = textfx::sort_lines(&text, ascending, case_sensitive);
    CString::new(sorted).unwrap().into_raw()
}

#[no_mangle]
pub extern "C" fn GeanyRust_TextFX_ToProperCase(c_text: *const c_char) -> *mut c_char {
    if c_text.is_null() {
        return std::ptr::null_mut();
    }
    let text = unsafe { CStr::from_ptr(c_text).to_string_lossy() };
    let converted = textfx::to_proper_case(&text);
    CString::new(converted).unwrap().into_raw()
}

#[no_mangle]
pub extern "C" fn GeanyRust_TextFX_ToSentenceCase(c_text: *const c_char) -> *mut c_char {
    if c_text.is_null() {
        return std::ptr::null_mut();
    }
    let text = unsafe { CStr::from_ptr(c_text).to_string_lossy() };
    let converted = textfx::to_sentence_case(&text);
    CString::new(converted).unwrap().into_raw()
}

#[no_mangle]
pub extern "C" fn GeanyRust_TextFX_TrimTrailingWhitespace(c_text: *const c_char) -> *mut c_char {
    if c_text.is_null() {
        return std::ptr::null_mut();
    }
    let text = unsafe { CStr::from_ptr(c_text).to_string_lossy() };
    let formatted = textfx::trim_trailing_whitespace(&text);
    CString::new(formatted).unwrap().into_raw()
}
