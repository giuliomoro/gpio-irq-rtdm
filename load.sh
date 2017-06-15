#!/bin/bash
set -x

config-pin overlay cape-universal

# P8.9 = 2/5 = GPIO 69 = IRQ trigger
config-pin P8.9 hi_pd
config-pin P8.9 in

# P8.10 = 2/4 = GPIO 68 = driver timing pin
config-pin P8.10 out

# P8.11 = 1/13 = GPIO 45 = userland timing pin
config-pin P8.11 out

./reload-module.sh

echo starting userland
./gpio-irq-test 69 45





