// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#ifndef ACBENCH_RINGBUFFER_H_
#define ACBENCH_RINGBUFFER_H_

#include <cstring>  // For std::memcpy(.)
#include <cassert>  // For assert(.)

// Defining ACBENCH_MULTITHREADED will make all acbench::ringbuffer thread-safe
//     Public member functions are thread-safe, but private and protected ones are not.
#ifdef ACBENCH_MULTITHREADED
#include <mutex>
#define ACBENCH_MUTEX_DECLARE mutable std::mutex m_mutex;  // mutable allows to change even in const methods
#define ACBENCH_MUTEX_GUARD std::lock_guard<std::mutex> mutex_lock(m_mutex);
#define ACBENCH_MUTEX_LOCK m_mutex.lock();
#define ACBENCH_MUTEX_IFLOCK if(lock){m_mutex.lock();}
#define ACBENCH_MUTEX_UNLOCK m_mutex.unlock();
#define ACBENCH_MUTEX_IFUNLOCK if(lock){m_mutex.unlock();}
#else
#define ACBENCH_MUTEX_DECLARE
#define ACBENCH_MUTEX_GUARD
#define ACBENCH_MUTEX_LOCK
#define ACBENCH_MUTEX_IFLOCK
#define ACBENCH_MUTEX_UNLOCK
#define ACBENCH_MUTEX_IFUNLOCK
#endif


namespace acbench {

    template<typename T>
    class ringbuffer {
     protected:
        ACBENCH_MUTEX_DECLARE

        int m_size_max = 0;
        int m_size = 0;
        T* m_data = nullptr;
        int m_front = 0;
        int m_back = 0;

        //! WARNING: Not thread-safe
        inline void destroy() {
            if ( m_data ) {
                delete[] m_data;  // GCOVR_EXCL_LINE
                m_data = nullptr;
            }
        }

     public:
        typedef T value_type;

        inline int size_max() const {
            return m_size_max;          // Atomic, no need of locked mutex
        }

     protected:
        // Copy constructor is forbidden to avoid implicit calls.
        // Do it manually if necessary (using `.resize_allocation(.)` and `.push_back(.)`)
        explicit ringbuffer(const ringbuffer<value_type>& rb) {
            (void)rb;
        }

        //! WARNING: Not thread-safe
        inline void memory_copy(value_type* pdest, const value_type* psrc, int size) {
            if (size == 0) return;  // GCOVR_EXCL_LINE
            assert(size > 0);
            std::memcpy(reinterpret_cast<void*>(pdest), reinterpret_cast<const void*>(psrc), sizeof(value_type)*static_cast<unsigned int>(size));
        }

        //! WARNING: Not thread-safe
        inline void memory_check_size(int nb_new_values) {
            (void)nb_new_values;
            assert(nb_new_values > 0);
            assert(m_size+nb_new_values <= m_size_max);
        }

        //! WARNING: Not thread-safe
        inline void clear_without_lock() {
            m_front = 0;
            m_back = 0;
            m_size = 0;
        }

     public:
        ringbuffer() {
        }
        ringbuffer& operator=(const ringbuffer& rb) {
            clear();
            push_back(rb);
            return *this;
        }
        //! Always loose the data
        inline void resize_allocation(int size_max) {
            ACBENCH_MUTEX_GUARD
            if (size_max == m_size_max) {
                clear_without_lock();
                return;
            }
            destroy();

            m_data = new value_type[size_max];  // GCOVR_EXCL_LINE // TODO(GD) Force contiguous memory
            m_size_max = size_max;

            clear_without_lock();
        }
        //! Does keep the allocation
        inline void clear() {
            ACBENCH_MUTEX_GUARD
            clear_without_lock();
        }
        ~ringbuffer() {
            ACBENCH_MUTEX_GUARD
            destroy();
        }

