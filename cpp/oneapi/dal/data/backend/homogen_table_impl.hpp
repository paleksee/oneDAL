/*******************************************************************************
* Copyright 2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

#pragma once

#include "oneapi/dal/data/common_helpers.hpp"
#include "oneapi/dal/data/table_metadata.hpp"

namespace oneapi::dal::backend {

class homogen_table_impl {
public:
    homogen_table_impl() : row_count_(0) {}

    template <typename Data>
    homogen_table_impl(std::int64_t N,
                       std::int64_t p,
                       const Data* data_pointer,
                       homogen_data_layout layout)
            : meta_(homogen_table_metadata{ make_data_type<Data>(), layout, p }),
              data_(array<Data>(),
                    reinterpret_cast<const byte_t*>(data_pointer),
                    N * p * sizeof(Data)),
              row_count_(N) {}

    homogen_table_impl(std::int64_t p,
                       const array<byte_t>& data,
                       table_feature feature,
                       homogen_data_layout layout)
            : meta_(homogen_table_metadata{ feature, layout, p }),
              data_(data),
              row_count_(data.get_count() / p / get_data_type_size(feature.get_data_type())) {}

    template <typename Data>
    homogen_table_impl(std::int64_t p, const array<Data>& data, homogen_data_layout layout)
            : meta_(homogen_table_metadata{ make_data_type<Data>(), layout, p }),
              row_count_(data.get_count() / p) {
        const std::int64_t N = row_count_;

        if (N * p != data.get_count()) {
            throw std::runtime_error("data size must be power of column count");
        }

        if (data.has_mutable_data()) {
            data_.reset(data, reinterpret_cast<byte_t*>(data.get_mutable_data()), data.get_size());
        }
        else {
            data_.reset(data, reinterpret_cast<const byte_t*>(data.get_data()), data.get_size());
        }
    }

    std::int64_t get_column_count() const {
        return meta_.get_feature_count();
    }

    std::int64_t get_row_count() const {
        return row_count_;
    }

    const homogen_table_metadata& get_metadata() const {
        return meta_;
    }

    const void* get_data() const {
        return data_.get_data();
    }

    template <typename T>
    void pull_rows(array<T>& a, const range& r) const;

    template <typename T>
    void push_rows(const array<T>& a, const range& r);

    template <typename T>
    void pull_column(array<T>& a, std::int64_t idx, const range& r) const;

    template <typename T>
    void push_column(const array<T>& a, std::int64_t idx, const range& r);

#ifdef ONEAPI_DAL_DATA_PARALLEL
    template <typename T>
    void pull_rows(sycl::queue& q, array<T>& a, const range& r, const sycl::usm::alloc& kind) const;

    template <typename T>
    void push_rows(sycl::queue& q, const array<T>& a, const range& r);

    template <typename T>
    void pull_column(sycl::queue& q,
                     array<T>& a,
                     std::int64_t idx,
                     const range& r,
                     const sycl::usm::alloc& kind) const;

    template <typename T>
    void push_column(sycl::queue& q, const array<T>& a, std::int64_t idx, const range& r);
#endif

private:
    homogen_table_metadata meta_;
    array<byte_t> data_;
    int64_t row_count_;
};

} // namespace oneapi::dal::backend
