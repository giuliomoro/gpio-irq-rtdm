#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/mman.h>
#include <error.h>

#include <native/task.h>
#include <native/timer.h>
#include <rtdm/rtdm.h>
#include <rtdk.h>
#include "gpio-irq.h"

#define VERBOSE

RT_TASK demo_task;
int fd;
int pin = 69;
int timingpin = 0;
int tpretcode;

char *rtdm_driver = "/dev/rtdm/gpio_irq";
#define EVERY 1000


static inline int toggle_timing_pin(void)
{
    if (!timingpin)
	return 0;
#ifdef VERBOSE
    rt_printf("requesting GPIO_IRQ_PIN_TOGGLE %d\n", GPIO_IRQ_PIN_TOGGLE);
#endif
    tpretcode = ioctl(fd, GPIO_IRQ_PIN_TOGGLE, &timingpin);
    if(tpretcode < 0)
    {
        rt_printf("ioctl returned %d\n", tpretcode);
    }
    return tpretcode;
}

void demo(void *arg)
{
    RTIME previous;
    int irqs = EVERY;
    struct gpio_irq_data rd = {
	.pin = pin,
	.falling =  0
    };
    int rc;

#ifdef VERBOSE
    rt_printf("requesting GPIO_IRQ_BIND %d\n", GPIO_IRQ_BIND);
#endif
    if ((rc = ioctl(fd,  GPIO_IRQ_BIND, &rd)) < 0) {
	    perror("ioctl GPIO_IRQ_BIND");
	    return;
    }

    rt_task_sleep(1);
    previous = rt_timer_read();
    
    while (1) {
        if(toggle_timing_pin() < 0)
            return;
        if(toggle_timing_pin() < 0)
            return;
#ifdef VERBOSE
        rt_printf("waiting %d\n", GPIO_IRQ_PIN_WAIT);
#endif
	if ((rc = ioctl (fd,  GPIO_IRQ_PIN_WAIT, 0)) < 0) {
            rt_printf("ioctl error! rc=%d %s\n", rc, strerror(-rc));
            break;
        }
#ifdef VERBOSE
	rt_printf("resuming\n");
#endif
	irqs--;
	if (!irqs)  {
	    irqs = EVERY;
	    rt_printf("%d IRQs, tpretcode=%d\n",EVERY, tpretcode);
	}  
    }
}

void catch_signal(int sig)
{
    fprintf (stderr, "catch_signal sig=%d\n", sig);
    signal(SIGTERM,  SIG_DFL);
    signal(SIGINT, SIG_DFL);
    rt_task_delete(&demo_task);
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

    // Init rt_printf() system
    rt_print_auto_init(1);
	
    // Open RTDM driver
    if ((fd = open(rtdm_driver, O_RDWR)) < 0) {
	perror("rt_open");
	exit(-1);
    }
    printf("Returned fd: %d\n", fd);

    rt_task_create(&demo_task, "trivial", 0, 99, 0);
    rt_task_start(&demo_task, &demo, NULL);
    pause();

    close(fd);
    return 0;
}
