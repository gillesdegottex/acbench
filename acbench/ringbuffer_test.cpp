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

void rb_push_front_const(test_t& test, ref_t& ref, float value) {
    test.push_front(value);
    ref.push_front(value);
}

void rb_push_front_const(test_t& test, ref_t& ref, float value, int n) {
    test.push_front(value, n);
    for (int i=0; i < n; ++i)
        ref.push_front(value);
}

void rb_push_front_array(test_t& test, ref_t& ref, float* array, int n) {
    test.push_front(array, n);
    for (int i=n-1; i >= 0; --i) 
        ref.push_front(array[i]);
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

TEST_CASE("ringbuffer_push_front_single") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_front_const(test, ref, 1.0f);
    REQUIRE(test.size() == 1);
    REQUIRE(test.front() == 1.0f);
    rb_require_equals(test, ref);

    rb_push_front_const(test, ref, 2.0f);
    REQUIRE(test.size() == 2);
    REQUIRE(test.front() == 2.0f);
    REQUIRE(test[0] == 2.0f);
    REQUIRE(test[1] == 1.0f);
    rb_require_equals(test, ref);

    rb_clear(test, ref);
    rb_push_back_rand(test, ref, 50);
    REQUIRE(test.size() == 50);
    rb_push_front_const(test, ref, 99.0f);
    REQUIRE(test.size() == 51);
    REQUIRE(test.front() == 99.0f);
    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_push_front_cst") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_front_const(test, ref, 0.0f, chunk_size);
    REQUIRE(test.size() == chunk_size);
    rb_require_equals(test, ref);

    rb_clear(test, ref);
    rb_push_front_const(test, ref, 0.0f, 75);
    REQUIRE(test.size() == 75);
    rb_pop_front(test, ref, 50);
    REQUIRE(test.size() == 25);
    rb_push_front_const(test, ref, 1.0f, 50);
    REQUIRE(test.size() == 75);
    rb_require_equals(test, ref);

    // Test wrapping around when pushing front
    rb_clear(test, ref);
    rb_push_back_rand(test, ref, 75);
    REQUIRE(test.size() == 75);
    rb_pop_front(test, ref, 50);
    REQUIRE(test.size() == 25);
    rb_push_front_const(test, ref, 2.0f, 50);
    REQUIRE(test.size() == 75);
    rb_require_equals(test, ref);

    // Shortcuts
    rb_push_front_const(test, ref, 1.0f, 0);
    rb_push_front_const(test, ref, 1.0f, 1);
    rb_pop_front(test, ref, 0);

    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_push_front_array") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    float* data = new float[chunk_size];
    for (int i=0; i < chunk_size; ++i)
        data[i] = acbench::rand_uniform_continuous_01<float>();

    rb_push_front_array(test, ref, data, chunk_size);
    rb_require_equals(test, ref);

    // Verify the order is correct (array[0] becomes front element)
    for (int i=0; i < chunk_size; ++i)
        REQUIRE(test[i] == data[i]);

    // Test wrapping around when pushing front array
    rb_clear(test, ref);
    rb_push_back_rand(test, ref, 75);
    REQUIRE(test.size() == 75);
    rb_pop_front(test, ref, 50);
    REQUIRE(test.size() == 25);
    rb_push_front_array(test, ref, data, 50);
    REQUIRE(test.size() == 75);
    rb_require_equals(test, ref);

    // Test when buffer wraps around during push_front
    rb_clear(test, ref);
    rb_push_back_rand(test, ref, chunk_size);
    REQUIRE(test.size() == chunk_size);
    rb_pop_front(test, ref, 90);
    REQUIRE(test.size() == 10);
    rb_push_front_array(test, ref, data, 50);
    REQUIRE(test.size() == 60);
    rb_require_equals(test, ref);

    // Test shortcuts
    rb_push_front_array(test, ref, data, 0);

    rb_require_equals(test, ref);

    delete[] data;
}

TEST_CASE("ringbuffer_push_front_wrapping") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    // Fill buffer, then pop from front to create wrap condition
    rb_push_back_rand(test, ref, chunk_size);
    REQUIRE(test.size() == chunk_size);
    rb_pop_front(test, ref, 80);
    REQUIRE(test.size() == 20);

    // Push front with values that will wrap around
    rb_push_front_const(test, ref, 5.0f, 60);
    REQUIRE(test.size() == 80);
    rb_require_equals(test, ref);

    // Verify front values
    for (int i=0; i < 60; ++i)
        REQUIRE(test[i] == 5.0f);

    // Test array push_front with wrapping
    rb_clear(test, ref);
    rb_push_back_rand(test, ref, chunk_size);
    rb_pop_front(test, ref, 70);
    REQUIRE(test.size() == 30);

    float* data = new float[50];
    for (int i=0; i < 50; ++i)
        data[i] = static_cast<float>(i);

    rb_push_front_array(test, ref, data, 50);
    REQUIRE(test.size() == 80);
    rb_require_equals(test, ref);

    // Verify the array was inserted correctly (array[0] becomes front)
    for (int i=0; i < 50; ++i)
        REQUIRE(test[i] == data[i]);

    delete[] data;
}

