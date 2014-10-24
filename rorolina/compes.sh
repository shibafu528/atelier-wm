#!/bin/sh
gcc -g -O0 -o es.o entrysearch.c `pkg-config --libs --cflags gtk+-2.0`

