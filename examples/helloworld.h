#ifndef _HELLOWORLD_H_
#define _HELLOWORLD_H_

#ifdef __cplusplus
extern "C" {
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

/* initialize */
DLL_PUBLIC helloworld *helloworld_init();

/* pass a "hello world" string to a callback function */
DLL_PUBLIC void helloworld_hello(helloworld *hw, void (*callback)(const char *));

/* free resources */
DLL_PUBLIC void helloworld_release(helloworld *hw);

#ifdef __cplusplus
}
#endif

#endif //_HELLOWORLD_H_
