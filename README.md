# NVIDIA Tools Extension for C++

Simple RAII wrapper for NVTX profiling library.

This was written a while ago, and probably will not work with the most recent
CUDA Toolkit versions.

## Quick doc

NVTX is a profiling library that introduces "special markers" to the profiled
CUDA-accelerated code. This simple library provides C++ wrappers that allow
creating markers based on the variable scope. More or less. this is how it was
supposed to work (the data is later analyzed by NVIDIA Nsight):

```C++

// the label and color is displayed in NVIDIA Nsight
const nvtx4cpp::NvAttribute prAttr{"processing", nvtx4cpp::NvColor::dark_red};
const nvtx4cpp::NvAttribute itAttr{"iteration",  nvtx4cpp::NvColor::gray};

void iterate ()
{
    // Thread range (dark red) starts with the function and ends when function exits.
    nvtx4cpp::NvThreadRange profileProcessing{prAttr};

    for (unsigned i = 0 ; i < 10; i++)
    {
        // Thread range (gray) starts with each iteration and ends before the next iteration.
        nvtx4cpp::NvThreadRange profileIteration{itAttr};

        some_CUDA_work();
    }
}

int main()
{
    cudaProfilerStart();
    iterate();
    cudaProfilerStop();
}

```
