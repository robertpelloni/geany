package parser

import (
	"unicode"
)

// TokenType designates lexical classifications of source tokens.
type TokenType int

const (
	TokenEOF TokenType = iota
	TokenError
	TokenIdentifier
	TokenKeyword
	TokenString
	TokenNumber
	TokenComment
	TokenPunctuation
	TokenOperator
)

// Token captures an atomic lexical element within the source code.
type Token struct {
	Type   TokenType
	Value  string
	Line   int
	Column int
}

// Lexer breaks raw source text into a sequential stream of tokens.
// It is designed to be error-resilient, safely handling unclosed strings, comments, and invalid runes.
type Lexer struct {
	input   []rune
	pos     int
	line    int
	col     int
	length  int
}

// NewLexer creates a fresh lexer instance ready to tokenize input.
func NewLexer(input string) *Lexer {
	r := []rune(input)
	return &Lexer{
		input:  r,
		pos:    0,
		line:   1,
		col:    1,
		length: len(r),
	}
}

// peek inspects the current rune without consuming it.
func (l *Lexer) peek() rune {
	if l.pos >= l.length {
		return 0
	}
	return l.input[l.pos]
}

// peekNext inspects the rune following the current position without advancing.
func (l *Lexer) peekNext() rune {
	if l.pos+1 >= l.length {
		return 0
	}
	return l.input[l.pos+1]
}

// read consumes and returns the next rune, advancing line and column counters.
func (l *Lexer) read() rune {
	if l.pos >= l.length {
		return 0
	}
	ch := l.input[l.pos]
	l.pos++
	if ch == '\n' {
		l.line++
		l.col = 1
	} else {
		l.col++
	}
	return ch
}

// NextToken scans and returns the next token from the input.
func (l *Lexer) NextToken() Token {
	l.skipWhitespace()

	if l.pos >= l.length {
		return Token{Type: TokenEOF, Line: l.line, Column: l.col}
	}

	startLine := l.line
	startCol := l.col
	ch := l.peek()

	// Line or block comments
	if ch == '/' {
		next := l.peekNext()
		if next == '/' {
			return l.readLineComment(startLine, startCol)
		} else if next == '*' {
			return l.readBlockComment(startLine, startCol)
		}
	} else if ch == '#' {
		// Shell / Python style comment
		return l.readLineComment(startLine, startCol)
	}

	// String literals
	if ch == '"' || ch == '\'' || ch == '`' {
		return l.readString(ch, startLine, startCol)
	}

	// Numbers
	if unicode.IsDigit(ch) {
		return l.readNumber(startLine, startCol)
	}

	// Identifiers and keywords
	if isIdentStart(ch) {
		return l.readIdentifier(startLine, startCol)
	}

	// Punctuation & operators
	l.read()
	return Token{
		Type:   TokenPunctuation,
		Value:  string(ch),
		Line:   startLine,
		Column: startCol,
	}
}

func (l *Lexer) skipWhitespace() {
	for l.pos < l.length {
		ch := l.peek()
		if ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n' {
			l.read()
		} else {
			break
		}
	}
}

func (l *Lexer) readLineComment(line, col int) Token {
	var val []rune
	for l.pos < l.length {
		ch := l.peek()
		if ch == '\n' {
			break
		}
		val = append(val, l.read())
	}
	return Token{
		Type:   TokenComment,
		Value:  string(val),
		Line:   line,
		Column: col,
	}
}

func (l *Lexer) readBlockComment(line, col int) Token {
	var val []rune
	// Consume /*
	val = append(val, l.read())
	val = append(val, l.read())

	for l.pos < l.length {
		ch := l.read()
		val = append(val, ch)
		if ch == '*' && l.peek() == '/' {
			val = append(val, l.read()) // Consume /
			break
		}
	}
	return Token{
		Type:   TokenComment,
		Value:  string(val),
		Line:   line,
		Column: col,
	}
}

func (l *Lexer) readString(quote rune, line, col int) Token {
	var val []rune
	val = append(val, l.read()) // Consume opening quote

	for l.pos < l.length {
		ch := l.read()
		val = append(val, ch)
		if ch == '\\' && l.pos < l.length {
			// Escaped character
			val = append(val, l.read())
			continue
		}
		if ch == quote {
			break
		}
	}

	return Token{
		Type:   TokenString,
		Value:  string(val),
		Line:   line,
		Column: col,
	}
}

func (l *Lexer) readNumber(line, col int) Token {
	var val []rune
	for l.pos < l.length {
		ch := l.peek()
		if unicode.IsDigit(ch) || ch == '.' || ch == 'x' || ch == 'X' || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F') {
			val = append(val, l.read())
		} else {
			break
		}
	}
	return Token{
		Type:   TokenNumber,
		Value:  string(val),
		Line:   line,
		Column: col,
	}
}

func (l *Lexer) readIdentifier(line, col int) Token {
	var val []rune
	for l.pos < l.length {
		ch := l.peek()
		if isIdentPart(ch) {
			val = append(val, l.read())
		} else {
			break
		}
	}
	strVal := string(val)
	tokType := TokenIdentifier
	if isCommonKeyword(strVal) {
		tokType = TokenKeyword
	}
	return Token{
		Type:   tokType,
		Value:  strVal,
		Line:   line,
		Column: col,
	}
}

func isIdentStart(ch rune) bool {
	return unicode.IsLetter(ch) || ch == '_'
}

func isIdentPart(ch rune) bool {
	return unicode.IsLetter(ch) || unicode.IsDigit(ch) || ch == '_'
}

func isCommonKeyword(val string) bool {
	switch val {
	case "func", "type", "struct", "interface", "package", "var", "const", "return",
		"class", "def", "import", "from", "async", "await",
		"namespace", "template", "public", "private", "protected", "void", "int", "bool",
		"if", "else", "for", "while", "switch", "case", "break", "continue":
		return true
	default:
		return false
	}
}
