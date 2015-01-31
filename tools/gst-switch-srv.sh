#!/bin/bash

#gdb --args \
./tools/gst-switch-srv -v \
    --gst-debug-no-color \
    --gst-debug="switch:5" \
    --gst-debug="convbin:0" \
    --gst-debug="tcpmixsrc:0" \
    --gst-debug="multiqueue:0" \
    --video-input-port="3000" \
    --audio-input-port="4000" \
    --controller-address="tcp:host=0.0.0.0,port=5000" \
    --record="record.dat"
