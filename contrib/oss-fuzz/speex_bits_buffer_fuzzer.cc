/* Copyright (C) 2026 Xiph.Org Foundation
   File: speex_bits_buffer_fuzzer.cc

   Fuzzes the caller-supplied (non-owned) SpeexBits buffer API.  The Ogg decoder
   fuzzer only drives speex_bits_init() (owner=1) with speex_bits_read_from(), so
   the caller-owned buffer path -- speex_bits_init_buffer() plus the owner==0
   branch of speex_bits_read_whole_bytes() -- is never exercised.  This target
   lets the fuzzer pick the buffer size and then replay an input-chosen sequence
   of whole-byte appends, unpacks, peeks, advances, rewinds and resets, the way a
   real caller manipulates a SpeexBits object, optionally feeding the bits to a
   Speex decoder.

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "speex/speex.h"
#include "speex/speex_bits.h"

#ifndef DISABLE_FLOAT_API
    typedef float output_type;
    #define speex_decode_func(a, b, c) speex_decode(a, b, c)
#else
    typedef spx_int16_t output_type;
    #define speex_decode_func(a, b, c) speex_decode_int(a, b, c)
#endif

/* A tiny cursor over the fuzz input; every decision is drawn from it. */
namespace {
struct Reader {
   const uint8_t *p;
   size_t n;
   size_t i;
   Reader(const uint8_t *data, size_t size) : p(data), n(size), i(0) {}
   bool done() const { return i >= n; }
   uint8_t u8() { return i < n ? p[i++] : 0; }
};
}

#define MAX_BUF_SIZE 1024
#define MAX_OPS 64
#define MAX_FRAME_SIZE 2000

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
   if (size < 2 || size > 4096)
      return 0;

   Reader r(data, size);

   /* Caller chooses the owned buffer size (1..MAX_BUF_SIZE). */
   int buf_size = (r.u8() % MAX_BUF_SIZE) + 1;
   char *backing = (char*)malloc(buf_size);
   if (!backing)
      return 0;

   SpeexBits bits;
   speex_bits_init_buffer(&bits, backing, buf_size); /* owner=0 */

   /* Optional decoder, selected and sized entirely from the input. */
   uint8_t cfg = r.u8();
   void *dec = NULL;
   int frame_size = 0;
   output_type output[MAX_FRAME_SIZE];
   if (cfg & 1) {
      int modeID = (cfg >> 1) & 1; /* 0 = narrowband, 1 = wideband */
      const SpeexMode *mode = speex_lib_get_mode(modeID);
      dec = speex_decoder_init(mode);
      if (dec) {
         speex_decoder_ctl(dec, SPEEX_GET_FRAME_SIZE, &frame_size);
         if (frame_size < 0 || frame_size > MAX_FRAME_SIZE) {
            speex_decoder_destroy(dec);
            dec = NULL;
         }
      }
   }

   /* Replay an input-driven sequence of SpeexBits operations. */
   for (int op = 0; op < MAX_OPS && !r.done(); op++)
   {
      switch (r.u8() % 9)
      {
         case 0: { /* append whole bytes from the input */
            int want = r.u8();
            int avail = (int)(r.n - r.i);
            int nbytes = want < avail ? want : avail;
            speex_bits_read_whole_bytes(&bits, (const char*)(r.p + r.i), nbytes);
            r.i += nbytes;
            break;
         }
         case 1:
            (void)speex_bits_unpack_unsigned(&bits, (r.u8() % 32) + 1);
            break;
         case 2:
            (void)speex_bits_unpack_signed(&bits, (r.u8() % 32) + 1);
            break;
         case 3:
            (void)speex_bits_peek_unsigned(&bits, (r.u8() % 32) + 1);
            break;
         case 4:
            (void)speex_bits_peek(&bits);
            break;
         case 5:
            speex_bits_advance(&bits, r.u8());
            break;
         case 6:
            speex_bits_rewind(&bits);
            break;
         case 7:
            speex_bits_reset(&bits); /* owner=0: clears cursor, keeps buffer */
            break;
         case 8:
            if (dec && frame_size > 0)
               (void)speex_decode_func(dec, &bits, output);
            else
               (void)speex_bits_remaining(&bits);
            break;
      }
   }

   if (dec)
      speex_decoder_destroy(dec);
   speex_bits_destroy(&bits);
   free(backing);
   return 0;
}
