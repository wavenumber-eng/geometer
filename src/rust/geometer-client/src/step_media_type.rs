//! Bounded Content-Type syntax at STEP input attachment boundaries only.
pub(crate) fn canonical(value: &str) -> Option<&'static str> {
    let bytes = value.as_bytes();
    if bytes.is_empty()
        || bytes.len() > 128
        || !bytes
            .iter()
            .all(|byte| *byte == b'\t' || (32..=126).contains(byte))
    {
        return None;
    }
    let mut parser = Parser { bytes, position: 0 };
    let kind = parser.token()?;
    parser.expect(b'/')?;
    let subtype = parser.token()?;
    if !(kind.eq_ignore_ascii_case(b"model") || kind.eq_ignore_ascii_case(b"application"))
        || !subtype.eq_ignore_ascii_case(b"step")
    {
        return None;
    }
    parser.parameters()?;
    Some("model/step")
}

struct Parser<'a> {
    bytes: &'a [u8],
    position: usize,
}
impl<'a> Parser<'a> {
    fn peek(&self) -> Option<u8> {
        self.bytes.get(self.position).copied()
    }

    fn cfws(&mut self) -> Option<()> {
        loop {
            while matches!(self.peek(), Some(b' ' | b'\t')) {
                self.position += 1;
            }
            if self.peek() != Some(b'(') {
                return Some(());
            }
            self.comment()?;
        }
    }

    fn comment(&mut self) -> Option<()> {
        self.position += 1;
        let mut depth = 1;
        while let Some(byte) = self.peek() {
            self.position += 1;
            match byte {
                b'\\' => {
                    self.peek()?;
                    self.position += 1;
                }
                b'(' => depth += 1,
                b')' => {
                    depth -= 1;
                    if depth == 0 {
                        return Some(());
                    }
                }
                _ => {}
            }
        }
        None
    }

    fn token(&mut self) -> Option<&'a [u8]> {
        self.cfws()?;
        let start = self.position;
        while self.peek().is_some_and(token_byte) {
            self.position += 1;
        }
        (self.position > start).then_some(&self.bytes[start..self.position])
    }

    fn expect(&mut self, byte: u8) -> Option<()> {
        self.cfws()?;
        if self.peek()? != byte {
            return None;
        }
        self.position += 1;
        Some(())
    }

    fn quoted(&mut self) -> Option<()> {
        self.position += 1;
        while let Some(byte) = self.peek() {
            self.position += 1;
            match byte {
                b'"' => return Some(()),
                b'\\' => {
                    self.peek()?;
                    self.position += 1;
                }
                _ => {}
            }
        }
        None
    }

    fn parameters(&mut self) -> Option<()> {
        loop {
            self.cfws()?;
            if self.peek().is_none() {
                return Some(());
            }
            self.expect(b';')?;
            self.token()?;
            self.expect(b'=')?;
            self.cfws()?;
            if self.peek() == Some(b'"') {
                self.quoted()?;
            } else {
                self.token()?;
            }
        }
    }
}

fn token_byte(byte: u8) -> bool {
    byte > 32 && byte < 127 && !b"()<>@,;:\\\"/[]?=".contains(&byte)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn shared_step_content_type_vectors_and_bounds() {
        let fixtures: serde_json::Value = serde_json::from_str(include_str!(
            "../../../../tests/fixtures/step_media_types.json"
        ))
        .unwrap();
        for row in fixtures["cases"].as_array().unwrap() {
            let value = row["value"].as_str().unwrap();
            assert_eq!(
                canonical(value),
                row["accepted"].as_bool().unwrap().then_some("model/step"),
                "{value:?}"
            );
        }
        let at_limit = format!("model/step{}", " ".repeat(118));
        assert_eq!(at_limit.len(), 128);
        assert_eq!(canonical(&at_limit), Some("model/step"));
        assert_eq!(canonical(&(at_limit + " ")), None);
        assert_eq!(canonical("model/step\u{7f}"), None);
    }
}
