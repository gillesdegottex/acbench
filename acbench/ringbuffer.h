// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#ifndef ACBENCH_RINGBUFFER_H_
#define ACBENCH_RINGBUFFER_H_

/**

Allocation:
    Only 2 functions can allocate memory:
        resize_allocation(.) which always allocates a new memory block and clear any previous data.
        reserve(.) which allocates only if the new size is greater than the current one, and preserves the previous data.

    The destructor always deallocate the memory.

Thread-safety:
    * By default, the functions are thread-safe.
    * WARNING: Except for element-wise accessors (ex. operator[](int)), which are _not_ thread-safe.
      Those must be called within a .lock() and .unlock() block to be thread-safe.
      It is done so bcs [] operator is usually used in a loop, and locking the mutex would be too expensive.
    * Public member functions are thread-safe, but protected ones are not.
    * You can always call the _nolock(.) non-thread-safe version of each function.
      (if it doesn't exists, it means there was no need to lock the mutex in the thread-safe version anyway)

    * On systems that don't have mutex or are single-threaded by nature (ex. Arduino), you can make the whole ringbuffer not thread-safe by defining ACBENCH_NOT_THREAD_SAFE before including this file.

**/

#ifndef ACBENCH_NOT_THREAD_SAFE
    #define ACBENCH_MULTITHREADED  // Move before the #includes or make it an option per ringbuffer
#endif


#include <cstring>  // For std::memcpy(.)
#include <cassert>  // For assert(.)

