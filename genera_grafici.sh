#!/bin/bash

gnuplot grafici.gnuplot

for f in *.eps; do epstopdf "$f"; done

rm -rf *.eps