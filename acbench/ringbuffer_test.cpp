// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#include <acbench/ringbuffer.h>

#include "utils.h"

#include <deque>

#include <catch2/catch_test_macros.hpp>

typedef acbench::ringbuffer<float> test_t;
typedef std::deque<float> ref_t;

void rb_init(test_t& test, ref_t& ref, int size) {
    test.resize_allocation(size);
    REQUIRE(test.size_max() == size);
    REQUIRE(test.size() == 0);

    // TODO A way to do a .reserve() for std::deque ?
    REQUIRE(ref.size() == 0);
}

template<typename ringbuffer1_t, typename ringbuffer2_t>
void rb_push_back_rand(ringbuffer1_t& rb1, ringbuffer2_t& rb2, int n) {
    for (int i=0; i < n; ++i) {
        auto value = acbench::rand_uniform_continuous_01<float>();
        rb1.push_back(value);
        rb2.push_back(value);
    }
}

template<typename ringbuffer1_t>
void rb_push_back_rand_single(ringbuffer1_t& rb1, int n) {
    for (int i=0; i < n; ++i) {
        auto value = acbench::rand_uniform_continuous_01<float>();
        rb1.push_back(value);
    }
}

void rb_push_back_const(test_t& test, ref_t& ref, float value) {
    test.push_back(value);
    ref.push_back(value);
}

void rb_push_back_const(test_t& test, ref_t& ref, float value, int n) {
    test.push_back(value, n);
    for (int i=0; i < n; ++i)
        ref.push_back(value);
}

void rb_push_back_array(test_t& test, ref_t& ref, float* array, int n) {
    test.push_back(array, n);
    for (int i=0; i < n; ++i) 
        ref.push_back(array[i]);
}

template<typename ringbuffer3_t>
void rb_push_back_ringbuffer(test_t& test, ref_t& ref, ringbuffer3_t& rb) {
    test.push_back(rb);
    for (int i=0; i < rb.size(); ++i)
        ref.push_back(rb[i]);
}

void rb_pop_front(test_t& test, ref_t& ref) {
    test.pop_front();
    ref.pop_front();
}

void rb_pop_front(test_t& rb1, ref_t& ref, int n) {
    rb1.pop_front(n);
    for (int i=0; i < n; ++i) {
        if (ref.size() > 0)
            ref.pop_front();
    }
}

void rb_pop_front(test_t& rb1, ref_t& ref, float* array, int n) {
    int n_poped = rb1.pop_front(array, n);
    REQUIRE(n_poped <= n);
    for (int i=0; i < n; ++i) {
        float ref_value = 0.0f;
        if (ref.size() > 0) {
            ref_value = ref.front();
            ref.pop_front();
            REQUIRE(ref_value == array[i]);
        }
    }
}


template<typename ringbuffer1_t, typename ringbuffer2_t>
void rb_clear(ringbuffer1_t& rb1, ringbuffer2_t& rb2) {
    rb1.clear();
    rb2.clear();
}

template<typename ringbuffer1_t, typename ringbuffer2_t>
void rb_require_equals(ringbuffer1_t& rb1, ringbuffer2_t& rb2) {
    REQUIRE(int(rb1.size()) == int(rb2.size()));
    for (int i=0; i < rb1.size(); ++i)
        REQUIRE(rb1[i] == rb2[i]);
}

void rb_require_equals_array(float* data1, float* data2, int n) {
    for (int i=0; i < n; ++i)
        REQUIRE(data1[i] == data2[i]);
}

template<typename ringbuffer1_t>
void rb_require_equals_array(ringbuffer1_t& rb1, float* data, int n) {
    REQUIRE(int(rb1.size()) == n);
    for (int i=0; i < int(rb1.size()); ++i)
        REQUIRE(rb1[i] == data[i]);
}

