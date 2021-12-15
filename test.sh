#!/bin/sh
make -B &&
Xephyr -br -ac -noreset -screen 400x200 :1 &
sleep 1 &&
DISPLAY=:1 ./bin/dwm &
sleep 1 &&
DISPLAY=:1 xterm
