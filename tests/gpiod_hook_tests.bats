#!/usr/bin/env bats

GPIOD=../gpiod
GPIOD_IP=127.0.0.1
GPIOD_PORT=1500
GPIOD_CONFIG=../etc/gpiod.conf.uapi
GPIOD_HOOKS=../etc/gphooks.d
GPIOD_ARGS=/tmp/gpiod_test_args
NC=nc

setup() {
    /sbin/modprobe gpio-mockup gpio_mockup_ranges=32,64
    for i in /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/*; do echo 0 > $i; done
    $GPIOD -c $(realpath $GPIOD_CONFIG) -d $(realpath $GPIOD_HOOKS) -l7
    cp test.sh /tmp/test.sh
}

@test "hook test args count" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/0
    tab_arg_count=$(sed -n 1p ../etc/gphooks.d/testtab | sed 's/  */ /g' | cut -s -d' ' -f 3- | wc -w)
    test_arg_count=$(cat /tmp/gpiod_test_args | wc -l)
    [ "$tab_arg_count" == "$test_arg_count" ]
}

@test "hook test passing label arg" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/1
    tab_label=$(sed -n 2p ../etc/gphooks.d/testtab | awk '{ print $1 }')
    test_label=$(sed -n 2p $GPIOD_ARGS)
    [ "$tab_label" == "$test_label" ]
}

@test "hook test passing event arg" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/2
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/2 # falling edge config trigger
    tab_event="TEST"
    test_event=$(sed -n 2p $GPIOD_ARGS)
    [ "$tab_event" == "$test_event" ]
}

@test "hook test passing event numerical arg" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/3
    tab_num_event="111"
    test_num_event=$(sed -n 2p $GPIOD_ARGS)
    [ "$tab_event" == "$test_event" ]
}

@test "hook test rising event" {

}

@test "hook test falling event" {

}

@test "hook test both event" {

}

@test "hook test spurios launch prevention" {

}

teardown() {
    killall gpiod
    /sbin/rmmod gpio-mockup
}
