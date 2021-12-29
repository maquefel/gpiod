#!/usr/bin/env bats

setup() {
    load 'common_setup'
    _common_setup
    sleep 0.5
    exec 100>/dev/tcp/$GPIOD_IP/$GPIOD_PORT
}

@test "uapi both interrupt test" {
    echo 1 > ${MOCKUP_PATH}/0
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 0\;1`
    [ "$output" != "" ]

    echo 0 > ${MOCKUP_PATH}/0
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 0\;0`
    [ "$output" != "" ]
}

@test "uapi rising interrupt test" {
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

    echo 1 > ${MOCKUP_PATH}/1
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 1\;1`
    [ "$output" != "" ]
}

@test "uapi falling interrupt test" {
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

@test "uapi both polled test" {
    echo 1 > ${MOCKUP_PATH}/3
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 3\;1`
    [ "$output" != "" ]

    echo 0 > ${MOCKUP_PATH}/3
    result=$(timeout 1 cat <&100) || true
    echo $result
    run `echo $result | grep 3\;0`
    [ "$output" != "" ]
}

teardown() {
    exec 100<&-
    exec 100>&-
    load 'common_setup'
    _common_teardown
}
