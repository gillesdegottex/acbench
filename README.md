![workflow](https://github.com/gillesdegottex/acbench/actions/workflows/cmake-multi-platform.yml/badge.svg)

Audio Containers Bench (ACBench)
================================

What?!? Yet another ringbuffer implementation?! ... Yes, well, no, wait, let me explain...

Implementation for audio needs the following properties:
* Optimised for float, 32b by default, 64b available.
* Forbidden implicit reallocation.
* Allows math operations with arrays of all sorts.

So, this repository propose to meet the following three goals:
* Benchmark/Compare known implementations of audio-related containers.
* Either point to existing solutions or suggest C++ templates for these containers (in the `acbench` namespace) that should be as efficient as possible.
* It is quite easy to break the performances of those containers. A wrong compilation flag, a small coding mistake that slips in a commit and the performances degrades drastically. This repository should also provide a simple to use test class for your favorite containers that can be easily included in a CI.

We start with ringbuffers, then simple vectors, then let's see.

### Specification of the implemented containers

* Compatible from C++11 and onwards.
* Self sufficient one header file per container type.
* Ability to pre-allocate the memory used, without re-allocation when used.
* A simple testing framework that can be run in a CI to ensure your containers are always as efficient as they should be.

### Usage

If you want to use the built-in `acbench::ringbuffer` class in your project, copy paste the file `ringbuffer.h` wherever you like.

The only 3 functions that changes any allocation is `resize_allocate(int)`, `reserve(int)` which implements the STL behavior and the destructor. Those 3 functions are only called by the user, there is no implicit calls for them in any other functions. Copy constructor is forbidden. Empty constructor does nothing. There is no other constructor. Assignement operator is allowed.

    acbench::ringbuffer<float> rb;
    rb.resize_allocation(44100)  // Allocation for a 1s buffer at 44.1kHz

    // Use rb like an std::deque, though try push_back(.) and pop_front(.) with float arrays instead of single float values.

## License

Apache 2.0, please see LICENSE file.

WARNING: Only the code of this project, including but not limited to the `acbench` namespace, is under the license mentionned above. However, the code of the other external implementations (ex. in the `benchmarks/ext` directory) is likely to NOT be under the same license (ex. Rubberband uses a commercial license). The licenses of those external implementations still fully apply. So please ensure you comply with the terms of these specific licences if you eventually use those external implementations.


# Benchmarking

## Benchmark vs. reality

In a real scenario, conversely to a benchmark framework, the same operation is never run 100'000 times in a row. Some mechanisms are thus necessary to make the benchmark either more realistic or at least indenpendant of usage scenarios.

It is first necessary to prevent the compiler to optimise inter-iterations (i.e. prevent the execution used on the 1st iteration to be somehow merged/influenced with the execution of the 2nd iteration). Two solutions are used here:
* The containers are benchmarked all together. Namely, for a given scenario (ex. a push of 512 values), for each iteration, the compared containers are used in a random order and this order is randomized for each iteration (i.e. each container is _not_ benchmarked systematically one after the same other one).

* By writting down the code for each container one below each other, in the same compilation unit, the position of the code block ends up impacting the performances (i.e. benchmarking `std::deque::push_back(.); RubberBand::RingBuffer<float>::write(.)` or `RubberBand::RingBuffer<float>::write(.); std::deque::push_back(.)` gives different results.). To make the benchmark results independent of the code position in the compilation unit, each container is encapsulated in a class, and benchmarked in a dedicated virtual function (note, the containers do _not_ use virtual functions of course, only the benchmark framework does).

Currently only 3 scenarios are tested for the ringbuffers (push_back an array, push_back then pop_front an array, push_back const values (often used when split a signal into frames)).
This is obviously very limited and represent only a small possibilities of usage.
So If you want to compare, just add your scenario.


## Benchmarking/Comparisons

### On Intel and Linux platforms

You might want to first stop the automatic throttling of the CPU and run it full speed to avoid this mechanism to impact the results of the benchmark:
To push the CPU to full speed:
```
echo 100 | sudo tee /sys/devices/system/cpu/intel_pstate/min_perf_pct
```
To let the throttling mechanism work again:
```
echo 1 | sudo tee /sys/devices/system/cpu/intel_pstate/min_perf_pct
```

### Usage

Benchmarking is done running the following commands

    cmake -DACBENCH_TESTS=OFF -DACBENCH_ASSERT=OFF -DACBENCH_TESTCOVERAGE=OFF ..
    mkdir run; cd run
    ../ringbuffers/benchmark_ringbuffers -i 1000
    python3 ../../ringbuffers/plot.py



## Ringbuffers

### Compared implementations

Most ringbuffers implementations that are _not_ dedicated to audio are underperforming for audio because most of them do not provide functions for pushing and pulling chunks of data, but only one value at a time.
Implementations that do not provide this functionality are thus ignored in this benchmark.

Current implementations:
* [Portaudio](https://portaudio.com/docs/v19-doxydocs-dev/pa__ringbuffer_8h.html): `PaUtilRingBuffer` ([Similar to MIT licence](https://portaudio.music.columbia.narkive.com/gzVJow3c/licensing-and-gnu))
* [Rubberband](https://github.com/breakfastquay/rubberband) `RubberBand::RingBuffer<float>` (Commercial License!)
* [Jack](https://jackaudio.org/api/ringbuffer_8h.html) `jack_ringbuffer_t` (LGPL)
* [Boost](https://www.boost.org/doc/libs/1_79_0/doc/html/circular_buffer.html): `boost::circular_buffer<float>` (Boost Software License, [Similar to MIT licence](https://fossa.com/blog/open-source-licenses-101-boost-software-license/))
* [STL](https://en.cppreference.com/w/cpp/container/deque): `std::deque<float>`
* ACBench: `acbench::ringbuffer<float>` (the one from this repository)

#### To add

* [Juce](https://forum.juce.com/t/ringbuffer-is-a-missing-piece/5202), [missing a ringbuffer?](https://forum.juce.com/t/pure-c-circularbuffer/58917)

## Vectors

to come...


## Testing

### Continuous Integration (CI)

In a CI, the following pipeline is recommended:

    mkdir build; cd build
    cmake -DACBENCH_TESTS=ON -DACBENCH_ASSERT=ON -DACBENCH_TESTCOVERAGE=ON ..
    make test_coverage
    cmake -DACBENCH_TESTS=ON -DACBENCH_ASSERT=ON -DACBENCH_TESTCOVERAGE=OFF ..
    ctest

Note that `make test_coverage` will print a summary, generate a webpage `ringbuffer_test_coverage.html` showing the results and generate `ringbuffer_test_coverage.json` and `ringbuffer_test_coverage.summary.json` that can be used for reporting.


