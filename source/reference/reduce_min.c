/*
 * Copyright (C) 2016-2020 C-SKY Limited. All rights reserved.
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

#include "csi_nn.h"
#include "csi_utils.h"

static int csi_reduce_min_f32(struct csi_tensor *input,
                            struct csi_tensor *output,
                            struct reduce_params *params)
{
    float *input_data = (float *)input->data;
    float *output_data = (float *)output->data;
    assert(params->axis_count==1);  //the Function realization assumption axis_count=1
    //axis=none
    if(*(params->axis) == -1) {
        int size = 1;
        for(int i = 0; i < input->dim_count; i++) {
            size = size * input->dim[i];
        }
        float res = *input_data;
        for(int j = 1; j < size; j++) {
            res = fmin(res, input_data[j]);
        }
        *output_data = res;
    } else {
        int axis = *(params->axis);
        int64_t outer_size = 1;
        for(int i = 0; i < axis; i++) {
            outer_size *= input->dim[i];
        }
        int64_t inner_size = 1;
        for(int i = axis + 1; i < input->dim_count; i++) {
            inner_size *= input->dim[i];
        }
        int cnt = input->dim[axis];

        for(int i = 0; i < outer_size; i++) {
            for(int k = 0; k < inner_size; k++) {
                float temp = *(input_data + k);
                for(int j = 1; j < cnt; j++) {
                    temp = fmin(temp, *(input_data + j * inner_size + k));
                }
                *(output_data + k) = temp;
            }
            input_data += inner_size * cnt;
            output_data += inner_size;
        }
    }
    return CSINN_TRUE;
}

static int csi_reduce_min_u8(struct csi_tensor *input,
                            struct csi_tensor *output,
                            struct reduce_params *params)
{
    uint8_t *input_data = (uint8_t *)input->data;
    uint8_t *output_data = (uint8_t *)output->data;
    assert(params->axis_count==1);  //the Function realization assumption axis_count=1
    //axis=none
    if(*(params->axis) == -1) {
        int size = 1;
        for(int i=0; i<input->dim_count; i++) {
            size = size * input->dim[i];
        }
        float res = csi_dequantize_f32(input_data[0], input->offset, input->multiplier, input->shift);
        for(int j = 1; j < size; j++) {
            float input_temp = csi_dequantize_f32(input_data[j], input->offset, input->multiplier, input->shift);
            res = fmin(res, input_temp);
        }
        *output_data = csi_quantize_f32(res, output->offset, output->multiplier, output->shift);
    } else {
        int axis = *(params->axis);
        int64_t outer_size = 1;
        for(int i = 0; i < axis; i++) {
            outer_size *= input->dim[i];
        }
        int64_t inner_size = 1;
        for(int i = axis + 1; i < input->dim_count; i++) {
            inner_size *= input->dim[i];
        }
        int cnt = input->dim[axis];

        for(int i = 0; i < outer_size; i++) {
            for(int k = 0; k < inner_size; k++) {
                float temp = csi_dequantize_f32(input_data[k], input->offset, input->multiplier, input->shift);
                for(int j = 1; j < cnt; j++) {
                    uint8_t input_val = *(input_data + j * inner_size + k);
                    float input_temp = csi_dequantize_f32(input_val, input->offset, input->multiplier, input->shift);
                    temp = fmin(temp, input_temp);
                }
                *(output_data + k) = csi_quantize_f32(temp, output->offset, output->multiplier, output->shift);
            }
            input_data += inner_size * cnt;
            output_data += inner_size;
        }
    }
    return CSINN_TRUE;
}

int csi_reduce_min_init(struct csi_tensor *input,
                        struct csi_tensor *output,
                        struct reduce_params *params)
{
    if (input->dtype == CSINN_DTYPE_UINT8) {
        params->bc = csi_reduce_min_u8;
    } else if (input->dtype == CSINN_DTYPE_FLOAT32) {
        params->bc = csi_reduce_min_f32;
    } else {
        return CSINN_UNSUPPORT_DTYPE;
    }
    return CSINN_TRUE;
}

int csi_reduce_min(struct csi_tensor *input,
                    struct csi_tensor *output,
                    struct reduce_params *params)
{
    if (params->bc != NULL) {
        params->bc(input, output, params);
    } else {
        return CSINN_CALLBACK_UNSET;
    }
    return CSINN_TRUE;
}