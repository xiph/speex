/* Copyright (C) 2019 Tristan Matthews
   File: speexdec_fuzzer.cc (based on speexdec.c)

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

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <ogg/ogg.h>

#include <math.h>

#include <string.h>
#include "speex/speex_header.h"
#include "speex/speex_stereo.h"
#include "speex/speex_callbacks.h"

#define MAX_FRAME_SIZE 2000

static void *process_header(ogg_packet *op, spx_int32_t enh_enabled, spx_int32_t *frame_size, int *granule_frame_size, spx_int32_t *rate, int *nframes, int *channels, SpeexStereoState *stereo, int *extra_headers)
{
   void *st;
   const SpeexMode *mode;
   SpeexHeader *header;
   int modeID;
   SpeexCallback callback;

   header = speex_packet_to_header((char*)op->packet, op->bytes);
   if (!header)
   {
      fprintf (stderr, "Cannot read header\n");
      return NULL;
   }
   if (header->mode >= SPEEX_NB_MODES || header->mode<0)
   {
      fprintf (stderr, "Mode number %d does not (yet/any longer) exist in this version\n",
               header->mode);
      free(header);
      return NULL;
   }

   modeID = header->mode;

   mode = speex_lib_get_mode (modeID);

   if (header->speex_version_id > 1)
   {
      fprintf (stderr, "This file was encoded with Speex bit-stream version %d, which I don't know how to decode\n", header->speex_version_id);
      free(header);
      return NULL;
   }

   if (mode->bitstream_version < header->mode_bitstream_version)
   {
      fprintf (stderr, "The file was encoded with a newer version of Speex. You need to upgrade in order to play it.\n");
      free(header);
      return NULL;
   }
   if (mode->bitstream_version > header->mode_bitstream_version)
   {
      fprintf (stderr, "The file was encoded with an older version of Speex. You would need to downgrade the version in order to play it.\n");
      free(header);
      return NULL;
   }

   st = speex_decoder_init(mode);
   if (!st)
   {
      fprintf (stderr, "Decoder initialization failed.\n");
      free(header);
      return NULL;
   }
   speex_decoder_ctl(st, SPEEX_SET_ENH, &enh_enabled);
   speex_decoder_ctl(st, SPEEX_GET_FRAME_SIZE, frame_size);
   *granule_frame_size = *frame_size;

   if (!*rate)
      *rate = header->rate;

   speex_decoder_ctl(st, SPEEX_SET_SAMPLING_RATE, rate);

   *nframes = header->frames_per_packet;

   if (*channels==-1)
      *channels = header->nb_channels;

   if (!(*channels==1))
   {
      *channels = 2;
      callback.callback_id = SPEEX_INBAND_STEREO;
      callback.func = speex_std_stereo_request_handler;
      callback.data = stereo;
      speex_decoder_ctl(st, SPEEX_SET_HANDLER, &callback);
   }

   *extra_headers = header->extra_headers;

   free(header);
   return st;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *fuzz_data, size_t fuzz_size)
{
   int c;
   short output[MAX_FRAME_SIZE];
   int frame_size=0, granule_frame_size=0;
   void *st=NULL;
   SpeexBits bits;
   int packet_count=0;
   int stream_init = 0;
   ogg_int64_t page_granule=0, last_granule=0;
   int skip_samples=0, page_nb_packets;
   ogg_sync_state oy;
   ogg_page       og;
   ogg_packet     op;
   ogg_stream_state os;
   int enh_enabled;
   int nframes=2;
   int print_bitrate=0;
   int close_in=0;
   int eos=0;
   int audio_size=0;
   SpeexStereoState stereo = SPEEX_STEREO_STATE_INIT;
   int channels=-1;
   int rate=0;
   int extra_headers=0;
   int lookahead;
   int speex_serialno = -1;

   enh_enabled = 1;

   /*Init Ogg data struct*/
   ogg_sync_init(&oy);

   speex_bits_init(&bits);
   /*Main decoding loop*/

   ssize_t bytes_remaining = fuzz_size;
   while (1)
   {
      char *data;
      int i, j, nb_read;
      /*Get the ogg buffer for writing*/
      nb_read = bytes_remaining > 200 ? 200 : bytes_remaining;
      data = ogg_sync_buffer(&oy, nb_read);
      /*Read bitstream from data*/
      memcpy(data, fuzz_data, nb_read);
      ogg_sync_wrote(&oy, nb_read);
      bytes_remaining -= nb_read;

      /*Loop for all complete pages we got (most likely only one)*/
      while (ogg_sync_pageout(&oy, &og)==1)
      {
         int packet_no;
         if (stream_init == 0) {
            ogg_stream_init(&os, ogg_page_serialno(&og));
            stream_init = 1;
         }
         if (ogg_page_serialno(&og) != os.serialno) {
            /* so all streams are read. */
            ogg_stream_reset_serialno(&os, ogg_page_serialno(&og));
         }
         /*Add page to the bitstream*/
         ogg_stream_pagein(&os, &og);
         page_granule = ogg_page_granulepos(&og);
         page_nb_packets = ogg_page_packets(&og);
         if (page_granule>0 && frame_size)
         {
            /* FIXME: shift the granule values if --force-* is specified */
            skip_samples = frame_size*(page_nb_packets*granule_frame_size*nframes - (page_granule-last_granule))/granule_frame_size;
            if (ogg_page_eos(&og))
               skip_samples = -skip_samples;
            /*else if (!ogg_page_bos(&og))
               skip_samples = 0;*/
         } else
         {
            skip_samples = 0;
         }
         /*printf ("page granulepos: %d %d %d\n", skip_samples, page_nb_packets, (int)page_granule);*/
         last_granule = page_granule;
         /*Extract all available packets*/
         packet_no=0;
         while (!eos && ogg_stream_packetout(&os, &op) == 1)
         {
            if (op.bytes>=5 && !memcmp(op.packet, "Speex", 5)) {
               speex_serialno = os.serialno;
            }
            if (speex_serialno == -1 || os.serialno != speex_serialno)
               break;
            /*If first packet, process as Speex header*/
            if (packet_count==0)
            {
               st = process_header(&op, enh_enabled, &frame_size, &granule_frame_size, &rate, &nframes, &channels, &stereo, &extra_headers);
               if (!st)
                  exit(1);
               speex_decoder_ctl(st, SPEEX_GET_LOOKAHEAD, &lookahead);
               if (!nframes)
                  nframes=1;

            } else if (packet_count<=1+extra_headers)
            {
               /* Ignore extra headers */
            } else {
               packet_no++;

               /*End of stream condition*/
               if (op.e_o_s && os.serialno == speex_serialno) /* don't care for anything except speex eos */
                  eos=1;

               /*Copy Ogg packet to Speex bitstream*/
               speex_bits_read_from(&bits, (char*)op.packet, op.bytes);
               for (j=0;j!=nframes;j++)
               {
                  int ret;
                  /*Decode frame*/
                  ret = speex_decode_int(st, &bits, output);

                  if (ret==-1)
                     break;
                  if (ret==-2)
                  {
                     fprintf (stderr, "Decoding error: corrupted stream?\n");
                     break;
                  }
                  if (speex_bits_remaining(&bits)<0)
                  {
                     fprintf (stderr, "Decoding overflow: corrupted stream?\n");
                     break;
                  }
                  if (channels==2)
                     speex_decode_stereo_int(output, frame_size, &stereo);

                  {
                     int frame_offset = 0;
                     int new_frame_size = frame_size;
                     /*printf ("packet %d %d\n", packet_no, skip_samples);*/
                     /*fprintf (stderr, "packet %d %d %d\n", packet_no, skip_samples, lookahead);*/
                     if (packet_no == 1 && j==0 && skip_samples > 0)
                     {
                        /*printf ("chopping first packet\n");*/
                        new_frame_size -= skip_samples+lookahead;
                        frame_offset = skip_samples+lookahead;
                     }
                     if (packet_no == page_nb_packets && skip_samples < 0)
                     {
                        int packet_length = nframes*frame_size+skip_samples+lookahead;
                        new_frame_size = packet_length - j*frame_size;
                        if (new_frame_size<0)
                           new_frame_size = 0;
                        if (new_frame_size>frame_size)
                           new_frame_size = frame_size;
                        /*printf ("chopping end: %d %d %d\n", new_frame_size, packet_length, packet_no);*/
                     }
                     if (new_frame_size>0)
                     {
                        audio_size+=sizeof(short)*new_frame_size*channels;
                     }
                  }
               }
            }
            packet_count++;
         }
      }
      if (bytes_remaining <= 0)
         break;
   }

   if (st)
      speex_decoder_destroy(st);
   else
   {
      fprintf (stderr, "This doesn't look like a Speex file\n");
   }
   speex_bits_destroy(&bits);
   if (stream_init)
      ogg_stream_clear(&os);
   ogg_sync_clear(&oy);

   return 0;
}