        inline value_type* data() const {
            return m_data;  // Atomic, no need of locked mutex
        }
        inline int size() const {
            return m_size;  // Atomic, no need of locked mutex
        }
        inline int front_index() const {
            return m_front;  // Atomic, no need of locked mutex
        }
        inline int back_index() const {
            return m_back;  // Atomic, no need of locked mutex
        }
        inline int max_size() const {
            return size_max;  // Atomic, no need of locked mutex
        }
        inline bool empty() const {
            return m_size == 0;  // Atomic, no need of locked mutex
        }
        value_type operator[](int n) const {
            ACBENCH_MUTEX_GUARD  // TODO Makes it very slow
            assert(n < m_size);
            return m_data[(m_front+n)%m_size_max];
        }
        value_type& operator[](int n) {
            ACBENCH_MUTEX_GUARD  // TODO Makes it very slow
            assert(n < m_size);
            return m_data[(m_front+n)%m_size_max];
        }
        inline value_type front() const {
            ACBENCH_MUTEX_GUARD  // TODO Makes it very slow
            return m_data[m_front];
        }

        inline void push_back(const value_type v) {
            ACBENCH_MUTEX_GUARD

            memory_check_size(1);

            m_data[m_back] = v;

            ++m_back;
            if (m_back >= m_size_max)
                m_back = 0;

            ++m_size;
        }
        inline void push_back(const value_type value, int nb_values) {
            if (nb_values <= 0)             // Ignore pushing no values
                return;

            ACBENCH_MUTEX_GUARD

            memory_check_size(nb_values);

            if (m_back+nb_values <= m_size_max) {

                value_type* pdata = m_data+m_back;
                for (int k=0; k < nb_values; ++k)
                    *pdata++ = value;
                m_back += nb_values;

            } else {
                // Need to slice the array into two segments

                // 1st segment: m_back:m_size_max-1
                int seg1size = m_size_max - m_back;
                value_type* pdata = m_data+m_back;
                for (int k=0; k < seg1size; ++k)
                    *pdata++ = value;

                // 2nd segment: 0:nb_values-seg1size
                int seg2size = nb_values - seg1size;
                pdata = m_data;
                for (int k=0; k < seg2size; ++k)
                    *pdata++ = value;

                m_back = seg2size;
            }

            m_size += nb_values;
        }
        inline void push_back(const value_type* array, int array_size) {
            if (array_size <= 0)             // Ignore push of empty buffers
                return;

            ACBENCH_MUTEX_GUARD

            memory_check_size(array_size);

            if (m_back+array_size <= m_size_max) {
                // No need to slice it
                memory_copy(m_data+m_back, array, array_size);
                m_back += array_size;

            } else {
                // Need to slice the array into two segments

                // 1st segment: m_back:m_size_max-1
                int seg1size = m_size_max - m_back;
                memory_copy(m_data+m_back, array, seg1size);

                // 2nd segment: 0:array_size-seg1size
                int seg2size = array_size - seg1size;
                memory_copy(m_data, array+seg1size, seg2size);

                m_back = seg2size;
            }

            m_size += array_size;
        }

