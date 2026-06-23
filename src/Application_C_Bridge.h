#ifndef APPLICATION_C_BRIDGE_H
#define APPLICATION_C_BRIDGE_H

#ifdef __cplusplus
#include <stdbool.h>
extern "C" {
#else
#include <stdbool.h>
#endif

// Opaque handle for the C code to hold the C++ Application pointer
typedef void* GeanyApplicationHandle;

// C API to interact with the C++ Application object
GeanyApplicationHandle geany_application_new(void);
void geany_application_free(GeanyApplicationHandle handle);

int geany_application_initialize(GeanyApplicationHandle handle, int argc, char** argv);
int geany_application_run(GeanyApplicationHandle handle);
void geany_application_quit(GeanyApplicationHandle handle);

// Go Backend FFI Declarations
void GeanyGo_Initialize(void);
void GeanyGo_Shutdown(void);
void GeanyGo_Scintilla_Bind(long long fnPtr, long long objPtr);
void GeanyGo_FreeString(char* s);
int GeanyGo_Editor_OpenDocument(const char* path);
void GeanyGo_Config_SetString(const char* section, const char* key, const char* value);
char* GeanyGo_Config_GetString(const char* section, const char* key, const char* fallback);
char* GeanyGo_TextFX_SortLines(const char* text, bool ascending, bool caseSensitive);
char* GeanyGo_TextFX_ToProperCase(const char* text);
char* GeanyGo_TextFX_ToSentenceCase(const char* text);
char* GeanyGo_TextFX_TrimTrailingWhitespace(const char* text);
void GeanyGo_UI_SetTabOrientation(bool vertical);

// Rust Backend FFI Declarations
void GeanyRust_Initialize(void);
void GeanyRust_Shutdown(void);
void GeanyRust_FreeString(char* s);
void GeanyRust_UI_SetTabOrientation(bool vertical);
char* GeanyRust_TextFX_SortLines(const char* text, bool ascending, bool case_sensitive);
char* GeanyRust_TextFX_ToProperCase(const char* text);
char* GeanyRust_TextFX_ToSentenceCase(const char* text);
char* GeanyRust_TextFX_TrimTrailingWhitespace(const char* text);

#ifdef __cplusplus
}
#endif

#endif // APPLICATION_C_BRIDGE_H
