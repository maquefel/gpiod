#!/usr/bin/env bats

GPIOD=../gpiod
GPIOD_IP=127.0.0.1
GPIOD_PORT=1500
GPIOD_CONFIG=../etc/gpiod.conf.sysfs
NC=nc

setup() {
    /sbin/modprobe gpio-mockup gpio_mockup_ranges=32,64
    for i in /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/*; do echo 0 > $i; done
    $GPIOD -c $(realpath $GPIOD_CONFIG) -l7
    exec 100>/dev/tcp/$GPIOD_IP/$GPIOD_PORT
}

@test "sysfs both interrupt test" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/0
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 0\;1`
    [ "$output" != "" ]

    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/0
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 0\;0`
    [ "$output" != "" ]
}

@test "sysfs rising interrupt test" {
    skip "gpio-mockup currently doesn't handle separate rise/fall and treat them as both"
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/1
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 1\;1`
    [ "$output" != "" ]

    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/1
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 1\;0`
    [ "$output" = "" ]
}

@test "sysfs falling interrupt test" {
    skip "gpio-mockup currently doesn't handle separate rise/fall and treat them as both"
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/2
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 2\;1`
    [ "$output" = "" ]

    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/2
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 2\;0`
    [ "$output" != "" ]
}

@test "sysfs both polled test" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/3
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 3\;1`
    [ "$output" != "" ]

    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/3
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 3\;0`
    [ "$output" != "" ]
}

@test "sysfs active_low test" {
    run cat /sys/class/gpio/gpio36/active_low
    [ "$output" = "1" ]
}

teardown() {
    exec 100<&-
    exec 100>&-
    killall gpiod
    /sbin/rmmod gpio-mockup
}
