#!/bin/sh
gcc -g -O0 -std=c99 -o es.o t_entrysearch.c entrysearch.c `pkg-config --libs --cflags gtk+-2.0`