TEST_CASE("ringbuffer_push_front_mixed") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    // Mix push_front and push_back operations
    rb_push_front_const(test, ref, 1.0f);
    rb_push_back_const(test, ref, 2.0f);
    rb_push_front_const(test, ref, 3.0f);
    rb_push_back_const(test, ref, 4.0f);
    REQUIRE(test.size() == 4);
    REQUIRE(test.front() == 3.0f);
    REQUIRE(test.back() == 4.0f);
    rb_require_equals(test, ref);

    // Test with larger operations
    rb_clear(test, ref);
    rb_push_front_const(test, ref, 10.0f, 30);
    rb_push_back_const(test, ref, 20.0f, 30);
    REQUIRE(test.size() == 60);
    rb_require_equals(test, ref);

    // Verify front and back
    for (int i=0; i < 30; ++i)
        REQUIRE(test[i] == 10.0f);
    for (int i=30; i < 60; ++i)
        REQUIRE(test[i] == 20.0f);
}

TEST_CASE("ringbuffer_reserve") {
    int chunk_size = 50;
    int larger_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_rand(test, ref, 30);
    REQUIRE(test.size() == 30);
    REQUIRE(test.size_max() == chunk_size);

    // Reserve should preserve data when increasing size
    test.reserve(larger_size);
    REQUIRE(test.size_max() == larger_size);
    REQUIRE(test.size() == 30);
    rb_require_equals(test, ref);

    // Reserve should do nothing when size is smaller or equal
    int current_max = test.size_max();
    test.reserve(chunk_size);
    REQUIRE(test.size_max() == current_max);
    REQUIRE(test.size() == 30);
    rb_require_equals(test, ref);

    // Reserve should preserve data after wrapping
    rb_clear(test, ref);
    rb_push_back_rand(test, ref, chunk_size);
    rb_pop_front(test, ref, 20);
    rb_push_back_rand(test, ref, 20);
    REQUIRE(test.size() == chunk_size);
    
    test.reserve(larger_size);
    REQUIRE(test.size_max() == larger_size);
    REQUIRE(test.size() == chunk_size);
    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_back") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_const(test, ref, 1.0f);
    REQUIRE(test.back() == 1.0f);
    REQUIRE(test.back() == ref.back());

    rb_push_back_const(test, ref, 2.0f);
    REQUIRE(test.back() == 2.0f);
    REQUIRE(test.back() == ref.back());

    rb_push_back_rand(test, ref, 50);
    REQUIRE(test.back() == ref.back());

    // Test back after wrapping
    rb_pop_front(test, ref, 30);
    rb_push_back_rand(test, ref, 30);
    REQUIRE(test.back() == ref.back());
    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_empty") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    REQUIRE(test.empty() == true);
    REQUIRE(test.size() == 0);

    rb_push_back_const(test, ref, 1.0f);
    REQUIRE(test.empty() == false);
    REQUIRE(test.size() == 1);

    test.clear();
    REQUIRE(test.empty() == true);
    REQUIRE(test.size() == 0);

    rb_push_back_rand_single(test, 50);
    REQUIRE(test.empty() == false);
    test.pop_front(50);
    REQUIRE(test.empty() == true);
}

TEST_CASE("ringbuffer_data_indices") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_const(test, ref, 1.0f);
    REQUIRE(test.front_data_index() >= 0);
    REQUIRE(test.front_data_index() < chunk_size);
    REQUIRE(test.back_data_index() >= 0);
    REQUIRE(test.back_data_index() < chunk_size);

    // After wrapping, indices should still be valid
    rb_push_back_rand(test, ref, chunk_size - 1);
    rb_pop_front(test, ref, 50);
    rb_push_back_rand(test, ref, 50);
    REQUIRE(test.front_data_index() >= 0);
    REQUIRE(test.front_data_index() < chunk_size);
    REQUIRE(test.back_data_index() >= 0);
    REQUIRE(test.back_data_index() < chunk_size);

    // Verify data access through indices
    int front_idx = test.front_data_index();
    int back_idx = test.back_data_index();
    REQUIRE(test.data()[front_idx] == test.front());
    REQUIRE(test.data()[back_idx] == test.back());
}

