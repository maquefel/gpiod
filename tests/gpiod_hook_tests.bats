#!/usr/bin/env bats

GPIOD=../gpiod
GPIOD_IP=127.0.0.1
GPIOD_PORT=1500
GPIOD_CONFIG=../etc/gpiod.conf.uapi
GPIOD_HOOK=../etc/gpio.d/testtab
GPIOD_ARGS=/tmp/gpiod_test_args
GPIOD_PID=/var/run/gpiod.pid

wait_for_file() {
    cnt=$(($2 - 1))
    while ! test -f "$1"; do
        [ $cnt -eq 0 ] && return 1
        cnt="$(($cnt - 1))"
        sleep 0.1
    done

    return 0
}

setup() {
    run /sbin/modprobe gpio-mockup gpio_mockup_ranges=32,64
    for i in /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/*; do echo 0 > $i; done
    $GPIOD -c $(realpath ${GPIOD_CONFIG}) -d $(realpath $(dirname ${GPIOD_HOOK})) -p ${GPIOD_PID}  -l7
    rm -rf ${GPIOD_ARGS}
    cp test.sh /tmp/test.sh
    cp sleep_test.sh /tmp/sleep_test.sh
}

@test "hook test args count" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/0
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_arg_count=$(sed -n 1p ${GPIOD_HOOK} | sed 's/  */ /g' | cut -s -d' ' -f 3- | wc -w)
    test_arg_count=$(cat /tmp/gpiod_test_args | wc -l)
    echo "tab_arg_count : $tab_arg_count"
    echo "test_arg_count : $test_arg_count"
    [ "$tab_arg_count" == "$test_arg_count" ]
}

@test "hook test passing label arg" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/1
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_label=$(sed -n 2p ${GPIOD_HOOK} | awk '{ print $1 }')
    test_label=$(sed -n 2p $GPIOD_ARGS)
    echo "tab_label : $tab_label"
    echo "test_label : $test_label"
    [ "$tab_label" == "$test_label" ]
}

@test "hook test passing event arg" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/2
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/2 # falling edge config trigger
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_event="FALL"
    test_event=$(sed -n 2p $GPIOD_ARGS)
    echo "tab_event : $tab_event"
    echo "test_event : $test_event"
    [ "$tab_event" == "$test_event" ]
}

@test "hook test passing event numerical arg" {
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/3
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_num_event="1"
    test_num_event=$(sed -n 2p $GPIOD_ARGS)
    echo "tab_event : $tab_event"
    echo "test_event : $test_event"
    [ "$tab_event" == "$test_event" ]
}

@test "hook test rising event" {
    pid=$(<${GPIOD_PID})
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    run ps -o pid= --ppid ${pid}
    [ "$output" == "" ]
}

@test "hook test falling event" {
    pid=$(<${GPIOD_PID})
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/7
    run ps -o pid= --ppid ${pid}
    echo $output
    [ "$output" == "" ]
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/7
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
}

@test "hook test both event" {
    pid=$(<${GPIOD_PID})
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
}

@test "hook test spurios launch prevention" {
    pid=$(<${GPIOD_PID})
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    echo $cnt
    [ "$cnt" == "1" ]
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/4
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    echo $cnt
    [ "$cnt" == "1" ]
    run kill ${child}
}

@test "hook test spawning multiply children" {
    pid=$(<${GPIOD_PID})
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    echo 0 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    echo 1 > /sys/kernel/debug/gpio-mockup-event/gpio-mockup-A/6
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    echo $cnt
    [ "$cnt" == "3" ]
    for c in `ps -o pid= --ppid ${pid}`; do
        run kill ${c}
        [ "$status" -eq 0 ]
    done
}

teardown() {
    killall gpiod
    run /sbin/rmmod gpio-mockup
    rm /tmp/test.sh
    rm /tmp/sleep_test.sh
}
