// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#ifndef ACBENCH_METHODS_H_
#define ACBENCH_METHODS_H_

// Portaudio
#include <portaudio.h>
#include <pa_ringbuffer.h>

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <random>

#include <iostream>
#include <fstream>

#include <deque>

// Boost
#include <boost/circular_buffer.hpp>

// RubberBand
#include "../ext/rubberband/single/RubberBandSingle.cpp"

// Jack
#include <jack/ringbuffer.h>

// ACBench
#include <acbench/ringbuffer.h>

#include <acbench/time_elapsed.h>


class Method {
 public:
    std::string m_name;
    int m_max_size = 0;
    int m_nb_repeat = 100;
    acbench::time_elapsed m_elapsed;

    explicit Method(const std::string& name, int max_size, int nb_repeat)
        : m_name(name)
        , m_max_size(max_size)
        , m_nb_repeat(nb_repeat) {
    }
    virtual ~Method() {
    }

    void write_file(const std::string& tag) const {
        std::string file_path = m_name+"_"+tag+"_elapsed.bin";
        std::ofstream fh(file_path, std::ios_base::binary);
        for (int n=0; n<m_elapsed.size(); ++n) {
            float value = m_elapsed.elapsed()[n]/m_nb_repeat;
            fh.write((char*)&value, sizeof(value));
        }
        fh.close();
    }

    virtual void clear() = 0;

    virtual void run_push_back_array(float* chunk, int chunk_size) = 0;
    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) = 0;
    virtual void run_push_back_const(float value, int chunk_size) = 0;

    virtual bool compare(const std::deque<float>& arr_ref) = 0;
};

// Fake ringbuffer that doesn't store the data
template<typename T>
class fastestbound_ringbuffer {
    int m_size = 0;
    float m_dummy_value = -1.0f;

 public:
    void clear() {
        m_size = 0;
    }

    typedef T value_type;
    inline void push_back(float v) {
        ++m_size;
    }
    inline void push_back(const value_type* array, int size) {
        (void)array;
        m_size += size;
    }
    inline void push_back(value_type value, int size) {
        (void)value;
        m_size += size;
    }
    inline void pop_front() {
        --m_size;
    }
    inline void pop_front(int n) {
        m_size -= n;
    }
    inline void pop_front(value_type* array, int n) {
        m_size -= n;
        if (m_size < 0)
            m_size = 0;
    }

    inline int size() const {
        return m_size;
    }
    inline float operator[](int n) const {
        (void)n;
        return m_dummy_value;
    }
    inline float& operator[](int n) {
        (void)n;
        return m_dummy_value;
    }
};

class MethodFastestBound : public Method {
 public:
    explicit MethodFastestBound(int max_size, int nb_repeat) : Method("FastestBound", max_size, nb_repeat) {}

    fastestbound_ringbuffer<float> m_buffer;

    void clear() {
        m_buffer.clear();
    }

    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (m_buffer.size()+chunk_size > m_max_size)
                m_buffer.pop_front(chunk_size);
            m_buffer.push_back(chunk, chunk_size);
        }
        m_elapsed.end(0.0);
    }

    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        while (static_cast<int>(m_buffer.size())+size_push <= m_max_size) {
            m_buffer.push_back(chunk_push, size_push);
        }
        while (static_cast<int>(m_buffer.size()) >= size_pull) {
            m_buffer.pop_front(chunk_pull, size_pull);
        }
        m_elapsed.end(0.0);
    }

    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (m_buffer.size()+chunk_size > m_max_size)
                m_buffer.pop_front(chunk_size);
            m_buffer.push_back(value, chunk_size);
        }
        m_elapsed.end(0.0);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        return true;  // Fake it
    }
};

class MethodSTL : public Method {
 public:
    explicit MethodSTL(int max_size, int nb_repeat) : Method("STL", max_size, nb_repeat) {}

    std::deque<float> m_buffer;

    void clear() {
        m_buffer.clear();
    }

