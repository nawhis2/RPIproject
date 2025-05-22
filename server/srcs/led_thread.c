#include "server.h"

extern int led_light;
extern pthread_mutex_t led_mutex;

typedef int (*FUNC)(void *);
void* led_thread(void *arg)
{
    FUNC ledfunc;
    void* handle = dlopen("libled.so", RTLD_LAZY);
    
    ledfunc = (FUNC)dlsym(handle, "LED_control");
    pthread_mutex_lock(&led_mutex);
    led_light = ledfunc(arg);
    pthread_mutex_unlock(&led_mutex);
    dlclose(handle);
    return NULL;
}