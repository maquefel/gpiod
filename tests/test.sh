#!/bin/bash
printf "%s\n" "$0" > /tmp/gpiod_test_args
for arg in "$@"
do
    printf "%s\n" "$arg" >> /tmp/gpiod_test_args
done
