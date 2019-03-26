#!/bin/bash

for N in 1000 5000 10000 50000 100000
do
    for i in {1..10}
    do
        aprun ./a.out $N >> res_$N.txt
    done
done