TEST_CASE("ringbuffer_pop_back_single") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_const(test, ref, 1.0f);
    rb_push_back_const(test, ref, 2.0f);
    rb_push_back_const(test, ref, 3.0f);

    float val = test.pop_back();
    REQUIRE(val == 3.0f);
    ref.pop_back();
    rb_require_equals(test, ref);

    val = test.pop_back();
    REQUIRE(val == 2.0f);
    ref.pop_back();
    rb_require_equals(test, ref);

    val = test.pop_back();
    REQUIRE(val == 1.0f);
    ref.pop_back();
    rb_require_equals(test, ref);
    REQUIRE(test.empty());

    // Test with wrapping
    rb_push_back_rand(test, ref, chunk_size);
    rb_pop_front(test, ref, 50);
    rb_push_back_rand(test, ref, 50);

    val = test.pop_back();
    ref.pop_back();
    rb_require_equals(test, ref);

    // Test multiple pop_back operations
    for (int i = 0; i < 20; ++i) {
        val = test.pop_back();
        ref.pop_back();
        rb_require_equals(test, ref);
    }
}

TEST_CASE("ringbuffer_pop_back_multiple") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    rb_push_back_rand(test, ref, 50);
    REQUIRE(test.size() == 50);

    test.pop_back(10);
    for (int i = 0; i < 10; ++i)
        ref.pop_back();
    REQUIRE(test.size() == 40);
    rb_require_equals(test, ref);

    test.pop_back(30);
    for (int i = 0; i < 30; ++i)
        ref.pop_back();
    REQUIRE(test.size() == 10);
    rb_require_equals(test, ref);

    // Test pop_back more than available (should clear)
    test.pop_back(100);
    ref.clear();
    REQUIRE(test.size() == 0);
    rb_require_equals(test, ref);

    // Test with wrapping
    rb_push_back_rand(test, ref, chunk_size);
    rb_pop_front(test, ref, 60);
    rb_push_back_rand(test, ref, 60);
    REQUIRE(test.size() == chunk_size);

    test.pop_back(40);
    for (int i = 0; i < 40; ++i)
        ref.pop_back();
    REQUIRE(test.size() == 60);
    rb_require_equals(test, ref);

    // Test edge cases
    test.pop_back(0);  // Should do nothing
    rb_require_equals(test, ref);

    test.pop_back(-1);  // Should do nothing
    rb_require_equals(test, ref);
}

TEST_CASE("ringbuffer_pop_front_to_ringbuffer") {
    int chunk_size = 100;

    test_t rb1;
    test_t rb2;
    ref_t ref1;
    ref_t ref2;
    rb_init(rb1, ref1, chunk_size);
    rb_init(rb2, ref2, chunk_size);

    rb_push_back_rand(rb1, ref1, 50);
    REQUIRE(rb1.size() == 50);

    int n_poped = rb1.pop_front(rb2);
    REQUIRE(n_poped == 50);
    REQUIRE(rb1.size() == 0);
    REQUIRE(rb2.size() == 50);
    rb_require_equals(rb2, ref1);

    // Test with wrapping
    rb_clear(rb1, ref1);
    rb_clear(rb2, ref2);
    rb_push_back_rand(rb1, ref1, chunk_size);
    rb_pop_front(rb1, ref1, 60);
    rb_push_back_rand(rb1, ref1, 60);
    REQUIRE(rb1.size() == chunk_size);

    n_poped = rb1.pop_front(rb2);
    REQUIRE(n_poped == chunk_size);
    REQUIRE(rb1.size() == 0);
    REQUIRE(rb2.size() == chunk_size);
    rb_require_equals(rb2, ref1);

    // Test partial pop
    rb_clear(rb1, ref1);
    rb_clear(rb2, ref2);
    rb_push_back_rand(rb1, ref1, 30);
    rb_push_back_rand(rb2, ref2, 20);

    n_poped = rb1.pop_front(rb2);
    REQUIRE(n_poped == 30);
    REQUIRE(rb1.size() == 0);
    REQUIRE(rb2.size() == 50);
    
    // Verify rb2 contains original rb2 data followed by rb1 data
    for (int i = 0; i < 20; ++i)
        REQUIRE(rb2[i] == ref2[i]);
    for (int i = 0; i < 30; ++i)
        REQUIRE(rb2[20 + i] == ref1[i]);
}