TEST_CASE("ringbuffer_ctor") {

    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_require_equals(test, ref);

    rb_push_back_rand(test, ref, chunk_size);
    REQUIRE(int(test.size()) == chunk_size);
    REQUIRE(int(ref.size()) == chunk_size);

    rb_require_equals(test, ref);

    test.resize_allocation(chunk_size);
    REQUIRE(test.size() == 0);
    ref.clear();
    REQUIRE(ref.size() == 0);

    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_accessors") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_const(test, ref, 1.0f);

    rb_push_back_rand(test, ref, chunk_size-1);
    REQUIRE(test.size() == chunk_size);

    REQUIRE(test[0] == 1.0f);
    REQUIRE(test.front() == test[0]);

    // Test operator[int] const
    const acbench::ringbuffer<float>& test_const = test;
    REQUIRE(test_const[0] == 1.0f);

    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_singlevalues") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    test.resize_allocation(chunk_size);
    REQUIRE(test.size_max() == chunk_size);

    rb_push_back_const(test, ref, 1.0f);
    REQUIRE(test.size() == 1);
    rb_clear(test, ref);
    REQUIRE(test.size() == 0);
    rb_require_equals(test, ref);

    rb_push_back_const(test, ref, 1.0f);
    REQUIRE(test.size() == 1);
    rb_push_back_const(test, ref, 2.0f);
    REQUIRE(test.size() == 2);
    rb_pop_front(test, ref);
    REQUIRE(test.size() == 1);

    rb_require_equals(test, ref);
}


TEST_CASE("ringbuffer_assign") {

    int chunk_size = 100;

    test_t rb1;
    ref_t ref;
    rb_init(rb1, ref, chunk_size);

    rb_push_back_rand(rb1, ref, chunk_size);
    REQUIRE(rb1.size() == chunk_size);

    rb_require_equals(rb1, ref);

    test_t rb2;
    rb2.resize_allocation(chunk_size);
    rb2 = rb1;

    rb_require_equals(rb2, ref);

    REQUIRE(rb1.data() != rb2.data());
    rb_require_equals(rb1, rb2);
}

TEST_CASE("ringbuffer_push_back_cst") {

    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_const(test, ref, 0.0f, chunk_size);
    REQUIRE(test.size() == chunk_size);

    rb_clear(test, ref);
    rb_push_back_const(test, ref, 0.0f, 75);
    REQUIRE(test.size() == 75);
    rb_pop_front(test, ref, 50);
    REQUIRE(test.size() == 25);
    rb_push_back_const(test, ref, 1.0f, 50);
    REQUIRE(test.size() == 75);

    rb_require_equals(test, ref);

    // Shortcuts
    rb_push_back_const(test, ref, 1.0f, 0);
    rb_push_back_const(test, ref, 1.0f, 1);
    rb_pop_front(test, ref, 0);

    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_push_back_array") {

    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    float* data = new float[chunk_size];
    for (int i=0; i < chunk_size; ++i)
        data[i] = acbench::rand_uniform_continuous_01<float>();

    rb_push_back_array(test, ref, data, chunk_size);

    rb_require_equals(test, ref);

    rb_require_equals_array(test, data, chunk_size);
    rb_require_equals_array(ref, data, chunk_size);

    // Need to slice the array into two segments
    rb_clear(test, ref);
    rb_push_back_array(test, ref, data, 75);
    REQUIRE(test.size() == 75);
    rb_pop_front(test, ref, 50);
    REQUIRE(test.size() == 25);
    rb_push_back_array(test, ref, data, 50);
    REQUIRE(test.size() == 75);

    rb_require_equals(test, ref);

    // Test shortcuts
    rb_push_back_array(test, ref, data, 0);  // TODO ?

    rb_require_equals(test, ref);

    delete[] data;
}

