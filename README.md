# Name

Lock Free Ring Buffer with a single producer and single consumer

## Description

The goal of this project is to better understand atomics, cache lines,\
false sharing, and memory ordering.

Have not worked with these topic before.

My initial goal is to try and create a generic lock free ring buffer\
that can eventually be used in other codebases as needed.

Also am trying to get a better understanding of performance / memory profiling.

So below there will probably so good / useless metrics as I get a better grasp\
at which metrics are valuable for this kind of project.

## Performance Metrics

### Consecutive Iterations \ (time diff between subsequent runs of the same work back to back)

Work: Adding 100,000,000 values to the queue via the producer and consumer\
should read all of these out in order then push to vector

#### Time to run as of 10/25/26

##### With vector::reserve

Starting threads
Duration of work: 2280.02
Starting threads
Duration of work: 2254.86
Starting threads
Duration of work: 2169.36
Starting threads
Duration of work: 2530.55
Starting threads
Duration of work: 2278.92

##### Without vector::reserve

Starting threads
Duration of work: 2505.49
Starting threads
Duration of work: 2525.29
Starting threads
Duration of work: 2401.74
Starting threads
Duration of work: 2352.39
Starting threads
Duration of work: 2433.15

##### Without vector work just checking against numbers in order

Starting threads
Duration of work: 2059.33
Consumer Result is good.
Starting threads
Duration of work: 2518.21
Consumer Result is good.
Starting threads
Duration of work: 1907.17
Consumer Result is good.
Starting threads
Duration of work: 2032.48
Consumer Result is good.
Starting threads
Duration of work: 2005.63
Consumer Result is good.

##### Moved buf on own cache line (not as much imporvement as I hoped)
Head address as int 140729162929664
Tail address as int 140729162929728
First Int address as int 140729162929792
Starting threads
Duration of work: 2039.22
Consumer Result is good.
Starting threads
Duration of work: 2035.78
Consumer Result is good.
Starting threads
Duration of work: 2050.67
Consumer Result is good.
Starting threads
Duration of work: 2498.03
Consumer Result is good.
Starting threads
Duration of work: 2429.2
Consumer Result is good.
Starting threads
Duration of work: 2419.03
Consumer Result is good.
Starting threads
Duration of work: 2119.53
Consumer Result is good.
Starting threads
Duration of work: 2115.8
Consumer Result is good.
Starting threads
Duration of work: 2416.09
Consumer Result is good.
Starting threads
Duration of work: 2103.8
Consumer Result is good.

