# Analysis of Linux Schedulers
##### Student: Carl Cortright

## Goal

Analyze the run-time performance of three linux process schedulers.

## Instructions for Building

###### Make Project
` make `

###### Clean Project
` make clean`

## Instructions for running

After making the project, just run one of the following commands:

`./pi-sched <children> <iterations> <policy> <vary priority>`

`./rw-sched <number of processes> <policy> <vary priority> <transfer size> <block size>`


`./mixed-sched <number of processes> <policy> <vary priority> <transfer size> <block size> <iterations>`

Or, to run the entire experiment, run:

`sudo ./experiment.sh`
