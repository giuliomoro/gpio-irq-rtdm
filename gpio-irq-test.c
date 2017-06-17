#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <error.h>
#include <string.h>

#include <pthread.h>
#include <rtdm/rtdm.h>
#include "gpio-irq.h"

#undef VERBOSE

pthread_t demo_task;
int fd;
int pin = 69;
int timingpin = 0;
int tpretcode;
int gShouldStop = 0;

char *rtdm_driver = "/dev/rtdm/gpio_irq";
#define EVERY 1000


static inline int toggle_timing_pin(void)
{
    if (!timingpin)
	return 0;
#ifdef VERBOSE
    printf("requesting GPIO_IRQ_PIN_TOGGLE %d on the timing pin %d\n", GPIO_IRQ_PIN_TOGGLE, timingpin);
#endif
    tpretcode = ioctl(fd, GPIO_IRQ_PIN_TOGGLE, &timingpin);
    if(tpretcode < 0)
    {
        printf("ioctl returned %d\n", tpretcode);
    }
    return tpretcode;
}

void* demo(void *arg)
{
    int irqs = EVERY;
    struct gpio_irq_data rd = {
	.pin = pin,
	.falling =  0
    };
    int rc;

    pthread_setname_np(pthread_self(), "trivial");
    struct sched_param  param = { .sched_priority = 99 };
    pthread_setschedparam(pthread_self(), SCHED_FIFO, &param);

#ifdef VERBOSE
    printf("requesting GPIO_IRQ_BIND %d on the pin %d\n", GPIO_IRQ_BIND, pin);
#endif
    if ((rc = ioctl(fd,  GPIO_IRQ_BIND, &rd)) < 0) {
	    perror("ioctl GPIO_IRQ_BIND");
	    return (void*)-2;
    }

    while (!gShouldStop) {
#ifdef VERBOSE
        printf("waiting %d for pin %d\n", GPIO_IRQ_PIN_WAIT, pin);
#endif
	if ((rc = ioctl (fd,  GPIO_IRQ_PIN_WAIT, 0)) < 0) {
            printf("ioctl error! rc=%d %s\n", rc, strerror(-rc));
            break;
        }
        if(toggle_timing_pin() < 0)
            return (void*)-1;
        if(toggle_timing_pin() < 0)
            return (void*)-1;
#ifdef VERBOSE
	printf("resuming\n");
	irqs--;
	if (!irqs)  {
	    irqs = EVERY;
	    printf("%d IRQs, tpretcode=%d\n",EVERY, tpretcode);
	}  
#endif
    }
    return (void*)0;
}

void catch_signal(int sig)
{
    fprintf (stderr, "catch_signal sig=%d\n", sig);
    signal(SIGTERM,  SIG_DFL);
    signal(SIGINT, SIG_DFL);
    gShouldStop = 0;
}

int main(int argc, char* argv[])
{
    if (argc > 1)
	pin = atoi(argv[1]);
    if (argc > 2)
	timingpin = atoi(argv[2]);

    signal(SIGTERM, catch_signal);
    signal(SIGINT, catch_signal);

    /* Avoids memory swapping for this program */
    mlockall(MCL_CURRENT|MCL_FUTURE);

    // Open RTDM driver
    if ((fd = open(rtdm_driver, O_RDWR)) < 0) {
	perror("rt_open");
	exit(-1);
    }
    printf("Returned fd: %d\n", fd);

    pthread_create(&demo_task, NULL, demo, NULL);

    void* ret;
    pthread_join(demo_task, &ret);
    int iret = (int)ret;
    printf("Returned: %d\n", iret);

    close(fd);
    return 0;
}
