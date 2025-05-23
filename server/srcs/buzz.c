#include "server.h"

int notes[] = {
	391, 391, 440, 440, 391, 391, 329.63, 329.63, \
	391, 391, 329.63, 329.63, 293.66, 293.66, 293.66, 0, \
	391, 391, 440, 440, 391, 391, 329.63, 329.63, \
	391, 329.63, 293.66, 329.63, 261.63, 261.63, 261.63, 0
};

void buzz_control(void *arg)
{
    arg_t* args = (arg_t*)arg;
    int *flag = (int*)args->arg;
    pthread_mutex_lock(args->mutex);
    softToneCreate(SPKR);
    for (int i = 0; i < TOTAL; ++i) {
        if (*flag)
        {
            pthread_mutex_unlock(args->mutex);
            softToneStop(SPKR);
            break;
        }
        softToneWrite(SPKR, notes[i]);
        delay(240);
    }
    softToneStop(SPKR);
    pthread_mutex_unlock(args->mutex);
}
