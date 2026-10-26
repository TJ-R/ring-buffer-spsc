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
