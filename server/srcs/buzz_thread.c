#include "server.h"

extern pthread_mutex_t buzz_mutex;
extern int music_stop;

typedef void (*FUNC)(void*);
void* buzz_thread(void *arg)
{
    FUNC buzzfunc;
    void* handle;
    if ((handle = dlopen("libbuzz.so", RTLD_LAZY)) == NULL)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return NULL;
    }

    if ((buzzfunc = (FUNC)dlsym(handle, "buzz_control")) == NULL)
    {
        fprintf(stderr, "dlsym error : %s\n", dlerror());
        dlclose(handle);
        return NULL;
    }
    
    arg_t args = {&buzz_mutex, &music_stop};

    buzzfunc(&args);
    music_stop = 0;
    dlclose(handle);
    return NULL;
}