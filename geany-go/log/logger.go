package log

import (
	"fmt"
	"io"
	"os"
	"sync"
	"time"
)

// Level categorizes the severity or source of a log message.
type Level int

const (
	LevelDebug Level = iota
	LevelInfo
	LevelWarning
	LevelError
	LevelCompiler // Specific to IDE build output
	LevelSearch   // Specific to IDE search output
)

// String representation of log levels for formatting.
func (l Level) String() string {
	switch l {
	case LevelDebug:
		return "DEBUG"
	case LevelInfo:
		return "INFO"
	case LevelWarning:
		return "WARN"
	case LevelError:
		return "ERROR"
	case LevelCompiler:
		return "COMPILER"
	case LevelSearch:
		return "SEARCH"
	default:
		return "UNKNOWN"
	}
}

// Entry represents a single, structured log event.
type Entry struct {
	Timestamp time.Time
	Level     Level
	Message   string
	Context   string // e.g. "main.c", "PluginLoader"
}

// Logger provides thread-safe, leveled logging.
// This is the Go port of the logging engine in src/log.c and msgwindow.c.
type Logger struct {
	mu       sync.Mutex
	out      io.Writer
	minLevel Level
	history  []Entry
	maxHist  int
}

// NewLogger initializes a structured logger.
func NewLogger(out io.Writer, minLevel Level) *Logger {
	if out == nil {
		out = os.Stdout
	}
	return &Logger{
		out:      out,
		minLevel: minLevel,
		history:  make([]Entry, 0),
		maxHist:  1000, // Keep last 1000 messages in memory for UI presentation
	}
}

// log Internal logging logic.
func (l *Logger) log(level Level, context, format string, args ...interface{}) {
	if level < l.minLevel {
		return
	}

	msg := fmt.Sprintf(format, args...)
	entry := Entry{
		Timestamp: time.Now(),
		Level:     level,
		Message:   msg,
		Context:   context,
	}

	l.mu.Lock()
	defer l.mu.Unlock()

	// Append to history
	l.history = append(l.history, entry)
	if len(l.history) > l.maxHist {
		l.history = l.history[1:] // Truncate oldest
	}

	// Write to output stream
	ctxStr := ""
	if context != "" {
		ctxStr = "[" + context + "] "
	}

	fmt.Fprintf(l.out, "%s [%s] %s%s\n", entry.Timestamp.Format("15:04:05"), level.String(), ctxStr, msg)
}

func (l *Logger) Debug(context, format string, args ...interface{}) { l.log(LevelDebug, context, format, args...) }
func (l *Logger) Info(context, format string, args ...interface{})  { l.log(LevelInfo, context, format, args...) }
func (l *Logger) Warn(context, format string, args ...interface{})  { l.log(LevelWarning, context, format, args...) }
func (l *Logger) Error(context, format string, args ...interface{}) { l.log(LevelError, context, format, args...) }
func (l *Logger) Compiler(context, format string, args ...interface{}) { l.log(LevelCompiler, context, format, args...) }
func (l *Logger) Search(context, format string, args ...interface{}) { l.log(LevelSearch, context, format, args...) }

// GetHistory returns a copy of the recent log entries (useful for the UI Message Window).
func (l *Logger) GetHistory() []Entry {
	l.mu.Lock()
	defer l.mu.Unlock()
	res := make([]Entry, len(l.history))
	copy(res, l.history)
	return res
}

// ClearHistory wipes the internal log buffer.
func (l *Logger) ClearHistory() {
	l.mu.Lock()
	defer l.mu.Unlock()
	l.history = make([]Entry, 0)
}
