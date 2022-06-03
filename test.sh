#!/bin/sh
make -B &&
Xephyr -br -ac -noreset -screen 1900x200 :1 &
sleep 1 &&
DISPLAY=:1 xrdb -merge ~/.Xresources
DISPLAY=:1 ./bin/dwm &
sleep 1 &&
DISPLAY=:1 xterm
