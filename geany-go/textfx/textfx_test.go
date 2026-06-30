package textfx

import "testing"

func TestSortLines(t *testing.T) {
	input := "banana\nApple\nCherry"
	expectedInsensitive := "Apple\nbanana\nCherry"
	if res := SortLines(input, true, false); res != expectedInsensitive {
		t.Errorf("Expected %q, got %q", expectedInsensitive, res)
	}

	expectedSensitive := "Apple\nCherry\nbanana"
	if res := SortLines(input, true, true); res != expectedSensitive {
		t.Errorf("Expected %q, got %q", expectedSensitive, res)
	}
}

func TestToProperCase(t *testing.T) {
	input := "hello world from go"
	expected := "Hello World From Go"
	if res := ToProperCase(input); res != expected {
		t.Errorf("Expected %q, got %q", expected, res)
	}
}

func TestToSentenceCase(t *testing.T) {
	input := "hello world! how are you?"
	expected := "Hello world! How are you?"
	if res := ToSentenceCase(input); res != expected {
		t.Errorf("Expected %q, got %q", expected, res)
	}
}
