#include "server.h"

int LED_control(void *arg)
{
    int led_light = atoi((char *)arg);

    softPwmWrite(GPIO17, led_light);
    return led_light;
}