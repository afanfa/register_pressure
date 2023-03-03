# Register Pressure in AMD GPUs #

The following blog post is focused on a practical demo showing how to apply the recommendations explained in the following [talk](https://vimeo.com/742349001) presented as an
OLCF training talk on August 23rd 2022. Here is a [link](https://docs.olcf.ornl.gov/training/training_archive.html) to the training archive where you can also find the slides
presented during the talk.

In this blog post, we will focus on the AMD CDNA2 architecture (MI200 series cards) and ROCm-5.4.

## Registers and Occupancy ##

General purpose registers are the fastest type of memory available in modern processors. In most cases, the ALUs (Arithmetic Logic Units) in traditional processor and accelerators can only directly access registers.
Unfortunately, registers are a scarse and expensive resource and compilers try their best to *optimize* the way logical variables are assigned to hardware registers to be manipulated by the ALU.

When we use the word *optimize* we should always clarify the objective of the optimization process. In fact, regular CPUs and accelerators (like GPUs), because of their very nature, have different ways of executing programs and achieving high performance.
Traditional CPUs are latency-oriented machines, designed to execute as many instructions as possible belonging to a single serial thread. On the other hand, GPUs are throughput-oriented machines, designed to take advantage
of parallelism between independent threads as much as possible.

In AMD GPUs, a high number of concurrent wavefronts running on the same Compute Unit (CU) allows the GPU to hide the time spent in accessing global memory (higher than the time needed to perform a compute operation) with other operations performed by other threads.

The term *occupancy* represents the maximum number of wavefronts that can run on the same CU potentialy at the same time. In general, having higher occupancy provides better performance by hiding costly memory accesses with other operations, but this is not always the case.

Ideally, we would like to have as much occupancy as possible, all the time. In reality, occupancy is limited by hardware design choices and resource limitations dictated by the shader code / kernel (HIP, OpenCL, etc.) running on the card.
For example, each CU of the AMD CDNA2 cards has four sets of wavefront slots, each of which can manage at the most **eight** wavefronts. This means that the physical limit to occupancy in CDNA2 is 32 wavefronts per CU.

The number of registers needed by a kernel is one of the most relevant factors deciding occupancy (another factor is the amount of LDS memory used).
The following table summarizes the maximum level of occupancy achievable on CDNA2 cards as a function of the number of VGPRs used by a kernel.

[CDNA2 Occupancy VGPR](img/occupancy_vgpr.JPG)

## Register Spilling ##

Register allocation is the process of assigning local variables and expression results of a GPU kernel to the registers available on the hardware. It is performed by the compiler at compilation time, and it is influenced by other stages like instruction scheduling.
Finding an optimal solution to this problem is hard (NP-Hard) and heuristic techniques need to be adopted to find sub-optimal solutions in a reasonable amount of time.

The compiler tries to apply heuristic techniques to maximize occupancy by reducing the need for registers following Table 1. When the amount of register requested starts becoming high performance is penalized by "register pressure" which leads to low occupancy and scratch memory usage.

Sometimes, the compiler may decide that it is fruitful to reach a better level of occupancy even though the request for registers is higher
than the limit reported in Table 1 (e.g., need for 134 registers but compiler allocates only 128 and the rest in scratch memory). This higher level of occupancy can be achieved by saving some variables in scratch memory: a portion of local memory, private to the thread, resident in 
global memory and much slower than register memory. This technique is called "register spilling".

Although observing scratch memory usage can be seen as a red flag for high register pressure, it should be considered in a broader performance context. In fact, achieving higher occupancy by saving a few registers can provide a substantial performance benefit as opposed to a lower
occupancy without any scratch memory usage.

In cases when the register pressure (request for registers) is way higher than the amount of hardware register available,
the performance will suffer of low occupancy (1 wave per CU in the worst case) and high cost of accessing the register variables that needed to be "spilled" to scratch memory.

## How to reduce register pressure ##

As mentioned before, the compiler applies sub-optimal heuristic techniques to maximize occupancy by minimizing the amount of register needed by a certain GPU kernels. These heuristic techniques sometimes fail to be close enough to an optimal solution and the programmer is required to restructure the code
in order to reduce register pressure and increase performance.

In this section, we will go through the steps of how to recognize a register pressure problem and how to mitigate it.

First of all, the amount of registers used by kernel can be detected in two ways: 1) compiling the file containing the kernels with the *-Rpass-analyze=kernel-resource-usage* flag, which will print at screen all the resource usage required by each kernel contained in the file at compile time.
Some of the information include SGPRs, VGPRs, ScratchSize, VGPR/SGPR spills, Occupancy, and LDS usage. 2) Compiling with *--save-temps* and look into the *hip-amdgcn-amd-amdhsa-gfx90a.s file for ".vgpr_spill_count".

Once the register pressure situation has been assessed/confirmed, there are a few tactics that can be adopted in the code to reduce it. 

<p>

1. ** Set the *__launch_bounds__* qualifier for each kernel **. By default, The compiler assumes that the block size of each kernel is composed by 1024 work item. When * __launch_bounds__ * is defined, the compiler can make more appropriate decision in register allocation, thus improving
the register pressure.

2. **Move variable declaration/assignment close to where they are used**. Assigning one or multiple variable at the top of a GPU kernel and use them at the very bottom forces the compiler those variables stored in register or scratch until they are used, thus impacting
the possibility of using those registers for more performance critical variables. By moving the declaration/assignment close to their first use will help the heuristic techniques to make more efficient choices on the rest of the code.

3. **Avoid allocating data on the stack**. Memory allocated on the stack (e.g., double array[10]) lives in scratch memory and it may be stored into registers by the compiler as an optimization step.
If your application makes use of memory allocated on the stack, seeing scratch memory usage should not be a big surprise.

4. **Avoid passing big object as kernel arguments**. Function arguments are allocated on the stack and may be optimized into registers as an optimization.

5. **Avoid writing large kernels with many function calls (including math functions and assertions)**. Currently, the compiler always inline device functions, including math function and assertions. Having many of these function calls introduces extra code and potentially higher register pressure.
As an example, replacing _pow(var,2.0)_ with a simple _var*var_ can significantly reduce register pressure.

6. **Keep loop unrolling under control**. Loop unrolling can be obtained by adding a _#pragma unroll_ command on a loop where the number of iterations is known at compile time. By doing so, all the iterations are completely unrolled, thus reducing the cost of checking the exit condition for the loop.
On the other hand, unrolling increases the register pressure because more variables need to be store in registers at the same time. In cases where register pressure is a concern, the use of loop unrolling should be limited.

7. **Manually spill to LDS**. As a last resort, it can be beneficial to use some LDS memory to manually store variables (possibly the ones with the longest liveness) and save a few register per thread.

##  ##

