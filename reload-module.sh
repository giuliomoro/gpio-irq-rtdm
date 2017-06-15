#!/bin/bash
rmmod gpio_irq_rtdm
insmod ./gpio_irq_rtdm.ko timing_pin=68