TEST_CASE("ringbuffer_pop_back_wrapping") {
    int chunk_size = 100;

    test_t test;
    ref_t ref;
    rb_init(test, ref, chunk_size);

    // Fill buffer and create wrap condition
    rb_push_back_rand(test, ref, chunk_size);
    rb_pop_front(test, ref, 80);
    rb_push_back_rand(test, ref, 80);
    REQUIRE(test.size() == chunk_size);

    // Pop back with wrapping
    test.pop_back(60);
    for (int i = 0; i < 60; ++i)
        ref.pop_back();
    REQUIRE(test.size() == 40);
    rb_require_equals(test, ref);

    // Continue popping
    test.pop_back(30);
    for (int i = 0; i < 30; ++i)
        ref.pop_back();
    REQUIRE(test.size() == 10);
    rb_require_equals(test, ref);
}

// Scenario: push_back up to max capacity, pop_front all, then push_back again.
TEST_CASE("ringbuffer_push_back_segment_m_end_wrap_bug") {
    int chunk_size = 10;

    // Test 1: Using push_back(rb, start, size) - tests line 559
    {
        test_t test;
        test.resize_allocation(chunk_size);
        REQUIRE(test.size() == 0);
        REQUIRE(test.capacity() == chunk_size);

        // Create source ringbuffer with exactly chunk_size elements
        test_t src;
        src.resize_allocation(chunk_size);
        for (int i = 0; i < chunk_size; ++i)
            src.push_back(static_cast<float>(i));
        REQUIRE(src.size() == chunk_size);

        // Push back the entire source segment - this triggers the bug
        // m_end starts at 0, we push chunk_size elements
        // After: m_end should be 0 (wrapped), not chunk_size (out of bounds)
        test.push_back(src, 0, chunk_size);
        REQUIRE(test.size() == chunk_size);

        // Verify values
        for (int i = 0; i < chunk_size; ++i)
            REQUIRE(test[i] == static_cast<float>(i));

        // Pop front all elements using the array version
        float* poped = new float[chunk_size];
        int n_poped = test.pop_front(poped, chunk_size);
        REQUIRE(n_poped == chunk_size);
        REQUIRE(test.size() == 0);

        // Verify popped values
        for (int i = 0; i < chunk_size; ++i)
            REQUIRE(poped[i] == static_cast<float>(i));

        delete[] poped;

        // Now push_back again - this is where the bug manifests!
        // If m_end == m_size_max (bug), this will access m_data[m_size_max] out of bounds
        test.push_back(42.0f);
        REQUIRE(test.size() == 1);
        REQUIRE(test.front() == 42.0f);
        REQUIRE(test.back() == 42.0f);

        // Push more to verify state is consistent
        test.push_back(43.0f);
        test.push_back(44.0f);
        REQUIRE(test.size() == 3);
        REQUIRE(test[0] == 42.0f);
        REQUIRE(test[1] == 43.0f);
        REQUIRE(test[2] == 44.0f);
    }

    // Test 2: Using push_back(rb) without segment - tests line 455
    {
        test_t test;
        test.resize_allocation(chunk_size);

        test_t src;
        src.resize_allocation(chunk_size);
        for (int i = 0; i < chunk_size; ++i)
            src.push_back(static_cast<float>(i + 100));

        test.push_back(src);
        REQUIRE(test.size() == chunk_size);

        float* poped = new float[chunk_size];
        test.pop_front(poped, chunk_size);
        REQUIRE(test.size() == 0);

        delete[] poped;

        // Push again - would crash if m_end wasn't wrapped
        test.push_back(99.0f);
        REQUIRE(test.size() == 1);
        REQUIRE(test.front() == 99.0f);
    }

    // Test 3: Using push_back(array, size) - tests line 318
    {
        test_t test;
        test.resize_allocation(chunk_size);

        float* data = new float[chunk_size];
        for (int i = 0; i < chunk_size; ++i)
            data[i] = static_cast<float>(i + 200);

        test.push_back(data, chunk_size);
        REQUIRE(test.size() == chunk_size);

        float* poped = new float[chunk_size];
        test.pop_front(poped, chunk_size);
        REQUIRE(test.size() == 0);

        // Push again
        test.push_back(88.0f);
        REQUIRE(test.size() == 1);
        REQUIRE(test.front() == 88.0f);

        delete[] data;
        delete[] poped;
    }

    // Test 4: Using push_back(value, count) - tests line 283
    {
        test_t test;
        test.resize_allocation(chunk_size);

        test.push_back(7.0f, chunk_size);
        REQUIRE(test.size() == chunk_size);

        float* poped = new float[chunk_size];
        test.pop_front(poped, chunk_size);
        REQUIRE(test.size() == 0);

        delete[] poped;

        // Push again
        test.push_back(77.0f);
        REQUIRE(test.size() == 1);
        REQUIRE(test.front() == 77.0f);
    }
}

