pub fn sort_lines(text: &str, ascending: bool, case_sensitive: bool) -> String {
    let mut lines: Vec<&str> = text.lines().collect();

    lines.sort_by(|a, b| {
        let cmp = if case_sensitive {
            a.cmp(b)
        } else {
            a.to_lowercase().cmp(&b.to_lowercase())
        };

        if ascending {
            cmp
        } else {
            cmp.reverse()
        }
    });

    lines.join("\n")
}

pub fn to_sentence_case(text: &str) -> String {
    let mut result = String::with_capacity(text.len());
    let mut capitalize_next = true;

    for c in text.chars() {
        if capitalize_next && c.is_alphabetic() {
            for upper in c.to_uppercase() {
                result.push(upper);
            }
            capitalize_next = false;
        } else {
            for lower in c.to_lowercase() {
                result.push(lower);
            }
            if c == '.' || c == '!' || c == '?' {
                capitalize_next = true;
            } else if !c.is_whitespace() {
                capitalize_next = false;
            }
        }
    }

    result
}

pub fn to_proper_case(text: &str) -> String {
    let mut result = String::with_capacity(text.len());
    let mut capitalize_next = true;

    for c in text.chars() {
        if c.is_whitespace() {
            result.push(c);
            capitalize_next = true;
        } else if capitalize_next {
            for upper in c.to_uppercase() {
                result.push(upper);
            }
            capitalize_next = false;
        } else {
            for lower in c.to_lowercase() {
                result.push(lower);
            }
        }
    }

    result
}

pub fn trim_trailing_whitespace(text: &str) -> String {
    text.lines()
        .map(|line| line.trim_end())
        .collect::<Vec<_>>()
        .join("\n")
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_sort_lines() {
        let input = "banana\nApple\nCherry";
        assert_eq!(sort_lines(input, true, false), "Apple\nbanana\nCherry");
        assert_eq!(sort_lines(input, true, true), "Apple\nCherry\nbanana");
    }

    #[test]
    fn test_to_sentence_case() {
        let input = "hello world! how are you?";
        assert_eq!(to_sentence_case(input), "Hello world! How are you?");
    }

    #[test]
    fn test_to_proper_case() {
        let input = "hello world from rust";
        assert_eq!(to_proper_case(input), "Hello World From Rust");
    }
}

use base64::{Engine as _, engine::general_purpose};

pub fn remove_blank_lines(text: &str) -> String {
    let mut result = Vec::new();
    for line in text.lines() {
        if !line.trim().is_empty() {
            result.push(line);
        }
    }
    result.join("\n")
}

pub fn remove_duplicate_lines(text: &str) -> String {
    let lines: Vec<&str> = text.lines().collect();
    if lines.is_empty() {
        return String::new();
    }

    let mut result = vec![lines[0]];
    for i in 1..lines.len() {
        if lines[i] != lines[i-1] {
            result.push(lines[i]);
        }
    }
    result.join("\n")
}

pub fn encode_base64(text: &str) -> String {
    general_purpose::STANDARD.encode(text)
}

pub fn decode_base64(text: &str) -> String {
    match general_purpose::STANDARD.decode(text) {
        Ok(bytes) => String::from_utf8_lossy(&bytes).into_owned(),
        Err(_) => text.to_string(), // Return original on failure
    }
}

#[cfg(test)]
mod additional_tests {
    use super::*;

    #[test]
    fn test_remove_blank_lines() {
        assert_eq!(remove_blank_lines("A\n\nB\n  \nC"), "A\nB\nC");
    }

    #[test]
    fn test_remove_duplicate_lines() {
        assert_eq!(remove_duplicate_lines("A\nA\nB\nC\nC"), "A\nB\nC");
    }

    #[test]
    fn test_encode_decode_base64() {
        let text = "Hello Rust!";
        let encoded = encode_base64(text);
        assert_ne!(text, encoded);
        assert_eq!(decode_base64(&encoded), text);
    }
}
