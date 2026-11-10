package btk

import (
	"fmt"
	"strings"
	"sync"

	"github.com/geany/geany-go/ui"
)

// TabWidget manages Qt4/CopperSpice tab widgets with support for horizontal and vertical tabs.
type TabWidget struct {
	mu          sync.RWMutex
	tabs        []string
	active      int
	orientation ui.TabOrientation
	multiLine   bool
}

func NewTabWidget() *TabWidget {
	return &TabWidget{
		tabs:        make([]string, 0),
		active:      -1,
		orientation: ui.Horizontal,
		multiLine:   false,
	}
}

func (tw *TabWidget) AddTab(title string) int {
	tw.mu.Lock()
	defer tw.mu.Unlock()
	tw.tabs = append(tw.tabs, title)
	if tw.active == -1 {
		tw.active = 0
	}
	return len(tw.tabs) - 1
}

func (tw *TabWidget) RemoveTab(idx int) error {
	tw.mu.Lock()
	defer tw.mu.Unlock()
	if idx < 0 || idx >= len(tw.tabs) {
		return fmt.Errorf("invalid tab index: %d", idx)
	}
	tw.tabs = append(tw.tabs[:idx], tw.tabs[idx+1:]...)
	if tw.active >= len(tw.tabs) {
		tw.active = len(tw.tabs) - 1
	}
	return nil
}

func (tw *TabWidget) SetOrientation(o ui.TabOrientation) {
	tw.mu.Lock()
	defer tw.mu.Unlock()
	tw.orientation = o
}

func (tw *TabWidget) SetMultiLine(enable bool) {
	tw.mu.Lock()
	defer tw.mu.Unlock()
	tw.multiLine = enable
}

func (tw *TabWidget) Count() int {
	tw.mu.RLock()
	defer tw.mu.RUnlock()
	return len(tw.tabs)
}

// StatusBar represents the IDE status strip for CopperSpice/Qt4 displaying coordinates, encoding, and filetype.
type StatusBar struct {
	mu       sync.RWMutex
	line     int
	col      int
	encoding string
	fileType string
	message  string
}

func NewStatusBar() *StatusBar {
	return &StatusBar{
		line:     1,
		col:      1,
		encoding: "UTF-8",
		fileType: "None",
	}
}

func (sb *StatusBar) SetPosition(line, col int) {
	sb.mu.Lock()
	defer sb.mu.Unlock()
	sb.line = line
	sb.col = col
}

func (sb *StatusBar) SetEncoding(enc string) {
	sb.mu.Lock()
	defer sb.mu.Unlock()
	sb.encoding = enc
}

func (sb *StatusBar) SetFileType(ft string) {
	sb.mu.Lock()
	defer sb.mu.Unlock()
	sb.fileType = ft
}

func (sb *StatusBar) SetMessage(msg string) {
	sb.mu.Lock()
	defer sb.mu.Unlock()
	sb.message = msg
}

func (sb *StatusBar) GetText() string {
	sb.mu.RLock()
	defer sb.mu.RUnlock()
	return fmt.Sprintf("Ln %d, Col %d | %s | %s | %s", sb.line, sb.col, sb.encoding, sb.fileType, sb.message)
}

// SearchBar implements incremental quick-search for the BTK CopperSpice editor.
type SearchBar struct {
	mu           sync.RWMutex
	query        string
	matchCase    bool
	resultsCount int
}

func NewSearchBar() *SearchBar {
	return &SearchBar{
		matchCase: false,
	}
}

func (sb *SearchBar) SetQuery(query string) {
	sb.mu.Lock()
	defer sb.mu.Unlock()
	sb.query = query
}

func (sb *SearchBar) ExecuteSearch(content string) int {
	sb.mu.Lock()
	defer sb.mu.Unlock()
	if sb.query == "" {
		sb.resultsCount = 0
		return 0
	}

	searchContent := content
	query := sb.query
	if !sb.matchCase {
		searchContent = strings.ToLower(content)
		query = strings.ToLower(query)
	}

	count := strings.Count(searchContent, query)
	sb.resultsCount = count
	return count
}

func (sb *SearchBar) GetResultsCount() int {
	sb.mu.RLock()
	defer sb.mu.RUnlock()
	return sb.resultsCount
}