TEST_CASE("ringbuffer_dynamic_allocation") {
    // Test default state
    test_t test;
    REQUIRE(test.dynamic_allocation() == false);

    // Test enabling dynamic allocation
    test.set_dynamic_allocation(true);
    REQUIRE(test.dynamic_allocation() == true);

    // Test auto-growth from empty (no pre-allocation)
    test.push_back(1.0f);
    REQUIRE(test.size() == 1);
    REQUIRE(test.capacity() >= 1);
    REQUIRE(test[0] == 1.0f);

    // Test auto-growth by pushing many values
    int initial_capacity = test.capacity();
    for (int i = 1; i < 100; ++i) {
        test.push_back(static_cast<float>(i + 1));
    }
    REQUIRE(test.size() == 100);
    REQUIRE(test.capacity() >= 100);
    REQUIRE(test.capacity() > initial_capacity);

    // Verify all data is preserved after multiple growths
    for (int i = 0; i < 100; ++i) {
        REQUIRE(test[i] == static_cast<float>(i + 1));
    }

    // Test growth with array push
    test.clear();
    float data[50];
    for (int i = 0; i < 50; ++i)
        data[i] = static_cast<float>(i * 10);

    test.push_back(data, 50);
    REQUIRE(test.size() == 50);
    for (int i = 0; i < 50; ++i)
        REQUIRE(test[i] == static_cast<float>(i * 10));

    // Push more to trigger growth
    test.push_back(data, 50);
    REQUIRE(test.size() == 100);
    for (int i = 0; i < 50; ++i) {
        REQUIRE(test[i] == static_cast<float>(i * 10));
        REQUIRE(test[i + 50] == static_cast<float>(i * 10));
    }

    // Test growth with wrapped data
    test.clear();
    test.resize_allocation(20);  // Small fixed size
    test.set_dynamic_allocation(true);

    // Fill and pop to create wrap condition
    for (int i = 0; i < 15; ++i)
        test.push_back(static_cast<float>(i));
    test.pop_front(10);  // Now front is at index 10, size is 5
    REQUIRE(test.size() == 5);

    // Push enough to wrap around and then trigger growth
    for (int i = 0; i < 30; ++i)
        test.push_back(static_cast<float>(100 + i));
    REQUIRE(test.size() == 35);
    REQUIRE(test.capacity() >= 35);

    // Verify data integrity (original 5 elements + 30 new)
    for (int i = 0; i < 5; ++i)
        REQUIRE(test[i] == static_cast<float>(10 + i));
    for (int i = 0; i < 30; ++i)
        REQUIRE(test[5 + i] == static_cast<float>(100 + i));

    // Test disabling dynamic allocation
    test.set_dynamic_allocation(false);
    REQUIRE(test.dynamic_allocation() == false);
}

TEST_CASE("ringbuffer_dynamic_allocation_m_end_invariant") {
    // Test that m_end < m_size_max invariant holds after growth
    // Scenario: buffer is full with wrapped data, trigger growth that
    // exactly fills the new space, then push one more

    test_t test;
    test.set_dynamic_allocation(true);
    test.resize_allocation(1);

    test.push_back(1.0f);
    test.push_back(1.0f);

    test.push_back(1.0f);
}

TEST_CASE("ringbuffer_shrink_to_fit") {
    test_t test;
    test.resize_allocation(100);
    REQUIRE(test.capacity() == 100);

    // Shrink empty buffer -> minimum size 1
    test.shrink_to_fit();
    REQUIRE(test.capacity() == 1);

    // Add elements and shrink
    test.resize_allocation(50);
    for (int i = 0; i < 10; ++i)
        test.push_back(static_cast<float>(i));
    REQUIRE(test.size() == 10);
    REQUIRE(test.capacity() == 50);

    test.shrink_to_fit();
    REQUIRE(test.capacity() == 10);
    for (int i = 0; i < 10; ++i)
        REQUIRE(test[i] == static_cast<float>(i));

    // Test with wrapped data
    test.resize_allocation(8);
    for (int i = 0; i < 8; ++i)
        test.push_back(static_cast<float>(i));
    test.pop_front(4);
    for (int i = 0; i < 2; ++i)
        test.push_back(static_cast<float>(100 + i));
    // Now: 6 elements, wrapped

    test.shrink_to_fit();
    REQUIRE(test.capacity() == 6);
    REQUIRE(test.size() == 6);
    REQUIRE(test[0] == 4.0f);
    REQUIRE(test[5] == 101.0f);
}

