package scintilla

/*
#cgo CFLAGS: -I../../scintilla/include -I../../scintilla/lexilla/include
#include "Scintilla.h"
#include "SciLexer.h"
#include <stdlib.h>
#include <stdint.h>

typedef intptr_t (*SciFnDirect)(intptr_t ptr, unsigned int iMessage, uintptr_t wParam, intptr_t lParam);

intptr_t sc_send_message(void* fn, intptr_t ptr, unsigned int msg, uintptr_t wp, intptr_t lp) {
    if (!fn) return 0;
    SciFnDirect fnPtr = (SciFnDirect)fn;
    return fnPtr(ptr, msg, wp, lp);
}
*/
import "C"

import (
	"fmt"
	"unsafe"
)

// ScintillaEditor wraps the C/C++ Scintilla API using CGO.
type ScintillaEditor struct {
	sciPtr C.intptr_t
	sciFn  unsafe.Pointer
}

func NewScintillaEditor(fnPtr, objPtr int64) *ScintillaEditor {
	return &ScintillaEditor{
		sciFn:  unsafe.Pointer(uintptr(fnPtr)),
		sciPtr: C.intptr_t(objPtr),
	}
}

func (se *ScintillaEditor) SendMessage(msg uint, wp uint64, lp int64) int64 {
	if se.sciFn == nil {
		return 0
	}
	res := C.sc_send_message(se.sciFn, se.sciPtr, C.uint(msg), C.uintptr_t(wp), C.intptr_t(lp))
	return int64(res)
}

func (se *ScintillaEditor) Init() {
	se.SendMessage(C.SCI_CLEARALL, 0, 0)
	se.SendMessage(C.SCI_SETCODEPAGE, C.SC_CP_UTF8, 0)
}

func (se *ScintillaEditor) InsertText(pos int, text string) {
	cStr := C.CString(text)
	defer C.free(unsafe.Pointer(cStr))
	se.SendMessage(C.SCI_INSERTTEXT, uint64(pos), int64(uintptr(unsafe.Pointer(cStr))))
}

func (se *ScintillaEditor) SetPosition(pos int) {
	se.SendMessage(C.SCI_GOTOPOS, uint64(pos), 0)
}

func (se *ScintillaEditor) GetTextLength() int {
	return int(se.SendMessage(C.SCI_GETLENGTH, 0, 0))
}

func (se *ScintillaEditor) GetText(length int) (string, error) {
	if length <= 0 {
		return "", nil
	}
	buf := make([]byte, length+1)
	res := se.SendMessage(C.SCI_GETTEXT, uint64(length+1), int64(uintptr(unsafe.Pointer(&buf[0]))))
	if res == 0 {
		return "", fmt.Errorf("failed to get text")
	}
	return C.GoString((*C.char)(unsafe.Pointer(&buf[0]))), nil
}
