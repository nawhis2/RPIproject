#include "server.h"

extern pthread_mutex_t led_mutex;
extern pthread_mutex_t pr_mutex;
extern int led_light;
extern int pr_light;

typedef int (*FUNC)(void*);
void* pr_thread(void *arg)
{
    FUNC prfunc;
    void* handle;
    if ((handle = dlopen("libpr.so", RTLD_LAZY)) == NULL)
    {
        fprintf(stderr, "dlopen error: %s\n", dlerror());
        return NULL;
    }
    if ((prfunc = (FUNC)dlsym(handle, "PR_control")) == NULL)
    {
        fprintf(stderr, "dlsym error : %s\n", dlerror());
        dlclose(handle);
        return NULL;
    }
    
    arg_t args = {&led_mutex, &led_light};

    pthread_mutex_lock(&pr_mutex);
    pr_light = prfunc(&args);
    pthread_mutex_unlock(&pr_mutex);
    
    dlclose(handle);
    return NULL;
}