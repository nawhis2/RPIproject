#include "server.h"

int PR_control(void* arg)
{
	const int threshold = 180;
	int fd;
	int a2dChannel = 0;     // analog channel AIN0, CDS sensors
	
	arg_t args = *(arg_t *)arg;
	int led_light = *(int*)args.arg;
	if((fd = wiringPiI2CSetupInterface("/dev/i2c-1",0x48))<0) {
		printf("wiringPiI2CSetupInterface failed:\n");
	}

	wiringPiI2CWrite(fd, 0x00 | a2dChannel);       // 0000_0000 

	int measured = wiringPiI2CRead(fd);
	pthread_mutex_lock(args.mutex);
	if (measured < threshold)
		softPwmWrite(GPIO17, 0);
	else 
		softPwmWrite(GPIO17, led_light);
	pthread_mutex_unlock(args.mutex);
	return measured;
}
