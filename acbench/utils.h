// Copyright (C) 2024 Gilles Degottex - All Rights Reserved
//
// You may use, distribute and modify this code under the
// terms of the Apache 2.0 license. You should have
// received a copy of this license with this file.
// If not, please visit:
//     https://github.com/gillesdegottex/acbench

#ifndef ACBENCH_UTILS_H_
#define ACBENCH_UTILS_H_

#include <random>

#include <iostream>
#include <cassert>

namespace acbench {

    //  TODO(GD) Replace with std::uniform_real_distribution
    template<typename T>
    inline T rand_uniform_continuous_01() {
        return static_cast<T>(std::rand()) / static_cast<T>(RAND_MAX);
    }

    //! (until std::format in c++26)
    template<typename T>
    inline std::string to_string(T v, const char* fmt) {
        int sz = std::snprintf(nullptr, 0, fmt, v);
        std::vector<char> buf(sz + 1);  // note +1 for null terminator
        std::snprintf(buf.data(), sz + 1, fmt, v);
        return std::string(buf.begin(), buf.end()-1);
    }

    inline void print(std::ostream* pout, const float* array, int size) {
        std::ostream& out = *pout;
        out << "[";
        for (int n = 0; n < size; ++n) {
            if (n > 0)
                out << ", ";
            out << array[n];
        }
        out << "]";
    }

    template<typename Array>
    inline void print(std::ostream* pout, const Array& array) {
        std::ostream& out = *pout;
        out << "[";
        for (int n=0; n < int(array.size()); ++n) {
            if (n > 0)
                out << ", ";
            out << array[n];
        }
        out << "]";
    }

    template<typename array_ref, typename array_test>
    inline bool compare(const array_ref& arr_ref, const array_test& arr_test) {
        if (static_cast<int>(arr_ref.size()) != static_cast<int>(arr_test.size())) {
            std::cerr << "ERROR: compare: reference and test arrays have different sizes." << std::endl;
            return false;
        }

        bool ok = true;
        for (int n=0; n < static_cast<int>(arr_ref.size()); ++n) {
            if (std::isnan(arr_test[n])) {
                std::cerr << "ERROR: compare: Values at index " << n << " is NaN." << std::endl;
                ok = false;
            }
            if (std::isinf(arr_test[n])) {
                std::cerr << "ERROR: compare: Values at index " << n << " is Inf." << std::endl;
                ok = false;
            }
            if (arr_ref[n] != arr_test[n]) {
                std::cerr << "ERROR: compare: Values at index " << n << " are different. " << arr_ref[n] << "!=" << arr_test[n] << std::endl;
                ok = false;
            }
        }
        return ok;
    }

}  // namespace acbench

#endif  // ACBENCH_UTILS_H_
