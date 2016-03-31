/*
 * pwm.c
 *
 *      Author: martin
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "pwm.h"
#include "common.h"
#include "syslog/syslog.h"

#define PATHLEN 50

int pwm_set_run(PwmChannel_t *pwm, int runState) {
    int len;
    char buffer[7]; /* allow room for trailing NUL byte */

    len = snprintf(buffer, sizeof(buffer), "%d", runState);
    write(pwm->run_fd, buffer, len);

    return 0;
}

int pwm_set_frequency(PwmChannel_t *pwm, float freq) {
    int len;
    char buffer[20];
    unsigned long period_ns;

    if (freq <= 0.0)
        return -1;

    period_ns = (unsigned long)(1e9 / freq);

    if (period_ns != pwm->period_ns) {
        pwm->period_ns = period_ns;

        len = snprintf(buffer, sizeof(buffer), "%lu", period_ns);
        write(pwm->period_fd, buffer, len);
    }

    return 0;
}

int pwm_set_polarity(PwmChannel_t *pwm, int polarity) {
    int len;
    char buffer[7]; /* allow room for trailing NUL byte */

    len = snprintf(buffer, sizeof(buffer), "%d", polarity);
    write(pwm->polarity_fd, buffer, len);

    return 0;
}

int pwm_set_duty_cycle(PwmChannel_t *pwm, float duty) {
    int len;
    char buffer[20];

    pwm->duty_ns = (unsigned long)(pwm->period_ns * duty);

    len = snprintf(buffer, sizeof(buffer), "%lu", pwm->duty_ns);
    write(pwm->duty_fd, buffer, len);

    return 0;
}

int pwm_start(const char *pinName, int exportNumber, char *pwmDir, float duty, float freq, int polarity, PwmChannel_t *pwm)
{
    char pwm_control_path[PATHLEN];


    char pwm_path[PATHLEN];				//pwm channel control folder
    char export[2];
    int fd;

    strncpy(pwm->key, pinName, KEYLEN);

    if (set_pinmux(pinName, "pwm") < 0)
    {
    	return -1;
    }
    //export pwm channel
    if ((fd = open("/sys/class/pwm/export", O_WRONLY)) < 0)
    {
    	LogError("pwm open(/sys/class/pwm/export) fail (%s)\n", strerror(errno));
        return -1;
    }
    snprintf(export, 2, "%i", exportNumber);
    write(fd,export,1);
    close(fd);

    snprintf(pwm_path, sizeof(pwm_path), "/sys/class/pwm/%s", pwmDir);

    //create the path for period control
    snprintf(pwm_control_path, sizeof(pwm_control_path), "%s/period_ns", pwm_path);
    if ((pwm->period_fd = open(pwm_control_path, O_WRONLY)) < 0)
    {
    	LogError("pwm open(%s) fail (%s)\n", pwm_control_path, strerror(errno));
        return -1;
    }

    snprintf(pwm_control_path, sizeof(pwm_control_path), "%s/duty_ns", pwm_path);
    if ((pwm->duty_fd = open(pwm_control_path, O_WRONLY)) < 0) {
        //error, close already opened period_fd.
    	LogError("pwm open(%s) fail (%s)\n", pwm_control_path, strerror(errno));
        close(pwm->period_fd);
        return -1;
    }

    snprintf(pwm_control_path, sizeof(pwm_control_path), "%s/polarity", pwm_path);
    if ((pwm->polarity_fd = open(pwm_control_path, O_WRONLY)) < 0) {
    	LogError("pwm open(%s) fail (%s)\n", pwm_control_path, strerror(errno));
        //error, close already opened period_fd and duty_fd.
        close(pwm->period_fd);
        close(pwm->duty_fd);
        return -1;
    }    

    snprintf(pwm_control_path, sizeof(pwm_control_path), "%s/run", pwm_path);
    if ((pwm->run_fd = open(pwm_control_path, O_WRONLY)) < 0) {
    	LogError("pwm open(%s) fail (%s)\n", pwm_control_path, strerror(errno));
        //error, close already opened period_fd and duty_fd.
    	close(pwm->polarity_fd);
        close(pwm->period_fd);
        close(pwm->duty_fd);
        return -1;
    }

    pwm_set_frequency(pwm, freq);
    pwm_set_polarity(pwm, polarity);
    pwm_set_duty_cycle(pwm, duty);
    pwm_set_run(pwm, 1);

    return 0;
}

int pwm_disable(PwmChannel_t *pwm)
{
	pwm_set_run(pwm, 0);
	//close the fds
	close(pwm->period_fd);
	close(pwm->duty_fd);
	close(pwm->polarity_fd);
	close(pwm->run_fd);
    return 0;    
}
