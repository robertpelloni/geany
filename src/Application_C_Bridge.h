#ifndef APPLICATION_C_BRIDGE_H
#define APPLICATION_C_BRIDGE_H

#ifdef __cplusplus
extern "C" {
#endif

// Opaque handle for the C code to hold the C++ Application pointer
typedef void* GeanyApplicationHandle;

// C API to interact with the C++ Application object
GeanyApplicationHandle geany_application_new(void);
void geany_application_free(GeanyApplicationHandle handle);

int geany_application_initialize(GeanyApplicationHandle handle, int argc, char** argv);
int geany_application_run(GeanyApplicationHandle handle);
void geany_application_quit(GeanyApplicationHandle handle);

#ifdef __cplusplus
}
#endif

#endif // APPLICATION_C_BRIDGE_H
