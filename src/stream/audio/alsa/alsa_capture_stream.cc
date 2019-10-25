// Copyright 2019 Fuzhou Rockchip Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "stream.h"

#include <assert.h>
#include <errno.h>

#include "alsa_utils.h"
#include "media_type.h"
#include "utils.h"

namespace easymedia {

class AlsaCaptureStream : public Stream {
public:
  AlsaCaptureStream(const char *param);
  virtual ~AlsaCaptureStream();
  static const char *GetStreamName() { return "alsa_capture_stream"; }
  virtual size_t Read(void *ptr, size_t size, size_t nmemb) final;
  virtual int Seek(int64_t offset _UNUSED, int whence _UNUSED) final {
    return -1;
  }
  virtual long Tell() final { return -1; }
  virtual size_t Write(const void *ptr _UNUSED, size_t size _UNUSED,
                       size_t nmemb _UNUSED) final {
    return 0;
  }
  virtual int Open() final;
  virtual int Close() final;

private:
  SampleInfo sample_info;
  std::string device;
  snd_pcm_t *alsa_handle;
  size_t frame_size;
};

AlsaCaptureStream::AlsaCaptureStream(const char *param)
    : alsa_handle(NULL), frame_size(0) {
  memset(&sample_info, 0, sizeof(sample_info));
  sample_info.fmt = SAMPLE_FMT_NONE;
  std::map<std::string, std::string> params;
  int ret = ParseAlsaParams(param, params, device, sample_info);
  UNUSED(ret);
  if (device.empty())
    device = "default";
  if (SampleInfoIsValid(sample_info))
    SetReadable(true);
  else
    LOG("missing some necessary param\n");
}

AlsaCaptureStream::~AlsaCaptureStream() {
  if (alsa_handle)
    AlsaCaptureStream::Close();
}

size_t AlsaCaptureStream::Read(void *ptr, size_t size, size_t nmemb) {
  size_t buffer_len = size * nmemb;
  snd_pcm_sframes_t gotten = 0;
  snd_pcm_sframes_t nb_samples =
      (size == frame_size ? nmemb : buffer_len / frame_size);
  while (nb_samples > 0) {
    // SND_PCM_ACCESS_RW_INTERLEAVED
    int status = snd_pcm_readi(alsa_handle, ptr, nb_samples);
    if (status < 0) {
      if (status == -EAGAIN) {
        /* Apparently snd_pcm_recover() doesn't handle this case - does it
         * assume snd_pcm_wait() above? */
        std::this_thread::sleep_for(std::chrono::microseconds(100));
        errno = EAGAIN;
        break;
      }
      status = snd_pcm_recover(alsa_handle, status, 0);
      if (status < 0) {
        /* Hmm, not much we can do - abort */
        LOG("ALSA write failed (unrecoverable): %s\n", snd_strerror(status));
        errno = EIO;
        break;
      }
      errno = EIO;
      break;
    }
    nb_samples -= status;
    gotten += status;
  }

  return gotten * frame_size / size;
}

int AlsaCaptureStream::Open() {
  snd_pcm_t *pcm_handle = NULL;
  snd_pcm_hw_params_t *hwparams = NULL;
  if (!Readable())
    return -1;
  int status = snd_pcm_hw_params_malloc(&hwparams);
  if (status < 0) {
    LOG("snd_pcm_hw_params_malloc failed\n");
    return -1;
  }
  pcm_handle = AlsaCommonOpenSetHwParams(device.c_str(), SND_PCM_STREAM_CAPTURE,
                                         0, sample_info, hwparams);
  if (!pcm_handle)
    goto err;
  if ((status = snd_pcm_hw_params(pcm_handle, hwparams)) < 0) {
    LOG("cannot set parameters (%s)\n", snd_strerror(status));
    goto err;
  }
#ifndef NDEBUG
  /* This is useful for debugging */
  do {
    unsigned int periods = 0;
    snd_pcm_uframes_t period_size = 0;
    snd_pcm_uframes_t bufsize = 0;
    snd_pcm_hw_params_get_periods(hwparams, &periods, NULL);
    snd_pcm_hw_params_get_period_size(hwparams, &period_size, NULL);
    snd_pcm_hw_params_get_buffer_size(hwparams, &bufsize);
    LOG("ALSA: period size = %ld, periods = %u, buffer size = %lu\n",
        period_size, periods, bufsize);
  } while (0);
#endif
  if ((status = snd_pcm_prepare(pcm_handle)) < 0) {
    LOG("cannot prepare audio interface for use (%s)\n", snd_strerror(status));
    goto err;
  }
  /* Switch to blocking mode for capture */
  // snd_pcm_nonblock(pcm_handle, 0);

  snd_pcm_hw_params_free(hwparams);
  frame_size = snd_pcm_frames_to_bytes(pcm_handle, 1);
  alsa_handle = pcm_handle;
  return 0;

err:
  if (hwparams)
    snd_pcm_hw_params_free(hwparams);
  if (pcm_handle) {
    snd_pcm_drain(pcm_handle);
    snd_pcm_close(pcm_handle);
  }
  return -1;
}

int AlsaCaptureStream::Close() {
  if (alsa_handle) {
    snd_pcm_drop(alsa_handle);
    snd_pcm_close(alsa_handle);
    alsa_handle = NULL;
    LOG("audio capture close done\n");
    return 0;
  }
  return -1;
}

DEFINE_STREAM_FACTORY(AlsaCaptureStream, Stream)

const char *FACTORY(AlsaCaptureStream)::ExpectedInputDataType() {
  return nullptr;
}

const char *FACTORY(AlsaCaptureStream)::OutPutDataType() { return AUDIO_PCM; }

} // namespace easymedia
