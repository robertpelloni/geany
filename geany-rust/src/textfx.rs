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
