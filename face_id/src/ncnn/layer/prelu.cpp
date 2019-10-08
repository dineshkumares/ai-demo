// Tencent is pleased to support the open source community by making ncnn available.
//
// Copyright (C) 2017 THL A29 Limited, a Tencent company. All rights reserved.
//
// Licensed under the BSD 3-Clause License (the "License"); you may not use this file except
// in compliance with the License. You may obtain a copy of the License at
//
// https://opensource.org/licenses/BSD-3-Clause
//
// Unless required by applicable law or agreed to in writing, software distributed
// under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
// CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.

#include "prelu.h"

namespace ncnn {

    DEFINE_LAYER_CREATOR(PReLU)

    PReLU::PReLU() {
        one_blob_only = true;
        support_inplace = true;
    }

    int PReLU::load_param(const ParamDict &pd) {
        num_slope = pd.get(0, 0);

        return 0;
    }

#if NCNN_STDIO

    int PReLU::load_model(FILE *binfp) {
        size_t nread;

        slope_data.create(num_slope);
        if (slope_data.empty())
            return -100;
        nread = fread(slope_data, num_slope * sizeof(float), 1, binfp);
        if (nread != 1) {
            fprintf(stderr, "PReLU read slope_data failed %ld\n", nread);
            return -1;
        }

        return 0;
    }

#endif // NCNN_STDIO

    int PReLU::load_model(const unsigned char *&mem) {
        slope_data = Mat(num_slope, (float *) mem);
        mem += num_slope * sizeof(float);

        return 0;
    }


    int PReLU::forward_inplace(Mat &bottom_top_blob) const {
        int dims = bottom_top_blob.dims;

        if (dims == 1) {
            int w = bottom_top_blob.w;

            float *ptr = bottom_top_blob;

            if (num_slope > 1) {
#pragma omp parallel for
                for (int i = 0; i < w; i++) {
                    if (ptr[i] < 0)
                        ptr[i] *= slope_data[i];
                }
            } else {
                float slope = slope_data[0];

#pragma omp parallel for
                for (int i = 0; i < w; i++) {
                    if (ptr[i] < 0)
                        ptr[i] *= slope;
                }
            }
        }

        if (dims == 2) {
            int w = bottom_top_blob.w;
            int h = bottom_top_blob.h;

#pragma omp parallel for
            for (int i = 0; i < h; i++) {
                float *ptr = bottom_top_blob.row(i);
                float slope = num_slope > 1 ? slope_data[i] : slope_data[0];

                for (int j = 0; j < w; j++) {
                    if (ptr[j] < 0)
                        ptr[j] *= slope;
                }
            }
        }

        if (dims == 3) {
            int w = bottom_top_blob.w;
            int h = bottom_top_blob.h;
            int channels = bottom_top_blob.c;
            int size = w * h;

#pragma omp parallel for
            for (int q = 0; q < channels; q++) {
                float *ptr = bottom_top_blob.channel(q);
                float slope = num_slope > 1 ? slope_data[q] : slope_data[0];

                for (int i = 0; i < size; i++) {
                    if (ptr[i] < 0)
                        ptr[i] *= slope;
                }
            }
        }

        return 0;
    }


} // namespace ncnn
