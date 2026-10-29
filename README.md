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

My system is x86 which from my research fixes memory ordering at lot of times\
making less room for error then an ARM based system. Does make it a little difficult\
to determine performance imporvements

## Performance Metrics

### Perf

#### Perf Stat

perf stat -e cycles,instructions,ls_dmnd_fills_from_sys.int_cache,ls_dmnd_fills_from_sys.ext_cache_local -r 10 ./time_test

##### Previous Impl

Durations were about the same as below in the Consecutive Iters.\
About ~2600ms\

25,896,466,685      cycles                                                                  ( +-  2.87% ) \
12,947,334,486      instructions                                                            ( +-  3.88% ) \
   118,076,551      ls_dmnd_fills_from_sys.int_cache                                        ( +-  2.50% ) \
             0      ls_dmnd_fills_from_sys.ext_cache_local\
\
       2.873395750 +- 0.080439412 seconds time elapsed  ( +-  2.80% )

##### Update Impl

Duration of work: 914.7
Duration of work: 914.876
Duration of work: 948.761
Duration of work: 869.304
Duration of work: 950.598
Duration of work: 989.531
Duration of work: 867.145
Duration of work: 988.96
Duration of work: 957.153
Duration of work: 940.599

 Performance counter stats for './time_test' (10 runs):

     8,230,386,473      cycles                                                                  ( +-  1.44% )
     3,370,647,928      instructions                                                            ( +-  1.33% )
        64,891,543      ls_dmnd_fills_from_sys.int_cache                                        ( +-  1.25% )
                 0      ls_dmnd_fills_from_sys.ext_cache_local

       0.935136724 +- 0.013580706 seconds time elapsed  ( +-  1.45% )

#### Perf C2C

perf c2c record -- ./time_test

I have a 5800x3D so unfortunately perf c2c does not work well with zen 3.

### Thread Sanitizer

g++ -fsanitize=thread -g -O1 -o tsan_test main.cpp \
currently no issues with threads according to this

### Consecutive Iterations \ (time diff between subsequent runs of the same work back to back)

g++ main.cpp -std=c++20 -O2 -Wall -Wextra -Wconversion -o time_test
Work: Adding 100,000,000 values to the queue via the producer and consumer\
should read all of these out in order then push to vector

#### Time to run as of 10/25/26

##### With vector::reserve

Starting threads

Duration of work: 2280.02\
Starting threads\
Duration of work: 2254.86\
Starting threads\
Duration of work: 2169.36\
Starting threads\
Duration of work: 2530.55\
Starting threads\
Duration of work: 2278.92\

##### Without vector::reserve

Starting threads\
Duration of work: 2505.49\
Starting threads\
Duration of work: 2525.29\
Starting threads\
Duration of work: 2401.74\
Starting threads\
Duration of work: 2352.39\
Starting threads\
Duration of work: 2433.15\

##### Without vector work just checking against numbers in order

Starting threads\
Duration of work: 2059.33\
Consumer Result is good.\
Starting threads\
Duration of work: 2518.21\
Consumer Result is good.\
Starting threads\
Duration of work: 1907.17\
Consumer Result is good.\
Starting threads\
Duration of work: 2032.48\
Consumer Result is good.\
Starting threads\
Duration of work: 2005.63\
Consumer Result is good.\

##### Moved buf on own cache line (not as much imporvement as I hoped)

Head address as int 140729162929664\
Tail address as int 140729162929728\
First Int address as int 140729162929792\
Starting threads\
Duration of work: 2039.22\
Consumer Result is good.\
Starting threads\
Duration of work: 2035.78\
Consumer Result is good.\
Starting threads\
Duration of work: 2050.67\
Consumer Result is good.\
Starting threads\
Duration of work: 2498.03\
Consumer Result is good.\
Starting threads\
Duration of work: 2429.2\
Consumer Result is good.\
Starting threads\
Duration of work: 2419.03\
Consumer Result is good.\
Starting threads\
Duration of work: 2119.53\
Consumer Result is good.\
Starting threads\
Duration of work: 2115.8\
Consumer Result is good.\
Starting threads\
Duration of work: 2416.09\
Consumer Result is good.\
Starting threads\
Duration of work: 2103.8\
Consumer Result is good.\

##### Moved capacity on own cache line

Best set of runs yet but I reran another 3 sets and times were about same
as the runs above so I can't verifiably say any improvement

Head address as int 140732126601920\
Tail address as int 140732126601984\
First Int address as int 140732126602048\
Starting threads\
Duration of work: 2337.65\
Consumer Result is good.\
Starting threads\
Duration of work: 1551.28\
Consumer Result is good.\
Starting threads\
Duration of work: 2404.43\
Consumer Result is good.\
Starting threads\
Duration of work: 1575.51\
Consumer Result is good.\
Starting threads\
Duration of work: 2306.65\
Consumer Result is good.\
Starting threads\
Duration of work: 1754.3\
Consumer Result is good.\
Starting threads\
Duration of work: 1644.58\
Consumer Result is good.\
Starting threads\
Duration of work: 2293.91\
Consumer Result is good.\
Starting threads\
Duration of work: 2379.17\
Consumer Result is good.\
Starting threads\
Duration of work: 2295.1\
Consumer Result is good.\

##### Removed the checker, cycle code, and lot of cout

Mins are lowest I've seen going to focus on generalizing first before more \
profiling. \

Duration of work: 2252.86 \
Duration of work: 1384.26 \
Duration of work: 1476.62 \
Duration of work: 1422.04 \
Duration of work: 2080.61 \
Duration of work: 2249.45 \
Duration of work: 2338.72 \
Duration of work: 1257.23 \
Duration of work: 2429 \
Duration of work: 1439

##### Back after a large refactor. Testing with different capacities

###### 4 (2^2)

Average time: 3510.407 for 10 runs

###### 64 (2^6)

Average time: 953.0891 across 10 runs

###### 1024 (2^10)

Average time: 922.1099 for 10 runs

###### 32768 (2^15)

Average time: 1025.4856 for 10 runs

###### 2^32 (16GB for ints)

Average time: 958.9644
min: 906.878
