// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#include "methods.h"

#include "../ext/cxxopts/include/cxxopts.hpp"  // TODO(GD) Very slow compilation

int main(int argc, char* argv[]) {

    cxxopts::Options options("benchmark_ringbuffers", "Benchmark ringbuffers types");
    options.add_options()
        ("i,iterations", "Number of total iteration for each chunk size.", cxxopts::value<int>()->default_value("100"))
        ("c,chunk_size_max", "Max chunk size.", cxxopts::value<int>()->default_value("8192"))
        ("r,nb_repeat", "Number of repetition of each instruction, to increase measure accuracy.", cxxopts::value<int>()->default_value("100"))
        ("h,help", "Print usage")
    ;
    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << std::endl;
        exit(0);
    }

    // std::srand(std::time(nullptr));
    std::srand(0);

    int nb_iter = result["iterations"].as<int>();
    std::cout << "#Iterations: " << nb_iter << std::endl;
    int chunk_size_max = result["chunk_size_max"].as<int>();
    int nb_repeat = result["nb_repeat"].as<int>();
    std::cout << "chunk_size_max: " << chunk_size_max << std::endl;

    std::vector<Method*> methods;
    methods.push_back(new MethodFastestBound(chunk_size_max, nb_repeat));
    methods.push_back(new MethodSTL(chunk_size_max, nb_repeat));
    std::deque<float>& arr_ref = static_cast<MethodSTL*>(methods.back())->m_buffer;  // The reference implementation
    methods.push_back(new MethodBoost(chunk_size_max, nb_repeat));
    methods.push_back(new MethodPortaudio(chunk_size_max, nb_repeat));
    methods.push_back(new MethodRubberBand(chunk_size_max, nb_repeat));
    methods.push_back(new MethodJack(chunk_size_max, nb_repeat));
    methods.push_back(new MethodACBench(chunk_size_max, nb_repeat));

    std::random_device rd;  // a seed source for the random number engine
    // std::mt19937 gen(rd());
    std::mt19937 gen(0);

    std::vector<int> methodorder(methods.size());
    std::iota(methodorder.begin(), methodorder.end(), 0);


    // Scenario: push_back_array  -----------------------------------------
    for (int chunk_size = 1; chunk_size <= chunk_size_max; chunk_size = static_cast<int>(1+chunk_size*1.1)) {
        std::cout << "INFO: chunk_size=" << chunk_size << std::endl;
        for (int iter=0; iter < nb_iter; ++iter) {
            float* chunk_push = new float[chunk_size];
            for (int n=0; n < chunk_size; ++n)
                chunk_push[n] = acbench::rand_uniform_continuous_01<float>();

            // Run each method in a randomized order
            std::random_shuffle(methodorder.begin(), methodorder.end());
            for (int mi=0; mi < static_cast<int>(methods.size()); ++mi) {
                methods[methodorder[mi]]->run_push_back_array(chunk_push, chunk_size);
            }

            delete[] chunk_push;
        }

        for (auto pmethod : methods) {
            pmethod->write_file("push_back_array_"+acbench::to_string<int>(chunk_size, "%i"));
            // std::cout << pmethod->m_name << ": " << pmethod->m_elapsed.stats(9) << std::endl;
            pmethod->m_elapsed.reset();
        }
    }

    for (auto pmethod : methods)
        pmethod->compare(arr_ref);


    // Scenario: push_pull_array ------------------------------------------
    for (auto pmethod : methods)
        pmethod->clear();

    for (int chunk_size = 1; chunk_size <= chunk_size_max; chunk_size = static_cast<int>(1+chunk_size*1.1)) {
        std::cout << "INFO: chunk_size=" << chunk_size << std::endl;
        int chunk_push_size = chunk_size;
        int chunk_pull_size = chunk_size;
        for (int iter=0; iter < nb_iter; ++iter) {
            float* chunk_push = new float[chunk_push_size];
            for (int n=0; n < chunk_push_size; ++n)
                chunk_push[n] = acbench::rand_uniform_continuous_01<float>();
            float* chunk_pull = new float[chunk_pull_size];

            // Run each method in a randomized order
            std::random_shuffle(methodorder.begin(), methodorder.end());
            for (int mi=0; mi < static_cast<int>(methods.size()); ++mi) {
                methods[methodorder[mi]]->run_push_pull_array(chunk_push, chunk_push_size, chunk_pull, chunk_pull_size);
            }

            delete[] chunk_push;
            delete[] chunk_pull;
        }

        for (auto pmethod : methods) {
            pmethod->write_file("push_pull_array_"+acbench::to_string<int>(chunk_size, "%i"));
            // std::cout << pmethod->m_name << ": " << pmethod->m_elapsed.stats(9) << std::endl;
            pmethod->m_elapsed.reset();
        }
    }

    for (auto pmethod : methods)
        pmethod->compare(arr_ref);


    // Scenario: push_back_const ----------------------------------------------
    // Not very interesting comparison as none of the methods are optimized for
    // this use case, except ACBench. Thus ACBench is ~50 times faster than the others.
    #if 0
        for (int chunk_size = 1; chunk_size <= chunk_size_max; chunk_size = static_cast<int>(1+chunk_size*1.1)) {
            std::cout << "INFO: chunk_size=" << chunk_size << std::endl;
            for (int iter=0; iter < nb_iter; ++iter) {
                float value = acbench::rand_uniform_continuous_01<float>();

                // Run each method in a randomized order
                std::random_shuffle(methodorder.begin(), methodorder.end());
                for (int mi=0; mi < static_cast<int>(methods.size()); ++mi)
                    methods[methodorder[mi]]->run_push_back_const(value, chunk_size);
            }

            for (auto pmethod : methods) {
                pmethod->write_file("const_"+acbench::to_string<int>(chunk_size, "%i"));
                // std::cout << pmethod->m_name << ": " << pmethod->m_elapsed.stats(9) << std::endl;
                pmethod->m_elapsed.reset();
            }
        }

        for (auto pmethod : methods)
            pmethod->compare(arr_ref);
    #endif


    return 0;
}
