/* Copyright (C) 2024 Tristan Matthews
   File: speexenc_fuzzer.cc

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   - Neither the name of the Xiph.org Foundation nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <fuzzer/FuzzedDataProvider.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "speex/speex.h"
#include "speex/speex_stereo.h"

#define MAX_PACKET (1500)
#define SAMPLES (32000 * 10)
#define MAX_FRAME_SAMP (2000)
#define MAX_FRAME_BYTES (2000)

static const int sampling_rates[] = {8000, 16000, 32000};
static const int channels[] = {1, 2};
static const int modes[] = {SPEEX_MODEID_NB, SPEEX_MODEID_WB, SPEEX_MODEID_UWB};

static spx_int16_t inbuf[sizeof(spx_int16_t) * SAMPLES] = {0};
static unsigned char packet[MAX_PACKET + 257];

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);

    int num_channels = fdp.PickValueInArray(channels);
    int sampling_rate = fdp.PickValueInArray(sampling_rates);
    int modeID = fdp.PickValueInArray(modes);
    int quality = fdp.ConsumeIntegralInRange(0, 10);
    int complexity = fdp.ConsumeIntegralInRange(0, 10);
    int vbr_max = fdp.ConsumeIntegralInRange(1, 20000);
    int bitrate = fdp.ConsumeIntegralInRange(1, 20000);
    bool vbr_enabled = fdp.ConsumeBool();
    bool vad_enabled = fdp.ConsumeBool();
    bool abr_enabled = fdp.ConsumeBool();
    bool dtx_enabled = fdp.ConsumeBool();
    bool highpass_enabled = fdp.ConsumeBool();

    spx_int32_t lookahead = 0;
    float vbr_quality = quality;

    const SpeexMode *mode;
    int frame_size;
    int nb_encoded = 0;
    SpeexBits bits;
    char cbits[MAX_FRAME_BYTES];
    void *st;
    mode = speex_lib_get_mode(modeID);

    st = speex_encoder_init(mode);
    speex_bits_init(&bits);

    speex_encoder_ctl(st, SPEEX_GET_FRAME_SIZE, &frame_size);
    speex_encoder_ctl(st, SPEEX_SET_COMPLEXITY, &complexity);
    speex_encoder_ctl(st, SPEEX_SET_SAMPLING_RATE, &sampling_rate);

    if (quality >= 0)
    {
        if (vbr_enabled)
        {
            if (vbr_max > 0)
                speex_encoder_ctl(st, SPEEX_SET_VBR_MAX_BITRATE, &vbr_max);
            speex_encoder_ctl(st, SPEEX_SET_VBR_QUALITY, &vbr_quality);
        }
        else
            speex_encoder_ctl(st, SPEEX_SET_QUALITY, &quality);
    }
    if (bitrate)
        speex_encoder_ctl(st, SPEEX_SET_BITRATE, &bitrate);

    if (vbr_enabled)
    {
        int tmp = 1;
        speex_encoder_ctl(st, SPEEX_SET_VBR, &tmp);
    }

    if (vad_enabled)
    {
        int tmp = 1;
        speex_encoder_ctl(st, SPEEX_SET_VAD, &tmp);
    }

    if (dtx_enabled)
    {
        int tmp = 1;
        speex_encoder_ctl(st, SPEEX_SET_DTX, &tmp);
    }

    if (abr_enabled)
        speex_encoder_ctl(st, SPEEX_SET_ABR, &abr_enabled);

    speex_encoder_ctl(st, SPEEX_SET_HIGHPASS, &highpass_enabled);

    speex_encoder_ctl(st, SPEEX_GET_LOOKAHEAD, &lookahead);

    fdp.ConsumeData(inbuf, sizeof(inbuf));

    size_t samp_count = 0;
    do {
        const int frame_size_samples = frame_size * sampling_rate / 2000;

        if (num_channels == 2)
            speex_encode_stereo_int(&inbuf[samp_count * num_channels], frame_size, &bits);

        speex_encode_int(st, &inbuf[samp_count * num_channels], &bits);

        speex_bits_insert_terminator(&bits);
        size_t byte_to_write = speex_bits_nbytes(&bits);
        size_t bytes_written = speex_bits_write(&bits, cbits, MAX_FRAME_BYTES);
        speex_bits_reset(&bits);

        nb_encoded += frame_size;
        if (nb_encoded < 0 || nb_encoded > MAX_PACKET) {
            break;
        }
        samp_count += frame_size;
    } while (samp_count < ((SAMPLES / 2) - MAX_FRAME_SAMP));

    speex_bits_destroy(&bits);
    speex_encoder_destroy(st);

    return 0;
}
