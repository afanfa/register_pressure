# Register Pressure in AMD GPUs #

The following blog post is focused on a practical demo showing how to apply the recommendations explained in the following [talk](https://vimeo.com/742349001) presented as an
OLCF training talk on August 23rd 2022. Here is a [link](https://docs.olcf.ornl.gov/training/training_archive.html) to the training archive where you can also find the slides
presented during the talk.

## Registers and Occupancy ##

General purpose registers are the fastest type of memory available in modern processors. In most cases, the ALUs (Arithmetic Logic Units) in traditional processor and accelerators can only directly access registers.
Unfortunately, registers are a scarse and expensive resource and compilers try their best to *optimize* the way logical variables are assigned to hardware registers to be manipulated by the ALU.

When we use the word *optimize* we should always clarify the objective of the optimization process. In fact, regular CPUs and accelerators (like GPUs), because of their very nature, have different waya of executing programs and achieving high performance.
Traditional CPUs are latency-oriented machines, designed to execute as many instructions as possible belonging to a single serial thread. On the other hand, GPUs are throughput-oriented machines, designed to take advantage
of parallelism between independent threads as much as possible.

In AMD GPUs, a high number of concurrent threads running on the same CU (Compute Unit) allows the GPU to hide the time spent in accessing global memory (always higher than the time needed to perform a compute operation) with other operations performed by other threads.

The term *occupancy* represents the maximum number of wavefronts that can run on the same CU potentialy at the same time.
By design, the AMD CDNA2 architecture (MI200 series cards), the maximum number of wavefronts able to run on a single CU is **eight**.
