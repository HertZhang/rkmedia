// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EASYMEDIA_MEDIA_CONFIG_H_
#define EASYMEDIA_MEDIA_CONFIG_H_

#include "image.h"
#include "media_type.h"
#include "sound.h"

typedef struct {
  ImageInfo image_info;
  CodecType codec_type;
  int qp_init; // h264 : 0 - 48, higher value means higher compress
               //        but lower quality
               // jpeg (quantization coefficient): 1 - 10,
               // higher value means lower compress but higher quality,
               // contrary to h264
} ImageConfig;

typedef struct {
  ImageConfig image_cfg;
  int qp_step;
  int qp_min;
  int qp_max;
  int bit_rate;
  int frame_rate;
  int level;
  int gop_size;
  int profile;
  // quality - quality parameter
  //    (extra CQP level means special constant-qp (CQP) mode)
  //    (extra AQ_ONLY means special aq only mode)
  // "worst", "worse", "medium", "better", "best", "cqp", "aq_only"
  const char *rc_quality;
  // rc_mode - rate control mode
  // "vbr", "cbr"
  const char *rc_mode;
} VideoConfig;

typedef struct {
  SampleInfo sample_info;
  CodecType codec_type;
  // uint64_t channel_layout;
  int bit_rate;
  float quality; // vorbis: 0.0 ~ 1.0;
} AudioConfig;

typedef struct {
  union {
    VideoConfig vid_cfg;
    ImageConfig img_cfg;
    AudioConfig aud_cfg;
  };
  Type type;
} MediaConfig;

#include <map>

namespace easymedia {
extern const char *rc_quality_strings[7];
extern const char *rc_mode_strings[2];
bool ParseMediaConfigFromMap(std::map<std::string, std::string> &params,
                             MediaConfig &mc);
_API std::string to_param_string(const ImageConfig &img_cfg);
_API std::string to_param_string(const VideoConfig &vid_cfg);
_API std::string to_param_string(const AudioConfig &aud_cfg);
_API std::string to_param_string(const MediaConfig &mc,
                                 const std::string &out_type);
} // namespace easymedia

#endif // #ifndef EASYMEDIA_MEDIA_CONFIG_H_
