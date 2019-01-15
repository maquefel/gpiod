#!/bin/bash

SLEEP_PID=

sig_term()
{
    kill ${SLEEP_PID}
    exit #$
}

trap sig_term SIGTERM SIGINT

# we do nothing - simply sleep to check retrigger
coproc sleep $1
SLEEP_PID=${COPROC_PID}
wait ${SLEEP_PID}
