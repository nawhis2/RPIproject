#include "server.h"

int notes[] = { 	/* 학교종을 연주하기 위한 계이름 */
	391, 391, 440, 440, 391, 391, 329.63, 329.63, \
	391, 391, 329.63, 329.63, 293.66, 293.66, 293.66, 0, \
	391, 391, 440, 440, 391, 391, 329.63, 329.63, \
	391, 329.63, 293.66, 329.63, 261.63, 261.63, 261.63, 0
};

void buzz_control(void *arg)
{
    arg_t* args = (arg_t*)arg;

    pthread_mutex_lock(args->mutex);
    softToneCreate(SPKR); 	/* 톤 출력을 위한 GPIO 설정 */
    for (int i = 0; i < TOTAL; ++i) {
        softToneWrite(SPKR, notes[i]); /* 톤 출력 : 학교종 연주 */
        delay(280); 		/* 음의 전체 길이만큼 출력되도록 대기 */
    }
    softToneStop(SPKR);
    pthread_mutex_unlock(args->mutex);
}
