#include "server.h"

extern pthread_mutex_t buzz_mutex;
extern pthread_mutex_t seg_mutex;

typedef void (*FUNC)(void*);
void* seg_thread(void *arg)
{
    FUNC segfunc;
    void* handle;
    if ((handle = dlopen("libseg.so", RTLD_LAZY)) == NULL)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return NULL;
    }

    if ((segfunc = (FUNC)dlsym(handle, "SEG_control")) == NULL)
    {
        fprintf(stderr, "dlsym error : %s\n", dlerror());
        dlclose(handle);
        return NULL;
    }
    
    arg_t args = {&buzz_mutex, arg};

    if (pthread_mutex_trylock(&seg_mutex))
    {
        fprintf(stderr, "7-segment is using now\n");
        return NULL;
    }
    segfunc(&args);
    pthread_mutex_unlock(&seg_mutex);
    dlclose(handle);
    return NULL;
}