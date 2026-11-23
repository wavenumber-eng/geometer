"""Bounded RFC 2045 STEP input recognition; attachment bytes are never transformed."""

STEP_MEDIA_TYPE = "model/step"
_SPECIALS = frozenset('()<>@,;:\\"/[]?=')


class _Invalid(ValueError):
    pass


class _Field:
    def __init__(self, value: str) -> None:
        self.value = value
        self.position = 0

    def take(self, expected: str) -> None:
        self.space()
        if self.position >= len(self.value) or self.value[self.position] != expected:
            raise _Invalid
        self.position += 1

    def token(self) -> str:
        self.space()
        start = self.position
        while self.position < len(self.value):
            character = self.value[self.position]
            if character in _SPECIALS or character in " \t":
                break
            self.position += 1
        if start == self.position:
            raise _Invalid
        return self.value[start:self.position]

    def space(self) -> None:
        while self.position < len(self.value):
            character = self.value[self.position]
            if character in " \t":
                self.position += 1
            elif character == "(":
                self.comment()
            else:
                return

    def comment(self) -> None:
        depth = 1
        self.position += 1
        while self.position < len(self.value):
            character = self.value[self.position]
            self.position += 1
            if character == "\\":
                if self.position >= len(self.value):
                    raise _Invalid
                self.position += 1
            elif character == "(":
                depth += 1
            elif character == ")":
                depth -= 1
                if depth == 0:
                    return
        raise _Invalid

    def parameter_value(self) -> None:
        self.space()
        if self.position >= len(self.value) or self.value[self.position] != '"':
            self.token()
            return
        self.position += 1
        while self.position < len(self.value):
            character = self.value[self.position]
            self.position += 1
            if character == '"':
                return
            if character == "\\":
                if self.position >= len(self.value):
                    raise _Invalid
                self.position += 1
        raise _Invalid


def canonical_step_media_type(value: str) -> str | None:
    """Admit a complete ASCII field of at most 128 bytes, including valid parameters.

    Unknown parameters have no decoder semantics. Framed fields exclude CR/LF,
    NUL and other controls; nested comments and quoted pairs remain supported.
    """
    if not 0 < len(value) <= 128 or any(
        ord(char) > 126 or (ord(char) < 32 and char != "\t") for char in value
    ):
        return None
    field = _Field(value)
    try:
        major = field.token().lower()
        field.take("/")
        minor = field.token().lower()
        if major not in ("model", "application") or minor != "step":
            return None
        field.space()
        while field.position < len(value):
            field.take(";")
            field.token()
            field.take("=")
            field.parameter_value()
            field.space()
    except _Invalid:
        return None
    return STEP_MEDIA_TYPE
