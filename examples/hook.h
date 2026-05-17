/**
 * Define hook macros that will be inserted into the wrap functions.
 * It's important to use the GDO_RAWPTR_* prefix to call the actual function pointer.
 * Parameter names are taken from the function prototype declarations used to
 * generate the header. The macros must be defined before the header is included.
 */

// gpointer g_malloc (gsize n_bytes);
#define GDO_HOOK_g_malloc(...) \
    do { \
      gpointer ptr = GDO_RAWPTR_g_malloc(__VA_ARGS__); \
      printf("memory address %p: %zu bytes allocated using g_malloc()\n", &ptr, n_bytes); \
      return ptr; \
    } while(0)

// gpointer g_realloc (gpointer mem, gsize n_bytes);
#define GDO_HOOK_g_realloc(...) \
    do { \
      gpointer ptr = GDO_RAWPTR_g_realloc(__VA_ARGS__); \
      printf("memory address %p: %zu bytes allocated using g_realloc()\n", &ptr, n_bytes); \
      return ptr; \
    } while(0)

// void g_free_sized (gpointer mem, size_t size);
#define GDO_HOOK_g_free_sized(...) \
    do { \
      GDO_RAWPTR_g_free_sized(__VA_ARGS__); \
      printf("memory address %p: %zu bytes memory released using g_free_sized()\n", &mem, size); \
      return; \
    } while(0)

