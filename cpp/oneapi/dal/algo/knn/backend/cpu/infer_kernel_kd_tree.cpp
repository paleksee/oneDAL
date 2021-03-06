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

#include <daal/src/algorithms/k_nearest_neighbors/kdtree_knn_classification_predict_dense_default_batch.h>
#include "data_management/data/numeric_table.h"

#include "oneapi/dal/algo/knn/backend/cpu/infer_kernel.hpp"
#include "oneapi/dal/algo/knn/backend/model_interop.hpp"
#include "oneapi/dal/algo/knn/detail/model_impl.hpp"
#include "oneapi/dal/backend/interop/common.hpp"
#include "oneapi/dal/backend/interop/table_conversion.hpp"
#include "oneapi/dal/detail/common.hpp"

namespace oneapi::dal::knn::backend {

using dal::backend::context_cpu;
using namespace daal::data_management;

namespace daal_knn = daal::algorithms::kdtree_knn_classification;
namespace interop  = dal::backend::interop;

template <typename Float, daal::CpuType Cpu>
using daal_knn_kd_tree_kernel_t = daal_knn::prediction::internal::
    KNNClassificationPredictKernel<Float, daal_knn::prediction::defaultDense, Cpu>;

template <typename Float>
static infer_result call_daal_kernel(const context_cpu& ctx,
                                     const descriptor_base& desc,
                                     const table& query,
                                     model m) {
    const std::int64_t row_count    = query.get_row_count();
    const std::int64_t column_count = query.get_column_count();

    auto arr_query  = row_accessor<const Float>{ query }.pull();
    auto arr_labels = array<Float>::empty(1 * row_count);
    // TODO: read-only access performed with deep copy of data since daal numeric tables are mutable.
    // Need to create special immutable homogen table on daal interop side

    // TODO: data is table, not a homogen_table. Think better about accessor - is it enough to have just a row_accessor?
    const auto daal_query =
        interop::convert_to_daal_homogen_table(arr_query, row_count, column_count);
    const auto daal_labels = interop::convert_to_daal_homogen_table(arr_labels, row_count, 1);

    const std::int64_t dummy_seed = 777;
    daal_knn::Parameter daal_parameter(
        desc.get_class_count(),
        desc.get_neighbor_count(),
        dummy_seed,
        desc.get_data_use_in_model() ? daal_knn::doUse : daal_knn::doNotUse);

    interop::call_daal_kernel<Float, daal_knn_kd_tree_kernel_t>(
        ctx,
        daal_query.get(),
        dal::detail::get_impl<detail::model_impl>(m).get_interop()->get_daal_model().get(),
        daal_labels.get(),
        &daal_parameter);
    return infer_result().set_labels(
        homogen_table_builder{}.reset(arr_labels, row_count, 1).build());
}

template <typename Float>
static infer_result infer(const context_cpu& ctx,
                          const descriptor_base& desc,
                          const infer_input& input) {
    return call_daal_kernel<Float>(ctx, desc, input.get_query(), input.get_model());
}

template <typename Float>
struct infer_kernel_cpu<Float, method::kd_tree> {
    infer_result operator()(const context_cpu& ctx,
                            const descriptor_base& desc,
                            const infer_input& input) const {
        return infer<Float>(ctx, desc, input);
    }
};

template struct infer_kernel_cpu<float, method::kd_tree>;
template struct infer_kernel_cpu<double, method::kd_tree>;

} // namespace oneapi::dal::knn::backend