TEST_CASE("ringbuffer_push_back_ringbuffer") {

    int chunk_size = 100;

    test_t rb1;
    ref_t ref;
    rb_init(rb1, ref, chunk_size);

    test_t rb2;
    rb2.resize_allocation(chunk_size);
    rb_push_back_rand_single(rb2, chunk_size);

    rb_push_back_ringbuffer(rb1, ref, rb2);
    rb_require_equals(rb1, rb2);
    rb_require_equals(rb1, ref);


    // The destination segment is continuous
    // The source segment is made of two continuous segments
    rb2.pop_front(75);
    rb_push_back_rand_single(rb2, 25);
    REQUIRE(rb2.size() == 50);

    rb_clear(rb1, ref);
    rb_push_back_ringbuffer(rb1, ref, rb2);
    rb_require_equals(rb1, rb2);
    rb_require_equals(rb1, ref);


    // The destination segment is made of two continuous segments
    // The source segment is continuous...
    rb_clear(rb1, ref);
    rb_push_back_rand(rb1, ref, 75);
    rb_pop_front(rb1, ref, 50);
    REQUIRE(rb1.size() == 25);
    rb_require_equals(rb1, ref);

    rb2.clear();
    rb_push_back_rand_single(rb2, 50);
    // rb2.print_details(&(std::cout));

    rb_push_back_ringbuffer(rb1, ref, rb2);
    REQUIRE(rb1.size() == 75);
    rb_require_equals(rb1, ref);


    // The destination segment is made of two continuous segments
    // The source segment is also made of two continuous segments...

    // the source's break point comes before the destination's max size...
    rb_clear(rb1, ref);
    rb_push_back_rand(rb1, ref, 75);
    rb_pop_front(rb1, ref, 50);
    REQUIRE(rb1.size() == 25);

    rb2.clear();
    rb_push_back_rand_single(rb2, chunk_size);
    rb2.pop_front(80);
    rb_push_back_rand_single(rb2, 25);
    REQUIRE(rb2.size() == 45);

    rb_push_back_ringbuffer(rb1, ref, rb2);
    REQUIRE(rb1.size() == 70);
    rb_require_equals(rb1, ref);


    // the source's break point comes after the destination's max size...
    rb_clear(rb1, ref);
    rb_push_back_rand(rb1, ref, 75);
    rb_pop_front(rb1, ref, 50);
    REQUIRE(rb1.size() == 25);
    rb_require_equals(rb1, ref);

    rb2.clear();
    rb_push_back_rand_single(rb2, chunk_size);
    rb2.pop_front(70);
    rb_push_back_rand_single(rb2, 25);
    REQUIRE(rb2.size() == 55);

    rb_push_back_ringbuffer(rb1, ref, rb2);
    REQUIRE(rb1.size() == 80);
    rb_require_equals(rb1, ref);


    // Test shortcuts
    rb2.clear();
    rb_clear(rb1, ref);
    rb_push_back_ringbuffer(rb1, ref, rb2);
    rb_require_equals(rb1, ref);
}

TEST_CASE("ringbuffer_pop_front") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_rand(test, ref, chunk_size);
    REQUIRE(test.size() == chunk_size);

    rb_pop_front(test, ref, 34);
    REQUIRE(test.size() == 66);
    rb_pop_front(test, ref, 567);
    REQUIRE(test.size() == 0);

    rb_require_equals(test, ref);


    rb_clear(test, ref);
    rb_push_back_rand(test, ref, chunk_size);
    REQUIRE(test.size() == chunk_size);

    rb_pop_front(test, ref, 75);
    REQUIRE(test.size() == 25);

    rb_push_back_rand(test, ref, 25);
    REQUIRE(test.size() == 50);

    rb_pop_front(test, ref, 33);
    REQUIRE(test.size() == 17);

    rb_require_equals(test, ref);


    rb_clear(test, ref);
    rb_push_back_rand(test, ref, chunk_size);
    REQUIRE(test.size() == chunk_size);
    rb_pop_front(test, ref, 99);
    rb_pop_front(test, ref);

    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_pop_front_array") {

    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    float* data = new float[chunk_size];
    for (int i=0; i < chunk_size; ++i)
        data[i] = acbench::rand_uniform_continuous_01<float>();

    rb_push_back_array(test, ref, data, chunk_size);

    rb_require_equals(test, ref);

    rb_require_equals_array(test, data, chunk_size);
    rb_require_equals_array(ref, data, chunk_size);

    // Test pop_front(array) when the data lays in a single continuous segment
    float* data_poped = new float[chunk_size];
    rb_pop_front(test, ref, data_poped, chunk_size);
    rb_require_equals(test, ref);
    rb_require_equals_array(data, data_poped, chunk_size);

    // Test pop_front(array) when the data is split in two separate segments
    rb_clear(test, ref);
    rb_push_back_array(test, ref, data, chunk_size);
    REQUIRE(test.size() == chunk_size);
    rb_require_equals(test, ref);
    rb_pop_front(test, ref, data_poped, 75);
    REQUIRE(test.size() == 25);
    rb_require_equals(test, ref);
    rb_push_back_array(test, ref, data, 25);
    REQUIRE(test.size() == 50);
    rb_require_equals(test, ref);

    rb_pop_front(test, ref, data_poped, 40);
    rb_require_equals(test, ref);

    // Test shortcuts
    rb_pop_front(test, ref, data_poped, 20);
    rb_require_equals(test, ref);

    rb_pop_front(test, ref, data_poped, 0);

    delete[] data_poped;
    delete[] data;
}


