#!/bin/bash

sleep 10
#/usr/bin/java -jar /usr/bin/derby-timer.jar -d SmartLineDevice -x -logdir /var/log -n /dev/ttyUSB0 http://web/derbynet
/usr/bin/java -jar /usr/bin/derby-timer.jar "$@"
