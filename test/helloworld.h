#ifndef _HELLOWORLD_H_
#define _HELLOWORLD_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CLANG_AST
#include <stdio.h>
#endif

#ifdef BUILDING_STATIC
    #define DLL_PUBLIC
#endif
#ifndef DLL_PUBLIC
    #if defined _WIN32 || defined __CYGWIN__
        #ifdef BUILDING_DLL
            #define DLL_PUBLIC __declspec(dllexport)
        #else
            #define DLL_PUBLIC __declspec(dllimport)
        #endif
    #elif defined __GNUC__
        #define DLL_PUBLIC __attribute__ ((visibility ("default")))
    #else
        #define DLL_PUBLIC
    #endif
#endif

typedef struct helloworld_ helloworld;
#ifndef CLANG_AST
typedef void (*helloworld_cb_t)(const char *);
#endif

/* pointer to callback function */
#ifdef CLANG_AST
void (*helloworld_callback)(const char *);
#else
extern DLL_PUBLIC helloworld_cb_t helloworld_callback;
#endif

/* array type */
extern DLL_PUBLIC char helloworld_buffer[64];

/* initialize */
DLL_PUBLIC helloworld *helloworld_init();
DLL_PUBLIC helloworld *helloworld_init_argv(int argc, char *argv[]);

/* pass a "hello world" string to a callback function */
DLL_PUBLIC void helloworld_hello(helloworld *hw);
DLL_PUBLIC void helloworld_hello2(helloworld *hw, void (*helloworld_cb)(const char *));

/* free resources */
DLL_PUBLIC void helloworld_release(helloworld *hw);

/* like fprintf() */
#ifndef CLANG_AST
DLL_PUBLIC int helloworld_fprintf(FILE *stream, const char *format, ...);
#endif

#ifdef __cplusplus
}
#endif

#endif //_HELLOWORLD_H_
