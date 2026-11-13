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

func TestRemoveBlankLines(t *testing.T) {
	input := "hello\n\n  \nworld\n\n!"
	expected := "hello\nworld\n!"
	if res := RemoveBlankLines(input); res != expected {
		t.Errorf("Expected %q, got %q", expected, res)
	}
}

func TestRemoveDuplicateLines(t *testing.T) {
	input := "a\na\nb\nc\nc\nc\nd"
	expected := "a\nb\nc\nd"
	if res := RemoveDuplicateLines(input); res != expected {
		t.Errorf("Expected %q, got %q", expected, res)
	}
}

func TestEncodeBase64(t *testing.T) {
	input := "Hello, Geany Ultra!"
	encoded := EncodeBase64(input)
	expected := "SGVsbG8sIEdlYW55IFVsdHJhIQ=="
	if encoded != expected {
		t.Errorf("Expected %q, got %q", expected, encoded)
	}
}

func TestDecodeBase64(t *testing.T) {
	input := "SGVsbG8sIEdlYW55IFVsdHJhIQ=="
	decoded, err := DecodeBase64(input)
	if err != nil {
		t.Fatalf("Unexpected error decoding base64: %v", err)
	}
	expected := "Hello, Geany Ultra!"
	if decoded != expected {
		t.Errorf("Expected %q, got %q", expected, decoded)
	}

	// Invalid input test
	_, err = DecodeBase64("!!!invalid-base64!!!")
	if err == nil {
		t.Errorf("Expected error for invalid base64, got nil")
	}
}