        inline void push_back(const ringbuffer<value_type>& rb) {
            if (rb.size() == 0)          // Ignore push of empty ringbuffers
                return;

            ACBENCH_MUTEX_GUARD

            memory_check_size(rb.size());

            if (m_back+rb.m_size <= m_size_max) {
                // The destination segment is continuous

                // Now let's see the source segment(s)
                if (rb.m_front+rb.m_size <= rb.m_size_max) {
                    // The source segment is continuous...
                    // ... easiest game of my life:
                    memory_copy(m_data+m_back, rb.m_data+rb.m_front, rb.m_size);

                } else {
                    // The source segment is made of two continuous segments

                    // 1st segment
                    int seg1size = rb.m_size_max - rb.m_front;
                    memory_copy(m_data+m_back, rb.m_data+rb.m_front, seg1size);

                    // 2nd segment
                    int seg2size = rb.m_size - seg1size;
                    memory_copy(m_data+m_back+seg1size, rb.m_data, seg2size);
                }

                m_back += rb.size();

            } else {
                // The destination segment is made of two continuous segments

                if (rb.m_front+rb.m_size <= rb.m_size_max) {
                    // The source segment is continuous...

                    // 1st segment
                    int seg1size = m_size_max - m_back;
                    memory_copy(m_data+m_back, rb.m_data+rb.m_front, seg1size);

                    // 2nd segment
                    int seg2size = rb.m_size - seg1size;
                    memory_copy(m_data, rb.m_data+rb.m_front+seg1size, seg2size);

                    m_back = seg2size;

                } else {
                    // The source segment is also made of two continuous segments...
                    // ... worst game of my life: 3 segments to handle.

                    // Let's check if the source's break point comes before or after the destination's max size.
                    if ((rb.m_size_max-rb.m_front) < (m_size_max-m_back)) {
                        // the source's break point comes before the destination's max size...

                        // 1st segment
                        int seg1size = rb.m_size_max - rb.m_front;
                        memory_copy(m_data+m_back, rb.m_data+rb.m_front, seg1size);

                        // 2nd segment
                        int seg2size = (m_size_max-m_back) - seg1size;
                        memory_copy(m_data+m_back+seg1size, rb.m_data, seg2size);

                        // 3rd segment
                        int seg3size = rb.m_size - seg1size - seg2size;
                        memory_copy(m_data, rb.m_data+seg2size, seg3size);

                        m_back = seg3size;

                    } else {
                        // the source's break point comes after the destination's max size...

                        // 1st segment
                        int seg1size = m_size_max - m_back;
                        memory_copy(m_data+m_back, rb.m_data+rb.m_front, seg1size);

                        // 2nd segment
                        int seg2size = (rb.m_size_max-rb.m_front) - seg1size;
                        memory_copy(m_data, rb.m_data+rb.m_front+seg1size, seg2size);

                        // 3rd segment
                        int seg3size = rb.m_size - seg1size - seg2size;
                        memory_copy(m_data+seg2size, rb.m_data, seg3size);

                        m_back = seg2size + seg3size;
                    }
                }
            }

            m_size += rb.m_size;
        }

        inline value_type pop_front() {
            assert(m_size >= 1);

            ACBENCH_MUTEX_GUARD

            value_type value = m_data[m_front];

            ++m_front;

            if (m_front >= m_size_max)
                m_front = 0;

            --m_size;

            return value;
        }
        inline void pop_front(int n) {
            if (n < 1) return;                // Just ignore pops of non-existing values

            ACBENCH_MUTEX_GUARD

            if (n >= m_size) {                // Clears all if not enough to be poped
                clear_without_lock();
                return;
            }

            m_front += n;
            if (m_front >= m_size_max)
                m_front -= m_size_max;

            m_size -= n;
        }

        inline int pop_front(value_type* array, int n) {
            if (n < 1) return 0;              // Just ignore pops of non-existing values

            ACBENCH_MUTEX_GUARD

            if (n > m_size)                   // Pop as many values as possible
                n = m_size;

            if (m_front+n <= m_size_max) {
                // No need to slice it
                memory_copy(array, m_data+m_front, n);
                m_front += n;

            } else {
                // Need to slice the array into two segments

                // 1st segment: m_front:m_size_max-1
                int seg1size = m_size_max - m_front;
                memory_copy(array, m_data+m_front, seg1size);

                // 2nd segment: 0:n-seg1size
                int seg2size = n - seg1size;
                memory_copy(array+seg1size, m_data, seg2size);

                m_front = seg2size;
            }

            m_size -= n;

            return n;
        }

        // TODO TODO TODO To test
        inline value_type pop_back() {
            assert(m_size >= 1);

            ACBENCH_MUTEX_GUARD

            int back = m_back - 1;
            if (back < 0)
                back = m_size_max-1;
            value_type value = m_data[back];

            --m_back;

            if (m_back < 0)
                m_back = m_size_max-1;

            --m_size;

            return value;
        }
        // TODO TODO TODO To test
        inline void pop_back(int n) {
            if (n < 1) return;                // Just ignore pops of non-existing values

            ACBENCH_MUTEX_GUARD

            if (n >= m_size) {                // Clears all if not enough to be poped
                clear_without_lock();
                return;
            }

            m_back -= n;
            if (m_back < 0)
                m_back += m_size_max;

            m_size -= n;
        }

    };

}  // namespace acbench

#endif  // ACBENCH_RINGBUFFER_H_
