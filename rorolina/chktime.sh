#!/bin/bash
TIME=$(expr `date +%s%N` / 1000000)
./$1 &> /dev/null
echo $(($(expr `date +%s%N` / 1000000) - $TIME))
