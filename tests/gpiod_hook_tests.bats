#!/usr/bin/env bats

setup() {
    load 'common_setup'
    _common_setup
}

@test "hook test args count" {
    echo 1 > ${MOCKUP_PATH}/0
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_arg_count=$(sed -n 1p ${GPIOD_HOOK} | sed 's/  */ /g' | cut -s -d' ' -f 3- | wc -w)
    test_arg_count=$(cat /tmp/gpiod_test_args | wc -l)
    echo "tab_arg_count : $tab_arg_count"
    echo "test_arg_count : $test_arg_count"
    [ "$tab_arg_count" == "$test_arg_count" ]
}

@test "hook test passing label arg" {
    echo 1 > ${MOCKUP_PATH}/1
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_label=$(sed -n 2p ${GPIOD_HOOK} | awk '{ print $1 }')
    test_label=$(sed -n 2p $GPIOD_ARGS)
    echo "tab_label : $tab_label"
    echo "test_label : $test_label"
    [ "$tab_label" == "$test_label" ]
}

@test "hook test passing event arg" {
    echo 1 > ${MOCKUP_PATH}/2
    echo 0 > ${MOCKUP_PATH}/2 # falling edge config trigger
    wait_for_file ${GPIOD_ARGS} 3
    [ $? -eq 0 ]
    tab_event="FALL"
    test_event=$(sed -n 2p $GPIOD_ARGS)
    echo "tab_event : $tab_event"
    echo "test_event : $test_event"
    [ "$tab_event" == "$test_event" ]
}

@test "hook test passing event numerical arg" {
    echo 1 > ${MOCKUP_PATH}/3
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
    echo 1 > ${MOCKUP_PATH}/6
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
    echo 0 > ${MOCKUP_PATH}/6
    run ps -o pid= --ppid ${pid}
    [ "$output" == "" ]
}

@test "hook test falling event" {
    pid=$(<${GPIOD_PID})
    echo 1 > ${MOCKUP_PATH}/7
    run ps -o pid= --ppid ${pid}
    echo $output
    [ "$output" == "" ]
    echo 0 > ${MOCKUP_PATH}/7
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
}

@test "hook test both event" {
    pid=$(<${GPIOD_PID})
    echo 1 > ${MOCKUP_PATH}/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
    echo 0 > ${MOCKUP_PATH}/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    [ "$cnt" != "0" ]
    run kill ${child}
}

@test "hook test spurios launch prevention" {
    pid=$(<${GPIOD_PID})
    echo 1 > ${MOCKUP_PATH}/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    echo $cnt
    [ "$cnt" == "1" ]
    echo 0 > ${MOCKUP_PATH}/4
    echo 1 > ${MOCKUP_PATH}/4
    child=$(ps -o pid= --ppid ${pid})
    cnt=$(echo ${child} | wc -w)
    echo $cnt
    [ "$cnt" == "1" ]
    run kill ${child}
}

@test "hook test spawning multiply children" {
    pid=$(<${GPIOD_PID})
    echo 0 > ${MOCKUP_PATH}/6
    echo 1 > ${MOCKUP_PATH}/6
    echo 0 > ${MOCKUP_PATH}/6
    echo 1 > ${MOCKUP_PATH}/6
    echo 0 > ${MOCKUP_PATH}/6
    echo 1 > ${MOCKUP_PATH}/6
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
    load 'common_setup'
    _common_teardown
}
