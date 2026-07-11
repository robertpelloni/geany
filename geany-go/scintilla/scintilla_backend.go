package scintilla

import "github.com/geany/geany-go/engine"

// Ensure ScintillaEditor implements engine.Backend
var _ engine.Backend = (*ScintillaEditor)(nil)

func (se *ScintillaEditor) GetText() string {
	length := se.GetTextLength()
	text, _ := se.GetRawText(length)
	return text
}

func (se *ScintillaEditor) SetText(text string) {
	se.SendMessage(2004, 0, 0) // SCI_CLEARALL
	se.InsertText(0, text)
}

func (se *ScintillaEditor) GetTextRange(start, end int) string {
    // Stub implementation to satisfy interface
    return ""
}

func (se *ScintillaEditor) DeleteRange(start, end int) {
    // Stub
}

func (se *ScintillaEditor) GetLineCount() int {
	return int(se.SendMessage(2154, 0, 0)) // SCI_GETLINECOUNT
}

func (se *ScintillaEditor) GetLineLength(line int) int {
	return int(se.SendMessage(2350, uint64(line), 0)) // SCI_LINELENGTH
}

func (se *ScintillaEditor) SetReadOnly(ro bool) {
	val := uint64(0)
	if ro {
		val = 1
	}
	se.SendMessage(2171, val, 0) // SCI_SETREADONLY
}

func (se *ScintillaEditor) IsReadOnly() bool {
	return se.SendMessage(2172, 0, 0) != 0 // SCI_GETREADONLY
}

func (se *ScintillaEditor) GetCursorPos() int {
	return int(se.SendMessage(2008, 0, 0)) // SCI_GETCURRENTPOS
}

func (se *ScintillaEditor) SetCursorPos(pos int) {
	se.SendMessage(2025, uint64(pos), 0) // SCI_GOTOPOS
}

func (se *ScintillaEditor) GetSelection() (int, int) {
	start := int(se.SendMessage(2143, 0, 0)) // SCI_GETSELECTIONSTART
	end := int(se.SendMessage(2145, 0, 0))   // SCI_GETSELECTIONEND
	return start, end
}

func (se *ScintillaEditor) SetSelection(start, end int) {
	se.SendMessage(2160, uint64(start), int64(end)) // SCI_SETSEL
}
