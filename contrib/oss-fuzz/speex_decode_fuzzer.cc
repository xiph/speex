/* Copyright (C) 2017 Tristan Matthews
   File: speex_decode_fuzzer.cc

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

#include <stdint.h>
#include <stddef.h>

#include <speex/speex.h>

/*Assumes raw, concatenated Speex packets as input.*/

/*The frame size in hardcoded for this sample code but it doesn't have to be.*/
#define FRAME_SIZE 160

#ifndef DISABLE_FLOAT_API
    typedef float output_type;
    #define speex_decode_func(a, b, c) speex_decode(a, b, c)
#else
    typedef spx_int16_t output_type;
    #define speex_decode_func(a, b, c) speex_decode_int(a, b, c)
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   output_type output[FRAME_SIZE];
   void *decoder_state;
   SpeexBits bitstream;
   int tmp;

   /*Create a new decoder state in narrowband mode.*/
   /*FIXME: do we need 1 fuzz target per mode?*/
   decoder_state = speex_decoder_init(&speex_nb_mode);

   /*Set the perceptual enhancement on.*/
   /*FIXME: should we toggle other decoder ctls?*/
   tmp = 1;
   speex_decoder_ctl(decoder_state, SPEEX_SET_ENH, &tmp);

   speex_bits_init(&bitstream);
   /*Read the entire bitstream at once, since packets are concatenated together.*/
   speex_bits_read_from(&bitstream, (char *)data, size);

   /*Decode the data, 1 packet at a time.*/
   while (!speex_decode_func(decoder_state, &bitstream, output))
       ; /*noop*/

   speex_decoder_destroy(decoder_state);
   speex_bits_destroy(&bitstream);
   return 0;
}