TEST_CASE("ringbuffer_copy_to_contiguous") {
    test_t test;
    test.resize_allocation(10);

    // Test empty buffer
    float out[10];
    test.copy_to_contiguous(out);  // Should do nothing, no crash

    // Test contiguous data
    for (int i = 0; i < 5; ++i)
        test.push_back(static_cast<float>(i));
    test.copy_to_contiguous(out);
    for (int i = 0; i < 5; ++i)
        REQUIRE(out[i] == static_cast<float>(i));

    // Test wrapped data
    test.clear();
    for (int i = 0; i < 10; ++i)
        test.push_back(static_cast<float>(i));
    test.pop_front(6);
    for (int i = 0; i < 4; ++i)
        test.push_back(static_cast<float>(100 + i));
    // Now: 8 elements, wrapped [6,7,8,9,100,101,102,103]

    REQUIRE(test.size() == 8);
    test.copy_to_contiguous(out);
    REQUIRE(out[0] == 6.0f);
    REQUIRE(out[1] == 7.0f);
    REQUIRE(out[2] == 8.0f);
    REQUIRE(out[3] == 9.0f);
    REQUIRE(out[4] == 100.0f);
    REQUIRE(out[7] == 103.0f);
}

TEST_CASE("ringbuffer_dynamic_allocation_branch_coverage") {
    // Test grow_allocation_nolock while loop (large capacity jump)
    test_t test;
    test.set_dynamic_allocation(true);

    // Start from m_size_max=0, push 100 elements forces multiple doublings
    // 0 -> 16 -> 32 -> 64 -> 128 (while loop runs multiple times)
    float data[100];
    for (int i = 0; i < 100; ++i)
        data[i] = static_cast<float>(i);
    test.push_back(data, 100);
    REQUIRE(test.size() == 100);
    REQUIRE(test.capacity() >= 100);

    // Test grow with wrapped data
    test.clear();
    test.resize_allocation(8);
    test.set_dynamic_allocation(true);
    for (int i = 0; i < 8; ++i)
        test.push_back(static_cast<float>(i));
    test.pop_front(6);  // m_front=6, size=2, data wraps
    for (int i = 0; i < 4; ++i)
        test.push_back(static_cast<float>(10 + i));
    // Now: 6 elements wrapped, trigger growth
    test.push_back(data, 10);
    REQUIRE(test.size() == 16);
    REQUIRE(test[0] == 6.0f);
    REQUIRE(test[1] == 7.0f);
}

TEST_CASE("ringbuffer_shrink_to_fit_branch_coverage") {
    test_t test;

    // Test early return when already minimal (size=1, capacity=1)
    test.resize_allocation(1);
    test.shrink_to_fit();  // Already minimal
    REQUIRE(test.capacity() == 1);

    // Test shrink when buffer is full (m_end wraps to 0)
    test.resize_allocation(4);
    for (int i = 0; i < 4; ++i)
        test.push_back(static_cast<float>(i));
    REQUIRE(test.size() == 4);
    test.shrink_to_fit();  // Already at size, m_end should wrap
    REQUIRE(test.capacity() == 4);
    REQUIRE(test.size() == 4);
    // Verify data intact after shrink
    for (int i = 0; i < 4; ++i)
        REQUIRE(test[i] == static_cast<float>(i));
}

TEST_CASE("ringbuffer_back_wrap_coverage") {
    // Test back() and back_data_index() when m_end == 0 (wraps to size_max-1)
    test_t test;
    test.resize_allocation(4);

    // Fill completely so m_end wraps to 0
    for (int i = 0; i < 4; ++i)
        test.push_back(static_cast<float>(i));
    // Now m_end == 0, so back should be at index 3

    REQUIRE(test.back() == 3.0f);
    REQUIRE(test.back_data_index() == 3);
}

TEST_CASE("ringbuffer_push_front_value_wrap_coverage") {
    // Test push_front(value, nb_values) with wrapping (sliced branch)
    test_t test;
    test.resize_allocation(8);

    // Push some elements then pop to move m_front forward
    for (int i = 0; i < 6; ++i)
        test.push_back(static_cast<float>(i));
    test.pop_front(4);  // m_front = 4, size = 2
    // Elements at indices 4, 5

    // Push front with enough values to wrap around
    test.push_front(99.0f, 5);  // Should wrap: needs indices 4-1=3,2,1,0 and 7
    REQUIRE(test.size() == 7);
    for (int i = 0; i < 5; ++i)
        REQUIRE(test[i] == 99.0f);
    REQUIRE(test[5] == 4.0f);
    REQUIRE(test[6] == 5.0f);
}

