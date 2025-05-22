#include "server.h"

static int gpiopins[4] = {23, 18, 15, 14};
static int number[10][4] = {
        {0, 0, 0, 0},
        {0, 0, 0, 1},
        {0, 0, 1, 0},
        {0, 0, 1, 1},
        {0, 1, 0, 0},
        {0, 1, 0, 1},
        {0, 1, 1, 0},
        {0, 1, 1, 1},
        {1, 0, 0, 0},
        {1, 0, 0, 1},
    };
 
static void turnOnNum(int num)
{
    for (int i = 0; i < 4; i++)
        digitalWrite(gpiopins[i], number[num][i] ? HIGH : LOW);
}

static void turnOffNum()
{
    for (int i = 0; i < 4; i++)
        digitalWrite(gpiopins[i], HIGH);
}

void SEG_control(void* arg)
{
    arg_t* args = (arg_t *)arg;
    int no = atoi((char*)args->arg);

    for (int i = 0; i < 4; i++)
        pinMode(gpiopins[i], OUTPUT);

    int notes[6] = {400, 0, 400, 0, 400, 0};
    for (; no > 0; no--)
    {
        turnOnNum(no);
        sleep(1);
        turnOffNum();
    }

    turnOnNum(0);
    pthread_mutex_lock(args->mutex);
    softToneCreate(SPKR);
    for (int i = 0; i < 6; i++)
    {
        printf("notes[%d] : %d\n", i, notes[i]);
        softToneWrite(SPKR, notes[i]);
        delay(280);
    }
    turnOffNum();
    softToneStop(SPKR);
    pthread_mutex_unlock(args->mutex);
}