    /* Scenario: push_back_array
     * 1. If the buffer is full, pop_front until there is enough space for chunk_size.
     * 2. Push back the given chunk.
     */
    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (static_cast<int>(m_buffer.size())+chunk_size > m_max_size) {
                for (int c = 0; (c < chunk_size) && (m_buffer.size() > 0); ++c)
                    m_buffer.pop_front();
            }
            for (int c=0; c < chunk_size; ++c)
                m_buffer.push_back(chunk[c]);
       }
        m_elapsed.end(0.0f);
    }

    /* Scenario: push_pull_array
     * 1. Push as many chunk as possible.
     * 2. Pull as many chunk as possible.
     */
    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            while (static_cast<int>(m_buffer.size())+size_push <= m_max_size) {
                for (int c=0; c < size_push; ++c)
                    m_buffer.push_back(chunk_push[c]);
            }
            while (static_cast<int>(m_buffer.size()) >= size_pull) {
                for (int c=0; c < size_pull; ++c) {
                    chunk_pull[c] = m_buffer.front();
                    m_buffer.pop_front();
                }
            }
       }
        m_elapsed.end(0.0f);
    }

    /* Scenario: push_back_const
     * 1. If the buffer is full, pop_front until there is enough space for chunk_size.
     * 2. Push back the given constant value.
     */
    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (static_cast<int>(m_buffer.size())+chunk_size > m_max_size) {
                for (int c = 0; (c < chunk_size) && (m_buffer.size() > 0); ++c)
                    m_buffer.pop_front();
            }
            for (int c=0; c < chunk_size; ++c)
                m_buffer.push_back(value);
       }
        m_elapsed.end(0.0f);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        return acbench::compare(arr_ref, m_buffer);
    }
};

class MethodBoost : public Method {
 public:
    explicit MethodBoost(int max_size, int nb_repeat)
        : Method("Boost", max_size, nb_repeat) {
        m_buffer.set_capacity(max_size);
    }

    boost::circular_buffer<float> m_buffer;

    void clear() {
        m_buffer.clear();
    }

    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (static_cast<int>(m_buffer.size())+chunk_size > m_max_size) {
                for (int c = 0; (c < chunk_size) && (m_buffer.size() > 0); ++c)
                    m_buffer.pop_front();
            }
            for (int c=0; c < chunk_size; ++c)
                m_buffer.push_back(chunk[c]);
        }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            while (static_cast<int>(m_buffer.size())+size_push <= m_max_size) {
                for (int c=0; c < size_push; ++c)
                    m_buffer.push_back(chunk_push[c]);
            }
            while (static_cast<int>(m_buffer.size()) >= size_pull) {
                for (int c=0; c < size_pull; ++c) {
                    chunk_pull[c] = m_buffer.front();
                    m_buffer.pop_front();
                }
            }
       }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (static_cast<int>(m_buffer.size())+chunk_size > m_max_size) {
                for (int c = 0; (c < chunk_size) && (m_buffer.size() > 0); ++c)
                    m_buffer.pop_front();
            }
            for (int c=0; c < chunk_size; ++c)
                m_buffer.push_back(value);
        }
        m_elapsed.end(0.0f);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        return acbench::compare(arr_ref, m_buffer);
    }
};

class MethodPortaudio : public Method {
 public:
    explicit MethodPortaudio(int max_size, int nb_repeat)
        : Method("Portaudio", max_size, nb_repeat) {
        // TODO(GD) Needs to be power of 2 for Portaudio
        pa_data = new float[max_size];
        if(std::log2(max_size) != std::floor(std::log2(max_size))) {
            DOUT << "ERROR: max_size (=" << max_size << ") needs to be power of 2 for Portaudio." << std::endl;
            std::exit(1);
        }
        PaUtil_InitializeRingBuffer(&pa_buffer, sizeof(float), max_size, pa_data);
    }

    float* pa_data = nullptr;
    PaUtilRingBuffer pa_buffer;

    virtual ~MethodPortaudio() {
        delete[] pa_data;
    }