TEST_CASE("ringbuffer_push_front_array_wrap_coverage") {
    // Test push_front(array, size) with wrapping (sliced branch)
    test_t test;
    test.resize_allocation(8);

    // Push some elements then pop to move m_front forward
    for (int i = 0; i < 6; ++i)
        test.push_back(static_cast<float>(i));
    test.pop_front(4);  // m_front = 4, size = 2

    // Push front array that wraps around
    float data[] = {10.0f, 20.0f, 30.0f, 40.0f, 50.0f};
    test.push_front(data, 5);
    REQUIRE(test.size() == 7);
    REQUIRE(test[0] == 10.0f);
    REQUIRE(test[4] == 50.0f);
    REQUIRE(test[5] == 4.0f);
    REQUIRE(test[6] == 5.0f);
}

TEST_CASE("ringbuffer_push_back_segment_wrap_coverage") {
    // Test push_back(rb, start, size) with various wrapping scenarios
    test_t src, dst;

    // Scenario 1: source wrapped, destination continuous
    src.resize_allocation(8);
    for (int i = 0; i < 8; ++i)
        src.push_back(static_cast<float>(i));
    src.pop_front(5);  // m_front = 5, size = 3 (elements 5,6,7)
    for (int i = 0; i < 4; ++i)
        src.push_back(static_cast<float>(10 + i));  // elements 5,6,7,10,11,12,13 (wraps)

    dst.resize_allocation(16);
    dst.push_back(src, 0, 7);  // Copy all from wrapped source
    REQUIRE(dst.size() == 7);
    REQUIRE(dst[0] == 5.0f);
    REQUIRE(dst[2] == 7.0f);
    REQUIRE(dst[3] == 10.0f);
    REQUIRE(dst[6] == 13.0f);

    // Scenario 2: destination wraps
    dst.clear();
    dst.resize_allocation(12);
    for (int i = 0; i < 8; ++i)
        dst.push_back(static_cast<float>(i));
    dst.pop_front(2);  // m_front = 2, size = 6, m_end = 8
    // Now push enough to wrap destination

    test_t src2;
    src2.resize_allocation(8);
    for (int i = 0; i < 5; ++i)
        src2.push_back(static_cast<float>(100 + i));

    dst.push_back(src2, 0, 5);  // destination wraps
    REQUIRE(dst.size() == 11);
    REQUIRE(dst[6] == 100.0f);
    REQUIRE(dst[10] == 104.0f);
}

TEST_CASE("ringbuffer_push_back_segment_complex_wrap") {
    // Test the complex 3-segment scenarios in push_back(rb, start, size)
    test_t src, dst;

    // Both source and destination wrap - source break before dest max
    src.resize_allocation(8);
    for (int i = 0; i < 8; ++i)
        src.push_back(static_cast<float>(i));
    src.pop_front(6);  // m_front = 6, size = 2
    for (int i = 0; i < 5; ++i)
        src.push_back(static_cast<float>(10 + i));  // 7 elements, wrapped

    dst.resize_allocation(16);  // Larger to fit all elements
    for (int i = 0; i < 10; ++i)
        dst.push_back(static_cast<float>(100 + i));
    dst.pop_front(2);  // m_front = 2, size = 8, m_end = 10
    // Now push to make destination wrap with wrapped source

    dst.push_back(src, 0, 7);
    REQUIRE(dst.size() == 15);

    // Verify integrity (first 8 from dst, then 7 from src)
    REQUIRE(dst[0] == 102.0f);
    REQUIRE(dst[7] == 109.0f);
    REQUIRE(dst[8] == 6.0f);
    REQUIRE(dst[9] == 7.0f);
    REQUIRE(dst[10] == 10.0f);
}

TEST_CASE("ringbuffer_push_back_segment_edge_cases") {
    test_t src, dst;

    // Test line 613: start+size > rb.size() (truncation)
    src.resize_allocation(8);
    for (int i = 0; i < 5; ++i)
        src.push_back(static_cast<float>(i));

    dst.resize_allocation(16);
    dst.push_back(src, 2, 100);  // Request more than available, should truncate to 3
    REQUIRE(dst.size() == 3);
    REQUIRE(dst[0] == 2.0f);
    REQUIRE(dst[2] == 4.0f);

    // Test line 618: rb_front wraps (start is beyond wrap point in wrapped source)
    src.clear();
    src.resize_allocation(8);
    for (int i = 0; i < 8; ++i)
        src.push_back(static_cast<float>(i));
    src.pop_front(6);  // m_front = 6, size = 2 (elements 6,7)
    for (int i = 0; i < 4; ++i)
        src.push_back(static_cast<float>(10 + i));  // size = 6, wrapped
    // src contains: [6, 7, 10, 11, 12, 13] with m_front=6, wraps at 8

    dst.clear();
    dst.resize_allocation(16);
    dst.push_back(src, 3, 3);  // Start at index 3 (element 11), wraps rb_front
    REQUIRE(dst.size() == 3);
    REQUIRE(dst[0] == 11.0f);
    REQUIRE(dst[1] == 12.0f);
    REQUIRE(dst[2] == 13.0f);
}

