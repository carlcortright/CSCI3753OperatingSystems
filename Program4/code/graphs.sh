# Clean any old output/input files, and make
make clean
make

# Run the experiment
# CPU intensive task
./pi-sched 5 10000000 SCHED_OTHER true
./pi-sched 10 10000000 SCHED_OTHER true
./pi-sched 25 10000000 SCHED_OTHER true
./pi-sched 40 10000000 SCHED_OTHER true
./pi-sched 50 10000000 SCHED_OTHER true
./pi-sched 100 10000000 SCHED_OTHER true
./pi-sched 150 10000000 SCHED_OTHER true
./pi-sched 200 10000000 SCHED_OTHER true
./pi-sched 250 10000000 SCHED_OTHER true
./pi-sched 300 10000000 SCHED_OTHER true
./pi-sched 350 10000000 SCHED_OTHER true
./pi-sched 400 10000000 SCHED_OTHER true
./pi-sched 450 10000000 SCHED_OTHER true
./pi-sched 500 10000000 SCHED_OTHER true


# IO intensive task
./rw-sched 5 SCHED_OTHER false 1024 8
./rw-sched 50 SCHED_OTHER false 1024 8
./rw-sched 500 SCHED_OTHER false 1024 8
./rw-sched 5 SCHED_FIFO false 1024 8
./rw-sched 50 SCHED_FIFO false 1024 8
./rw-sched 500 SCHED_FIFO false 1024 8
./rw-sched 5 SCHED_RR false 1024 8
./rw-sched 50 SCHED_RR false 1024 8
./rw-sched 500 SCHED_RR false 1024 8
./rw-sched 5 SCHED_OTHER true 1024 8
./rw-sched 50 SCHED_OTHER true 1024 8
./rw-sched 500 SCHED_OTHER true 1024 8
./rw-sched 5 SCHED_FIFO true 1024 8
./rw-sched 50 SCHED_FIFO true 1024 8
./rw-sched 500 SCHED_FIFO true 1024 8
./rw-sched 5 SCHED_RR true 1024 8
./rw-sched 50 SCHED_RR true 1024 8
./rw-sched 500 SCHED_RR true 1024 8

# Mixed IO and CPU intensive task
./mixed-sched 5 SCHED_OTHER false 1024 8 10000000
./mixed-sched 50 SCHED_OTHER false 1024 8 10000000
./mixed-sched 500 SCHED_OTHER false 1024 8 10000000
./mixed-sched 5 SCHED_FIFO false 1024 8 10000000
./mixed-sched 50 SCHED_FIFO false 1024 8 10000000
./mixed-sched 500 SCHED_FIFO false 1024 8 10000000
./mixed-sched 5 SCHED_RR false 1024 8 10000000
./mixed-sched 50 SCHED_RR false 1024 8 10000000
./mixed-sched 500 SCHED_RR false 1024 8 10000000
./mixed-sched 5 SCHED_OTHER true 1024 8 10000000
./mixed-sched 50 SCHED_OTHER true 1024 8 10000000
./mixed-sched 500 SCHED_OTHER true 1024 8 10000000
./mixed-sched 5 SCHED_FIFO true 1024 8 10000000
./mixed-sched 50 SCHED_FIFO true 1024 8 10000000
./mixed-sched 500 SCHED_FIFO true 1024 8 10000000
./mixed-sched 5 SCHED_RR true 1024 8 10000000
./mixed-sched 50 SCHED_RR true 1024 8 10000000
./mixed-sched 500 SCHED_RR true 1024 8 10000000
# Cleanup
make clean