    void clear() {
        PaUtil_FlushRingBuffer(&pa_buffer);
    }

    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            ring_buffer_size_t pa_size = PaUtil_GetRingBufferReadAvailable(&pa_buffer);
            if (pa_size+chunk_size > m_max_size) {
                PaUtil_AdvanceRingBufferReadIndex(&pa_buffer, std::min<ring_buffer_size_t>(pa_size, chunk_size));
            }
            PaUtil_WriteRingBuffer(&pa_buffer, reinterpret_cast<void*>(chunk), chunk_size);
        }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            ring_buffer_size_t pa_size = PaUtil_GetRingBufferWriteAvailable(&pa_buffer);
            while (size_push <= pa_size) {
                PaUtil_WriteRingBuffer(&pa_buffer, reinterpret_cast<void*>(chunk_push), size_push);
                pa_size = PaUtil_GetRingBufferWriteAvailable(&pa_buffer);
            }
            pa_size = PaUtil_GetRingBufferReadAvailable(&pa_buffer);
            while (pa_size >= size_pull) {
                PaUtil_ReadRingBuffer(&pa_buffer, reinterpret_cast<void*>(chunk_pull), size_pull);
                pa_size = PaUtil_GetRingBufferReadAvailable(&pa_buffer);
            }
       }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            ring_buffer_size_t pa_size = PaUtil_GetRingBufferReadAvailable(&pa_buffer);
            if (pa_size+chunk_size > m_max_size) {
                PaUtil_AdvanceRingBufferReadIndex(&pa_buffer, std::min<ring_buffer_size_t>(pa_size, chunk_size));
            }
            for (int i = 0; i < chunk_size; ++i) {
                PaUtil_WriteRingBuffer(&pa_buffer, reinterpret_cast<void*>(&value), 1);  // TODO unfair
            }
        }
        m_elapsed.end(0.0f);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        int pa_size = PaUtil_GetRingBufferReadAvailable(&pa_buffer);
        float* tmp = new float[pa_size];
        PaUtil_ReadRingBuffer(&pa_buffer, reinterpret_cast<void*>(tmp), pa_size);
        std::vector<float> vtmp(pa_size);
        for (int n = 0; n < pa_size; ++n)
            vtmp[n] = tmp[n];
        bool ret = acbench::compare(arr_ref, vtmp);
        delete[] tmp;
        return ret;
    }
};

class MethodRubberBand : public Method {
 public:
    RubberBand::RingBuffer<float> m_buffer;

    explicit MethodRubberBand(int max_size, int nb_repeat)
        : Method("RubberBand", max_size, nb_repeat)
        , m_buffer(max_size)
    {}

    void clear() {
        m_buffer.reset();
    }

    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (m_buffer.getReadSpace()+chunk_size > m_max_size) {
                m_buffer.skip(std::min(m_buffer.getReadSpace(), chunk_size));
            }
            m_buffer.write(chunk, chunk_size);
        }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            while (size_push <= m_buffer.getWriteSpace()) {
                m_buffer.write(chunk_push, size_push);
            }
            while (m_buffer.getReadSpace() >= size_pull) {
                m_buffer.read(chunk_pull, size_pull);
            }
       }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (m_buffer.getReadSpace()+chunk_size > m_max_size) {
                m_buffer.skip(std::min(m_buffer.getReadSpace(), chunk_size));
            }
            for (int i = 0; i < chunk_size; ++i) {
                m_buffer.write(&value, 1);
            }
        }
        m_elapsed.end(0.0f);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        // TODO(GD) Simplify
        float* tmp = new float[m_buffer.getReadSpace()];
        m_buffer.peek<float>(tmp, m_buffer.getReadSpace());
        std::vector<float> vtmp(m_buffer.getReadSpace());
        for (int n = 0; n < m_buffer.getReadSpace(); ++n)
            vtmp[n] = tmp[n];
        bool ret = acbench::compare(arr_ref, vtmp);
        delete[] tmp;
        return ret;
    }
};