TEST_CASE("ringbuffer_push_back_segment_source_break_after_dest") {
    // Test lines 687-696: source break comes AFTER destination max size
    // Need:
    //   1. m_end+rb_size > m_size_max (dest wraps)
    //   2. rb_front+rb_size > rb.m_size_max (source wraps)
    //   3. (rb.size_max()-rb_front) >= (m_size_max-m_end) (source break >= dest break)
    test_t src, dst;

    // Create source with late wrap point
    src.resize_allocation(8);
    for (int i = 0; i < 8; ++i)
        src.push_back(static_cast<float>(i));
    src.pop_front(5);  // m_front = 5, size = 3
    for (int i = 0; i < 4; ++i)
        src.push_back(static_cast<float>(100 + i));
    // src: m_front=5, size=7, data=[5,6,7,100,101,102,103]
    // src.size_max - rb_front = 8 - 5 = 3 elements before wrap

    // Create destination with early wrap point
    dst.resize_allocation(8);
    for (int i = 0; i < 7; ++i)
        dst.push_back(static_cast<float>(200 + i));
    dst.pop_front(5);  // m_front = 5, size = 2, m_end = 7
    // dst: m_size_max - m_end = 8 - 7 = 1 element before wrap

    // src break = 3, dst break = 1: 3 >= 1, so lines 687-696
    // Push 5 elements: rb_front=5, rb_size=5, 5+5=10 > 8, source wraps
    // m_end=7, rb_size=5, 7+5=12 > 8, dest wraps
    dst.push_back(src, 0, 5);
    REQUIRE(dst.size() == 7);
    REQUIRE(dst[2] == 5.0f);
    REQUIRE(dst[6] == 101.0f);
}

TEST_CASE("ringbuffer_push_back_segment_empty_cases") {
    // Test lines 608-610: early returns
    test_t src, dst;

    dst.resize_allocation(16);
    dst.push_back(1.0f);

    // Line 608: rb.size() == 0
    src.resize_allocation(8);  // Empty source
    dst.push_back(src, 0, 5);
    REQUIRE(dst.size() == 1);  // No change

    // Line 609: size == 0
    src.push_back(1.0f);
    dst.push_back(src, 0, 0);
    REQUIRE(dst.size() == 1);  // No change

    // Line 610: start >= rb.size()
    dst.push_back(src, 10, 5);  // start=10 > size=1
    REQUIRE(dst.size() == 1);  // No change
}

TEST_CASE("ringbuffer_push_back_segment_m_end_wrap") {
    // Test line 702: m_end wrapping after segment push
    test_t src, dst;

    src.resize_allocation(8);
    for (int i = 0; i < 4; ++i)
        src.push_back(static_cast<float>(i));

    // Setup destination to wrap m_end after push
    dst.resize_allocation(8);
    for (int i = 0; i < 6; ++i)
        dst.push_back(static_cast<float>(100 + i));
    dst.pop_front(2);  // m_front=2, size=4, m_end=6

    // Push 4 elements: m_end=6+4=10, 10 >= 8, so m_end -= 8 = 2
    dst.push_back(src, 0, 4);
    REQUIRE(dst.size() == 8);
    REQUIRE(dst[0] == 102.0f);
    REQUIRE(dst[7] == 3.0f);
}

TEST_CASE("ringbuffer_pop_back_wrap_coverage") {
    // Test pop_back() when m_end == 0 (triggers wrap to m_size_max-1)
    test_t test;
    test.resize_allocation(4);

    // Fill completely so m_end == 0
    for (int i = 0; i < 4; ++i)
        test.push_back(static_cast<float>(i));
    REQUIRE(test.size() == 4);

    // Pop back - m_end is 0, should wrap to 3
    float val = test.pop_back();
    REQUIRE(val == 3.0f);
    REQUIRE(test.size() == 3);

    // Pop again
    val = test.pop_back();
    REQUIRE(val == 2.0f);
    REQUIRE(test.size() == 2);
}
