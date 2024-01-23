/*
 * Copyright (C) 2016-2023 T-Head Semiconductor Co., Ltd. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "testutil.h"

int main(int argc, char **argv)
{
    init_testsuite("Testing function of sum(layer).\n");

    struct csinn_session *sess = csinn_alloc_session();
    sess->base_run_mode = CSINN_RM_CPU_GRAPH;
    sess->model.save_mode = CSINN_RUN_ONLY;
    sess->dynamic_shape = CSINN_FALSE;
    struct csinn_tensor *input = csinn_alloc_tensor(sess);
    struct csinn_tensor *output = csinn_alloc_tensor(sess);
    struct csinn_tensor *reference = csinn_alloc_tensor(sess);
    struct csinn_reduce_params *params =
        (csinn_reduce_params *)csinn_alloc_params(sizeof(struct csinn_reduce_params), sess);
    int in_size = 1, out_size = 1;

    int *buffer = read_input_data_f32(argv[1]);

    input->dim_count = buffer[0];
    int axis = buffer[1];
    int keep_dim = buffer[2];
    output->dim_count = input->dim_count;
    for (int i = 0; i < input->dim_count; i++) {
        input->dim[i] = buffer[i + 3];
        if (keep_dim == 1) {
            if (i == axis) {
                output->dim[i] = 1;
            } else {
                output->dim[i] = input->dim[i];
            }
        }
        in_size *= input->dim[i];
    }

    if (keep_dim == 0) {
        for (int i = 0; i < input->dim_count; i++) {
            if (i < axis) {
                output->dim[i] = input->dim[i];
            } else if (i > axis) {
                output->dim[i - 1] = input->dim[i];
            }
        }
        output->dim_count = input->dim_count - 1;
    }

    input->dtype = CSINN_DTYPE_FLOAT32;
    input->layout = CSINN_LAYOUT_NCHW;
    input->is_const = 0;
    input->quant_channel = 1;
    set_layout(input);

    output->dtype = CSINN_DTYPE_FLOAT32;
    output->layout = CSINN_LAYOUT_NCHW;
    output->is_const = 0;
    output->quant_channel = 1;
    set_layout(output);

    input->data = (float *)(buffer + 3 + input->dim_count);
    reference->data = (float *)(buffer + 3 + input->dim_count + in_size);
    output->data = reference->data;
    float difference = argc > 2 ? atof(argv[2]) : 0.99;

    params->axis = &axis;
    params->axis_count = 1;  // must be 1
    params->base.api = CSINN_API;
    params->base.layout = CSINN_LAYOUT_NCHW;

#if (DTYPE == 32)
    test_unary_op(input, output, params, CSINN_DTYPE_FLOAT32, CSINN_QUANT_FLOAT32, sess,
                  csinn_reduce_sum_init, csinn_reduce_sum, &difference);
#elif (DTYPE == 16)
    test_unary_op(input, output, params, CSINN_DTYPE_FLOAT16, CSINN_QUANT_FLOAT16, sess,
                  csinn_reduce_sum_init, csinn_reduce_sum, &difference);
#elif (DTYPE == 8)
    test_unary_op(input, output, params, CSINN_DTYPE_INT8, CSINN_QUANT_INT8_ASYM, sess,
                  csinn_reduce_sum_init, csinn_reduce_sum, &difference);
#endif

    return done_testing();
}
