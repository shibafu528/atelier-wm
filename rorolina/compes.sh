#!/bin/sh
gcc -g -O0 -std=c99 -o es.o entrysearch.c `pkg-config --libs --cflags gtk+-2.0`

