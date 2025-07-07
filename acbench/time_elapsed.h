// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#ifndef ACBENCH_TIME_ELAPSED_H_
#define ACBENCH_TIME_ELAPSED_H_

#include "utils.h"
#include <acbench/ringbuffer.h>
// #include <acbench/vector.h>

#include <chrono>  // TODO(GD) Not approved??
#include <deque>
#include <algorithm>
#include <string>
#include <cmath>
#include <mutex>
#include <iostream>
#include <cassert>

namespace acbench {

    //! This object stores the last million time intervals between `.start()` and `.end()` calls.
    //  ( limit can be changed with `set_size_max(.)` )
    class time_elapsed {
     private:

        std::chrono::high_resolution_clock::time_point m_start;
        std::chrono::high_resolution_clock::time_point m_end;

        acbench::ringbuffer<double> m_elapsed;
        acbench::ringbuffer<double> m_proced_duration;
        // mutable std::mutex m_elapsed_median_mutex;
        // mutable acbench::vector<double> m_elapsed_median_sorted;

        int m_size_max = 1000000;

     public:
        explicit time_elapsed(int size_max = 1000000) {
            set_size_max(size_max);
            m_elapsed.resize_allocation(m_size_max);
            m_proced_duration.resize_allocation(m_size_max);
            // m_elapsed_median_sorted.resize_allocation(size_max);
            reset();
        }
        time_elapsed(const time_elapsed& te) {
            set_size_max(te.size_max());
            m_elapsed.push_back(te.m_elapsed);
            m_proced_duration.push_back(te.m_proced_duration);
            // for(auto el : te.m_elapsed)
                // m_elapsed.push_back(el);
            // m_elapsed_median_sorted.resize_allocation(te.m_elapsed_median_sorted.size_max());
            // m_elapsed_median_sorted.resize(te.m_elapsed_median_sorted.size());
            m_start = te.m_start;
            m_end = te.m_end;
        }
        time_elapsed& operator=(const time_elapsed& te) {
            m_elapsed = te.m_elapsed;
            m_start = te.m_start;
            m_end = te.m_end;
            m_proced_duration = te.m_proced_duration;
            return *this;
        }
        ~time_elapsed() {
        }
        inline int size() const {
            return m_elapsed.size();
        }
        inline int size_max() const {
            return m_size_max;
        }
        //! Set the maximum number of time intervals to store.
        inline void set_size_max(int size_max) {
            assert(size_max > 0);
            m_size_max = size_max;
            m_elapsed.resize_allocation(m_size_max);
            m_proced_duration.resize_allocation(m_size_max);
            reset();
        }
        inline void merge(const time_elapsed& te) {
            m_elapsed.push_back(te.m_elapsed);
            m_proced_duration.push_back(te.m_proced_duration);
        }
        inline void start() {
            m_start = std::chrono::high_resolution_clock::now();
        }
        inline void end(float proced_duration) {
            m_end = std::chrono::high_resolution_clock::now();
            std::chrono::duration<double> diff = (m_end-m_start);
            if ((m_size_max > 0) && (m_elapsed.size()+1 > m_size_max)) {
                m_elapsed.pop_front();
                m_proced_duration.pop_front();
            }
            m_elapsed.push_back(diff.count());
            m_proced_duration.push_back(proced_duration);
        }
        const acbench::ringbuffer<double>& elapsed() const {
            return m_elapsed;
        }
        double elapsed_last() const {
            assert(m_elapsed.size() > 0);
            return m_elapsed[m_elapsed.size()-1];
        }
        inline void reset() {
            m_elapsed.clear();
            m_proced_duration.clear();
        }
        inline double proced_duration() const {
            double proced_duration_sum = 0.0;
            for (int n=0; n<m_proced_duration.size(); ++n)
                proced_duration_sum += m_proced_duration[n];
            return proced_duration_sum;
        }
        inline double sum() const {
            double sum = 0.0;
            for (int n=0; n<m_elapsed.size(); ++n)
                sum += m_elapsed[n];
            return sum;
        }
        inline double min() const {
            assert(m_elapsed.size() > 0);
            double val = m_elapsed[0];
            for (int n = 1; n < int(m_elapsed.size()); ++n)
                val = std::min(val, m_elapsed[n]);
            return val;
        }
        inline double max() const {
            assert(m_elapsed.size() > 0);
            double val = m_elapsed[0];
            for (int n = 1; n < int(m_elapsed.size()); ++n)
                val = std::max(val, m_elapsed[n]);
            return val;
        }
        inline double mean() const {
            assert(m_elapsed.size() > 0);
            double mean_sum = 0.0;
            for (int n=0; n < int(m_elapsed.size()); ++n) {
                mean_sum += m_elapsed[n];
            }
            return mean_sum/m_elapsed.size();
        }

        // inline double median() const {
        //     assert(m_elapsed.size() > 0);
        //     assert(m_elapsed_median_sorted.size_max()>=m_elapsed.size());
        //     int n = m_elapsed.size() / 2;
        //     const std::lock_guard<std::mutex> lock(m_elapsed_median_mutex);
        //     m_elapsed_median_sorted = m_elapsed;
        //     std::nth_element(m_elapsed_median_sorted.begin(), m_elapsed_median_sorted.begin()+n, m_elapsed_median_sorted.end());
        //     return m_elapsed_median_sorted[n];
        // }
        inline double std() const {
            assert(m_elapsed.size() > 0);
            if (m_elapsed.size() == 1)
                return 0.0;
            double meanv = mean();
            double var_sum = 0.0;
            for (int n=0; n < int(m_elapsed.size()); ++n) {
                double d = m_elapsed[n] - meanv;
                var_sum += d*d;
            }
            double var;
            if (m_elapsed.size() > 1)
                var = var_sum / (m_elapsed.size()-1);
            else
                var = var_sum;
            return std::sqrt(var);
        }
        inline std::string stats(int exp10=6) const {
            if (m_elapsed.size() < 1)
                return "empty, #0";
            std::string res;
            std::string unit;
            if (exp10==0)        unit="s";
            else if (exp10==3)   unit="ms";
            else if (exp10==6)   unit="µs";
            else if (exp10==9)   unit="ns";
            else if (exp10==12)  unit="ps";
            else {
                assert(exp10 && "exp10 should be 3, 6, 9 or 12, nothing else.");
            }
            
            res += "mean="+acbench::to_string(mean()*std::pow(10,exp10), "%7.2f")+unit+", std="+acbench::to_string(std()*std::pow(10,exp10), "%7.2f")+unit+", max="+acbench::to_string(max()*std::pow(10,exp10), "%7.2f")+unit+", dur="+acbench::to_string(proced_duration(), "%4.2f");

            if (proced_duration() > 0.0)
                res += ", RTX="+acbench::to_string(proced_duration()/sum(), "%5.3f");

            res += ", #"+std::to_string(size())+"/"+std::to_string(size_max());

            if (size()==size_max())
                res += "(max capacity was reached!)";

            return res;
        }

        inline void print(std::ostream* pout) const {
            acbench::print(pout, m_elapsed);
        }
    };

}  // namespace acbench

#endif  // ACBENCH_TIME_ELAPSED_H_