class MethodJack : public Method {
 public:
    explicit MethodJack(int max_size, int nb_repeat)
        : Method("Jack", max_size, nb_repeat) {
        m_buffer = jack_ringbuffer_create(max_size*sizeof(float)+1);  // TODO(GD) +1 !
    }

    jack_ringbuffer_t* m_buffer = nullptr;

    virtual ~MethodJack() {
        jack_ringbuffer_free(m_buffer);
    }

    void clear() {
        jack_ringbuffer_reset(m_buffer);
    }

    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            int size = jack_ringbuffer_read_space(m_buffer)/sizeof(float);
            if (size+chunk_size > m_max_size) {
                jack_ringbuffer_read_advance(m_buffer, std::min<ring_buffer_size_t>(size, chunk_size)*sizeof(float));
            }
            jack_ringbuffer_write(m_buffer, reinterpret_cast<char*>(chunk), chunk_size*sizeof(float));
        }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            int size = jack_ringbuffer_write_space(m_buffer)/sizeof(float);
            while (size_push <= size) {
                jack_ringbuffer_write(m_buffer, reinterpret_cast<char*>(chunk_push), size_push*sizeof(float));
                size = jack_ringbuffer_write_space(m_buffer)/sizeof(float);
            }
            size = jack_ringbuffer_read_space(m_buffer)/sizeof(float);
            while (size >= size_pull) {
                jack_ringbuffer_read(m_buffer, reinterpret_cast<char*>(chunk_pull), size_pull*sizeof(float));
                size = jack_ringbuffer_read_space(m_buffer)/sizeof(float);
            }
       }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            int size = jack_ringbuffer_read_space(m_buffer)/sizeof(float);
            if (size+chunk_size > m_max_size) {
                jack_ringbuffer_read_advance(m_buffer, std::min<ring_buffer_size_t>(size, chunk_size)*sizeof(float));
            }
            for (int i = 0; i < chunk_size; ++i) {
                jack_ringbuffer_write(m_buffer, reinterpret_cast<char*>(&value), 1*sizeof(float));
            }
        }
        m_elapsed.end(0.0f);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        int size = jack_ringbuffer_read_space(m_buffer)/sizeof(float);
        float* tmp = new float[size];
        jack_ringbuffer_read(m_buffer, reinterpret_cast<char*>(tmp), size*sizeof(float));
        std::vector<float> vtmp(size);
        for (int n = 0; n < size; ++n)
            vtmp[n] = tmp[n];
        bool ret = acbench::compare(arr_ref, vtmp);
        delete[] tmp;
        return ret;
    }
};

class MethodACBench : public Method {
 public:
    acbench::ringbuffer<float> m_buffer;

    explicit MethodACBench(int max_size, int nb_repeat)
        : Method("ACBench", max_size, nb_repeat) {
        m_buffer.resize_allocation(max_size);
    }

    void clear() {
        m_buffer.clear();
    }

    virtual void run_push_back_array(float* chunk, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (m_buffer.size()+chunk_size > m_max_size)
                m_buffer.pop_front(chunk_size);
            m_buffer.push_back(chunk, chunk_size);
        }
        m_elapsed.end(0.0f);
    }

    virtual void run_push_pull_array(float* chunk_push, int size_push, float* chunk_pull, int size_pull) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            while (m_buffer.size()+size_push <= m_max_size) {
                m_buffer.push_back(chunk_push, size_push);
            }
            while (m_buffer.size() >= size_pull) {
                m_buffer.pop_front(chunk_pull, size_pull);
            }
        }
        m_elapsed.end(0.0);
    }

    virtual void run_push_back_const(float value, int chunk_size) {
        m_elapsed.start();
        for (int n = 0; n < m_nb_repeat; ++n) {
            if (m_buffer.size()+chunk_size > m_max_size)
                m_buffer.pop_front(chunk_size);
            m_buffer.push_back(value, chunk_size);
        }
        m_elapsed.end(0.0f);
    }

    virtual bool compare(const std::deque<float>& arr_ref) {
        return acbench::compare(arr_ref, m_buffer);
    }
};

#endif  // ACBENCH_METHODS_H_
