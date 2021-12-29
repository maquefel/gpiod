#!/usr/bin/env bats

GPIOD=../gpiod
GPIOD_CONFIG=../etc/gpiod.conf.sysfs
GPIOD_IP=127.0.0.1
GPIOD_PORT=1500
GPIOD_HOOK=../etc/gpio.d/testtab
GPIOD_ARGS=/tmp/gpiod_test_args
GPIOD_PID=/var/run/gpiod.pid

setup() {
    /sbin/modprobe gpio-mockup gpio_mockup_ranges=32,40
    gpiochip_name=$(cat /sys/kernel/debug/gpio | grep gpio-mockup-A | awk -F: '{ print $1 }')
    export gpiochip_name
    export MOCKUP_PATH=/sys/kernel/debug/gpio-mockup/${gpiochip_name}
    for i in ${MOCKUP_PATH}/*; do echo 0 > $i; done
    coproc { ${GPIOD} -n -c $(realpath ${GPIOD_CONFIG}) -d $(realpath $(dirname ${GPIOD_HOOK})) -p ${GPIOD_PID}; } 
    export SAVED_PID=$gpiod_PID
    sleep 0.5
    exec 100>/dev/tcp/$GPIOD_IP/$GPIOD_PORT
}

@test "sysfs both interrupt test" {
    echo 1 > ${MOCKUP_PATH}/0
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 0\;1`
    [ "$output" != "" ]

    echo 0 > ${MOCKUP_PATH}/0
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 0\;0`
    [ "$output" != "" ]
}

@test "sysfs rising interrupt test" {
    skip "gpio-mockup currently doesn't handle separate rise/fall and treat them as both"
    echo 1 > ${MOCKUP_PATH}/1
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 1\;1`
    [ "$output" != "" ]

    echo 0 > ${MOCKUP_PATH}/1
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 1\;0`
    [ "$output" = "" ]
}

@test "sysfs falling interrupt test" {
    skip "gpio-mockup currently doesn't handle separate rise/fall and treat them as both"
    echo 1 > ${MOCKUP_PATH}/2
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 2\;1`
    [ "$output" = "" ]

    echo 0 > ${MOCKUP_PATH}/2
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 2\;0`
    [ "$output" != "" ]
}

@test "sysfs both polled test" {
    echo 1 > ${MOCKUP_PATH}/3
    result=$(timeout 1 cat <&100) || true
    run `echo $result | grep 3\;1`
    [ "$output" != "" ]

    echo 0 > ${MOCKUP_PATH}/3
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
    ${GPIOD} -p ${GPIOD_PID} -k
    wait $SAVED_PID || true
    /sbin/rmmod gpio-mockup
}
