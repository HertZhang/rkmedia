/*
 * Copyright (C) 2017 Hertz Wang 1989wanghang@163.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see http://www.gnu.org/licenses
 *
 * Any non-GPL usage of this software or parts of this software is strictly
 * forbidden.
 *
 */

#ifndef EASYMEDIA_CODEC_H_
#define EASYMEDIA_CODEC_H_

#include <list>
#include <memory>

#include "media_config.h"

namespace easymedia {

class MediaBuffer;
class Codec {
public:
  Codec();
  virtual ~Codec() = 0;
  static const char *GetCodecName() { return nullptr; }
  MediaConfig &GetConfig() { return config; }
  void SetConfig(const MediaConfig &cfg) { config = cfg; }
  _API std::shared_ptr<MediaBuffer> GetExtraData(void **data = nullptr,
                                                 size_t *size = nullptr);
  _API bool SetExtraData(void *data, size_t size, bool realloc = true);
  void SetExtraData(const std::shared_ptr<MediaBuffer> &data) {
    extra_data = data;
  }

  virtual bool Init() = 0;
  // sync call, input and output must be valid
  virtual int Process(const std::shared_ptr<MediaBuffer> &input,
                      std::shared_ptr<MediaBuffer> &output,
                      std::shared_ptr<MediaBuffer> extra_output = nullptr) = 0;

  // some codec may output many buffers with one input.
  // sync or async safe call, depends on specific codec.
  virtual int SendInput(const std::shared_ptr<MediaBuffer> &input) = 0;
  virtual std::shared_ptr<MediaBuffer> FetchOutput() = 0;

private:
  MediaConfig config;
  std::shared_ptr<MediaBuffer> extra_data;
};

_API const uint8_t *find_nalu_startcode(const uint8_t *p, const uint8_t *end);
// must be h264 data
_API std::list<std::shared_ptr<MediaBuffer>>
split_h264_separate(const uint8_t *buffer, size_t length, int64_t timestamp);
_API std::list<std::shared_ptr<MediaBuffer>>
split_h265_separate(const uint8_t *buffer, size_t length, int64_t timestamp);

} // namespace easymedia

#endif // #ifndef EASYMEDIA_CODEC_H_
