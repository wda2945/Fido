/*
 * pwm.h
 *
 *      Author: martin
 */

#ifndef PWM_H
#define PWM_H

#define KEYLEN 7

typedef struct pwm_exp
{
    char key[KEYLEN+1]; /* leave room for terminating NUL byte */
    int period_fd;
    int duty_fd;
    int polarity_fd;
    int run_fd;
    unsigned long duty_ns;
    unsigned long period_ns;
} PwmChannel_t;

int pwm_start(const char *pinName, int exportNumber, char *pwmDir, float duty, float freq, int polarity, PwmChannel_t *chan);

int pwm_set_duty_cycle(PwmChannel_t *chan, float duty);
int pwm_set_frequency(PwmChannel_t *chan, float freq);
int pwm_set_polarity(PwmChannel_t *chan, int polarity);
int pwm_set_run(PwmChannel_t *pwm, int runState) ;

int pwm_disable(PwmChannel_t *pwm);

#endif
