package parser

import (
	"strings"
	"sync"

	"github.com/geany/geany-go/symbols"
)

// Parser parses source code files into structured symbols.
// It is designed to be completely thread-safe and resilient against partial, broken,
// or incomplete code buffers during real-time typing in the editor.
type Parser struct {
	mu sync.RWMutex
}

// NewParser initializes a new parser engine instance.
func NewParser() *Parser {
	return &Parser{}
}

// ParseSource processes source code content according to the requested language
// and extracts all identified symbols (classes, functions, interfaces, structs, variables).
func (p *Parser) ParseSource(filename string, content string, lang string) ([]symbols.Symbol, error) {
	p.mu.Lock()
	defer p.mu.Unlock()

	lexer := NewLexer(content)
	var tokens []Token
	for {
		tok := lexer.NextToken()
		if tok.Type == TokenEOF {
			break
		}
		tokens = append(tokens, tok)
	}

	normLang := strings.ToLower(lang)
	switch normLang {
	case "go", "golang":
		return p.parseGo(filename, tokens), nil
	case "c", "cpp", "c++", "cxx", "h", "hpp":
		return p.parseCPP(filename, tokens), nil
	case "python", "py":
		return p.parsePython(filename, tokens), nil
	default:
		// Generic fallback symbol extraction
		return p.parseGeneric(filename, tokens), nil
	}
}

// parseGo parses Go language declarations: func, type Struct, type Interface, var, const.
func (p *Parser) parseGo(filename string, tokens []Token) []symbols.Symbol {
	var syms []symbols.Symbol
	n := len(tokens)

	for i := 0; i < n; i++ {
		tok := tokens[i]
		if tok.Type == TokenKeyword {
			switch tok.Value {
			case "func":
				// Function or method: func Name(...) or func (recv) Name(...)
				if i+1 < n {
					nextTok := tokens[i+1]
					if nextTok.Value == "(" {
						// Method: scan past receiver
						j := i + 2
						for j < n && tokens[j].Value != ")" {
							j++
						}
						if j+1 < n && tokens[j+1].Type == TokenIdentifier {
							methodName := tokens[j+1].Value
							sig := p.extractSignature(tokens, j+2)
							syms = append(syms, symbols.Symbol{
								Name:      methodName,
								File:      filename,
								Line:      tokens[j+1].Line,
								Kind:      symbols.KindFunction,
								Signature: sig,
							})
						}
					} else if nextTok.Type == TokenIdentifier {
						sig := p.extractSignature(tokens, i+2)
						syms = append(syms, symbols.Symbol{
							Name:      nextTok.Value,
							File:      filename,
							Line:      nextTok.Line,
							Kind:      symbols.KindFunction,
							Signature: sig,
						})
					}
				}

			case "type":
				// type Name struct / type Name interface
				if i+2 < n && tokens[i+1].Type == TokenIdentifier {
					typeName := tokens[i+1].Value
					kind := symbols.KindClass
					if tokens[i+2].Value == "struct" {
						kind = symbols.KindStruct
					} else if tokens[i+2].Value == "interface" {
						kind = symbols.KindInterface
					}
					syms = append(syms, symbols.Symbol{
						Name: typeName,
						File: filename,
						Line: tokens[i+1].Line,
						Kind: kind,
					})
				}

			case "var", "const":
				if i+1 < n && tokens[i+1].Type == TokenIdentifier {
					syms = append(syms, symbols.Symbol{
						Name: tokens[i+1].Value,
						File: filename,
						Line: tokens[i+1].Line,
						Kind: symbols.KindVariable,
					})
				}
			}
		}
	}
	return syms
}

// parseCPP extracts classes, structs, and functions from C/C++ source code.
func (p *Parser) parseCPP(filename string, tokens []Token) []symbols.Symbol {
	var syms []symbols.Symbol
	n := len(tokens)

	for i := 0; i < n; i++ {
		tok := tokens[i]

		// Class / Struct extraction: class MyClass {...} / struct MyStruct {...}
		if (tok.Value == "class" || tok.Value == "struct") && i+1 < n {
			if tokens[i+1].Type == TokenIdentifier {
				kind := symbols.KindClass
				if tok.Value == "struct" {
					kind = symbols.KindStruct
				}
				syms = append(syms, symbols.Symbol{
					Name: tokens[i+1].Value,
					File: filename,
					Line: tokens[i+1].Line,
					Kind: kind,
				})
			}
		}

		// Function definition: identifier followed by ( ... ) followed by {
		if (tok.Type == TokenIdentifier || tok.Type == TokenKeyword) && i+1 < n {
			if tokens[i+1].Value == "(" {
				// Avoid keywords like if, for, while, switch
				if !isControlFlowKeyword(tok.Value) {
					sig := p.extractSignature(tokens, i+1)
					syms = append(syms, symbols.Symbol{
						Name:      tok.Value,
						File:      filename,
						Line:      tok.Line,
						Kind:      symbols.KindFunction,
						Signature: sig,
					})
				}
			}
		}
	}
	return syms
}

// parsePython extracts def and class definitions from Python code.
func (p *Parser) parsePython(filename string, tokens []Token) []symbols.Symbol {
	var syms []symbols.Symbol
	n := len(tokens)

	for i := 0; i < n; i++ {
		tok := tokens[i]
		if tok.Value == "def" && i+1 < n && tokens[i+1].Type == TokenIdentifier {
			sig := p.extractSignature(tokens, i+2)
			syms = append(syms, symbols.Symbol{
				Name:      tokens[i+1].Value,
				File:      filename,
				Line:      tokens[i+1].Line,
				Kind:      symbols.KindFunction,
				Signature: sig,
			})
		} else if tok.Value == "class" && i+1 < n && tokens[i+1].Type == TokenIdentifier {
			syms = append(syms, symbols.Symbol{
				Name: tokens[i+1].Value,
				File: filename,
				Line: tokens[i+1].Line,
				Kind: symbols.KindClass,
			})
		}
	}
	return syms
}

// parseGeneric extracts any function-like patterns from unidentified languages.
func (p *Parser) parseGeneric(filename string, tokens []Token) []symbols.Symbol {
	var syms []symbols.Symbol
	n := len(tokens)

	for i := 0; i < n; i++ {
		tok := tokens[i]
		if tok.Type == TokenIdentifier && i+1 < n && tokens[i+1].Value == "(" {
			if !isControlFlowKeyword(tok.Value) {
				sig := p.extractSignature(tokens, i+1)
				syms = append(syms, symbols.Symbol{
					Name:      tok.Value,
					File:      filename,
					Line:      tok.Line,
					Kind:      symbols.KindFunction,
					Signature: sig,
				})
			}
		}
	}
	return syms
}

func (p *Parser) extractSignature(tokens []Token, start int) string {
	if start >= len(tokens) || tokens[start].Value != "(" {
		return ""
	}
	var b strings.Builder
	depth := 0
	for i := start; i < len(tokens); i++ {
		tok := tokens[i]
		b.WriteString(tok.Value)
		if tok.Value == "(" {
			depth++
		} else if tok.Value == ")" {
			depth--
			if depth <= 0 {
				break
			}
		}
		if i+1 < len(tokens) && tokens[i+1].Value != "," && tokens[i+1].Value != ")" && tok.Value != "(" {
			b.WriteString(" ")
		}
	}
	return b.String()
}

func isControlFlowKeyword(val string) bool {
	switch val {
	case "if", "for", "while", "switch", "catch", "return", "sizeof":
		return true
	default:
		return false
	}
}