#ifdef ACBENCH_MULTITHREADED
#include <mutex>
#define ACBENCH_MUTEX_DECLARE mutable std::mutex m_mutex;  // mutable allows to change even in const methods
#define ACBENCH_MUTEX_GUARD std::lock_guard<std::mutex> mutex_lock(m_mutex);
#define ACBENCH_MUTEX_LOCK m_mutex.lock();
#define ACBENCH_MUTEX_UNLOCK m_mutex.unlock();
#else
#define ACBENCH_MUTEX_DECLARE
#define ACBENCH_MUTEX_GUARD
#define ACBENCH_MUTEX_LOCK
#define ACBENCH_MUTEX_UNLOCK
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
        int m_end = 0;  // One after the last element

        inline void destroy_nolock() {
            if ( m_data ) {
                delete[] m_data;  // GCOVR_EXCL_LINE
                m_data = nullptr;
            }
        }

     public:
        typedef T value_type;

     protected:
        // Copy constructor is forbidden to avoid implicit calls.
        // Do it manually if necessary (using `.resize_allocation(.)` and `.push_back(.)`)
        explicit ringbuffer(const ringbuffer<value_type>& rb) {
            (void)rb;
        }

        inline void memory_copy_nolock(value_type* pdest, const value_type* psrc, int size) {
            if (size == 0) return;  // GCOVR_EXCL_LINE
            assert(size > 0);
            std::memcpy(reinterpret_cast<void*>(pdest), reinterpret_cast<const void*>(psrc), sizeof(value_type)*static_cast<unsigned int>(size));
        }

        inline void memory_check_size_nolock(int nb_new_values) {
            (void)nb_new_values;
            assert(nb_new_values > 0);
            assert(m_size+nb_new_values <= m_size_max);
        }

        inline void clear_nolock() {
            m_front = 0;
            m_end = 0;
            m_size = 0;
        }

     public:
        //! Only allowed constructor
        ringbuffer() {
        }
        ringbuffer& operator=(const ringbuffer<value_type>& rb) {
            ACBENCH_MUTEX_GUARD
            this->clear_nolock();
            this->push_back_nolock(rb);
            return *this;
        }
        //! Allocate a new memory block and clear any previous data.
        //   * Always loose the data and reset the container to an empty state.
        //  (it is purposely not called reserve(.), because its behavior is different, see below).
        inline void resize_allocation(int size_max) {
            ACBENCH_MUTEX_GUARD
            if (size_max == m_size_max) {
                this->clear_nolock();
                return;
            }
            this->destroy_nolock();

            m_data = new value_type[size_max];  // GCOVR_EXCL_LINE // TODO(GD) Force contiguous memory
            m_size_max = size_max;

            this->clear_nolock();
        }
        // A more standard allocation function with behavior equivalent to std::vector::reserve()
        //  * It does nothing if the new size is less than or equal to the current size.
        //  * Otherwise, it increases the allocation and preserves the previous data.
        inline void reserve(int size_max) {
            ACBENCH_MUTEX_GUARD
            if (size_max <= m_size_max)
                return;

            value_type* new_data = new value_type[size_max];  // TODO(GD) Force contiguous memory
            memory_copy_nolock(new_data, m_data, m_size);

            delete[] m_data;
            m_data = new_data;

            m_size_max = size_max;
        }

        //! Does keep the allocation
        inline void clear() {
            ACBENCH_MUTEX_GUARD
            this->clear_nolock();
        }
        ~ringbuffer() {
            ACBENCH_MUTEX_GUARD
            this->destroy_nolock();
        }

        #ifdef ACBENCH_MULTITHREADED
        inline void lock() {
            m_mutex.lock();
        }
        inline void unlock() {
            m_mutex.unlock();
        }
        //! This is usefull to build a guard object out of the ringbuffer's mutex.
        inline std::mutex& mutex() const {
            return const_cast<std::mutex&>(m_mutex);
        }
        inline bool is_thread_safe() const {
            return true;
        }
        #else
        inline void lock() {
        }
        inline void unlock() {
        }
        inline bool is_thread_safe() const {
            return false;
        }
        #endif

        inline value_type* data() const {
            return m_data;  // Atomic, no need of locked mutex
        }
        inline int capacity() const {
            return m_size_max;          // Atomic, no need of locked mutex
        }
        inline int size_max() const {
            return capacity();          // Atomic, no need of locked mutex
        }
        inline int size() const {
            return m_size;  // Atomic, no need of locked mutex
        }
        // The index of the first element is always 0, as for any circular buffer.
        // So this function returns the index of `front` within the allocated memory, ie. relative to data()
        inline int front_data_index() const {
            assert(m_size > 0);
            return m_front;  // Atomic, no need of locked mutex
        }
        inline value_type front() const {
            assert(m_size > 0);
            ACBENCH_MUTEX_GUARD
            return m_data[m_front];
        }
        // The index of the last element is always size()-1, as for any circular buffer.
        // So this is the index of `back` within the allocated memory, ie. relative to data()
        inline int back_data_index() const {
            ACBENCH_MUTEX_GUARD
            int back = m_end - 1;
            if (back < 0)
                back = m_size_max-1;
            return back;
        }
        inline value_type back() const {
            assert(m_size > 0);
            ACBENCH_MUTEX_GUARD
            int back = m_end - 1;
            if (back < 0)
                back = m_size_max-1;
            return m_data[back];
        }
        inline bool empty() const {
            return m_size == 0;  // Atomic, no need of locked mutex
        }

        //! WARNING: Not thread-safe
        value_type operator[](int n) const {
            assert(n < m_size);
            return m_data[(m_front+n)%m_size_max];
        }
        //! WARNING: Not thread-safe
        value_type& operator[](int n) {
            assert(n < m_size);
            return m_data[(m_front+n)%m_size_max];
        }

        inline void push_back_nolock(const value_type v) {

            memory_check_size_nolock(1);

            m_data[m_end] = v;

            ++m_end;
            if (m_end >= m_size_max)
                m_end = 0;

            ++m_size;
        }
        inline void push_back(const value_type v) {
            ACBENCH_MUTEX_GUARD
            push_back_nolock(v);
        }
        inline void push_back_nolock(const value_type value, int nb_values) {
            if (nb_values <= 0)             // Ignore pushing no values
                return;

            memory_check_size_nolock(nb_values);

            if (m_end+nb_values <= m_size_max) {

                value_type* pdata = m_data+m_end;
                for (int k=0; k < nb_values; ++k)
                    *pdata++ = value;
                m_end += nb_values;

            } else {
                // Need to slice the array into two segments

                // 1st segment: m_end:m_size_max-1
                int seg1size = m_size_max - m_end;
                value_type* pdata = m_data+m_end;
                for (int k=0; k < seg1size; ++k)
                    *pdata++ = value;

                // 2nd segment: 0:nb_values-seg1size
                int seg2size = nb_values - seg1size;
                pdata = m_data;
                for (int k=0; k < seg2size; ++k)
                    *pdata++ = value;

                m_end = seg2size;
            }

            m_size += nb_values;
        }
        inline void push_back(const value_type value, int nb_values) {
            ACBENCH_MUTEX_GUARD
            push_back_nolock(value, nb_values);
        }
        inline void push_back_nolock(const value_type* array, int array_size) {
            if (array_size <= 0)             // Ignore push of empty buffers
                return;

            memory_check_size_nolock(array_size);

            if (m_end+array_size <= m_size_max) {
                // No need to slice it
                memory_copy_nolock(m_data+m_end, array, array_size);
                m_end += array_size;

            } else {
                // Need to slice the array into two segments

                // 1st segment: m_end:m_size_max-1
                int seg1size = m_size_max - m_end;
                memory_copy_nolock(m_data+m_end, array, seg1size);

                // 2nd segment: 0:array_size-seg1size
                int seg2size = array_size - seg1size;
                memory_copy_nolock(m_data, array+seg1size, seg2size);

                m_end = seg2size;
            }

            m_size += array_size;
        }
        inline void push_back(const value_type* array, int array_size) {
            ACBENCH_MUTEX_GUARD
            push_back_nolock(array, array_size);
        }

        inline void push_back_nolock(const ringbuffer<value_type>& rb) {
            if (rb.size() == 0)          // Ignore push of empty ringbuffers
                return;

            memory_check_size_nolock(rb.size());

            if (m_end+rb.m_size <= m_size_max) {
                // The destination segment is continuous

                // Now let's see the source segment(s)
                if (rb.m_front+rb.m_size <= rb.m_size_max) {
                    // The source segment is continuous...
                    // ... easiest game of my life:
                    memory_copy_nolock(m_data+m_end, rb.m_data+rb.m_front, rb.m_size);

                } else {
                    // The source segment is made of two continuous segments

                    // 1st segment
                    int seg1size = rb.m_size_max - rb.m_front;
                    memory_copy_nolock(m_data+m_end, rb.m_data+rb.m_front, seg1size);

                    // 2nd segment
                    int seg2size = rb.m_size - seg1size;
                    memory_copy_nolock(m_data+m_end+seg1size, rb.m_data, seg2size);
                }

                m_end += rb.size();

            } else {
                // The destination segment is made of two continuous segments

                if (rb.m_front+rb.m_size <= rb.m_size_max) {
                    // The source segment is continuous...

                    // 1st segment
                    int seg1size = m_size_max - m_end;
                    memory_copy_nolock(m_data+m_end, rb.m_data+rb.m_front, seg1size);

                    // 2nd segment
                    int seg2size = rb.m_size - seg1size;
                    memory_copy_nolock(m_data, rb.m_data+rb.m_front+seg1size, seg2size);

                    m_end = seg2size;

                } else {
                    // The source segment is also made of two continuous segments...
                    // ... worst game of my life: 3 segments to handle.

                    // Let's check if the source's break point comes before or after the destination's max size.
                    if ((rb.m_size_max-rb.m_front) < (m_size_max-m_end)) {
                        // the source's break point comes before the destination's max size...

                        // 1st segment
                        int seg1size = rb.m_size_max - rb.m_front;
                        memory_copy_nolock(m_data+m_end, rb.m_data+rb.m_front, seg1size);

                        // 2nd segment
                        int seg2size = (m_size_max-m_end) - seg1size;
                        memory_copy_nolock(m_data+m_end+seg1size, rb.m_data, seg2size);

                        // 3rd segment
                        int seg3size = rb.m_size - seg1size - seg2size;
                        memory_copy_nolock(m_data, rb.m_data+seg2size, seg3size);

                        m_end = seg3size;

                    } else {
                        // the source's break point comes after the destination's max size...

                        // 1st segment
                        int seg1size = m_size_max - m_end;
                        memory_copy_nolock(m_data+m_end, rb.m_data+rb.m_front, seg1size);

                        // 2nd segment
                        int seg2size = (rb.m_size_max-rb.m_front) - seg1size;
                        memory_copy_nolock(m_data, rb.m_data+rb.m_front+seg1size, seg2size);

                        // 3rd segment
                        int seg3size = rb.m_size - seg1size - seg2size;
                        memory_copy_nolock(m_data+seg2size, rb.m_data, seg3size);

                        m_end = seg2size + seg3size;
                    }
                }
            }

            m_size += rb.m_size;
        }
        inline void push_back(const ringbuffer<value_type>& rb) {
            ACBENCH_MUTEX_GUARD
            push_back_nolock(rb);
        }

        inline value_type pop_front_nolock() {
            assert(m_size >= 1);

            value_type value = m_data[m_front];

            ++m_front;

            if (m_front >= m_size_max)
                m_front = 0;

            --m_size;

            return value;
        }
        inline value_type pop_front() {
            ACBENCH_MUTEX_GUARD
            return pop_front_nolock();
        }
        inline void pop_front_nolock(int n) {
            if (n < 1) return;                // Just ignore pops of non-existing values

            if (n >= m_size) {                // Clears all if not enough to be poped
                clear_nolock();
                return;
            }

            m_front += n;
            if (m_front >= m_size_max)
                m_front -= m_size_max;

            m_size -= n;
        }
        inline void pop_front(int n) {
            ACBENCH_MUTEX_GUARD
            pop_front_nolock(n);
        }

        inline int pop_front_nolock(value_type* array, int n) {
            if (n < 1) return 0;              // Just ignore pops of non-existing values

            if (n > m_size)                   // Pop as many values as possible
                n = m_size;

            if (m_front+n <= m_size_max) {
                // No need to slice it
                memory_copy_nolock(array, m_data+m_front, n);
                m_front += n;

            } else {
                // Need to slice the array into two segments

                // 1st segment: m_front:m_size_max-1
                int seg1size = m_size_max - m_front;
                memory_copy_nolock(array, m_data+m_front, seg1size);

                // 2nd segment: 0:n-seg1size
                int seg2size = n - seg1size;
                memory_copy_nolock(array+seg1size, m_data, seg2size);

                m_front = seg2size;
            }

            m_size -= n;

            return n;
        }
        inline int pop_front(value_type* array, int n) {
            ACBENCH_MUTEX_GUARD
            return pop_front_nolock(array, n);
        }
        // Equivalent to rb.push_back(*this) and this->clear()
        inline int pop_front_nolock(ringbuffer<value_type>& rb) {
            int this_size = size();
            rb.push_back_nolock(*this);
            this->clear_nolock();
            return this_size;
        }
        inline int pop_front(ringbuffer<value_type>& rb) {
            ACBENCH_MUTEX_GUARD
            return pop_front_nolock(rb);
        }

        // TODO TODO TODO To test
        inline value_type pop_back_nolock() {
            assert(m_size >= 1);

            int back = m_end - 1;
            if (back < 0)
                back = m_size_max-1;
            value_type value = m_data[back];

            --m_end;

            if (m_end < 0)
                m_end = m_size_max-1;

            --m_size;

            return value;
        }
        inline value_type pop_back() {
            ACBENCH_MUTEX_GUARD
            return pop_back_nolock();
        }
        // TODO TODO TODO To test
        inline void pop_back_nolock(int n) {
            if (n < 1) return;                // Just ignore pops of non-existing values

            if (n >= m_size) {                // Clears all if not enough to be poped
                clear_nolock();
                return;
            }

            m_end -= n;
            if (m_end < 0)
                m_end += m_size_max;

            m_size -= n;
        }
        inline void pop_back(int n) {
            ACBENCH_MUTEX_GUARD
            pop_back_nolock(n);
        }
    };

}  // namespace acbench

#endif  // ACBENCH_RINGBUFFER_H_
