/* Copyright (c) 2007-2008 CSIRO
   Copyright (c) 2007-2009 Xiph.Org Foundation
   Written by Jean-Marc Valin */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions
   are met:

   - Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
   ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER
   OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

//#include <stdio.h>
//#include <stdlib.h>
#include "opus.h"
#include "opus_defines.h"

#define MAX_PACKET 1500

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

#define SAMPLE_RATE 48000
#define CHANNELNUM   2

#define NULL 0
#define EXIT_FAILURE 1
#define EXIT_SUCCESS 0

int Opus_Task(void)
{
    int err;
    char *inFile, *outFile;
    //FILE *fin, *fout;
    //OpusEncoder *enc=NULL;
    OpusDecoder *dec=NULL;
   // int args;
    int len[2];
    int frame_size, channels;
    opus_int32 bitrate_bps=0;
    unsigned char *data[2];
    unsigned char *fbytes;
    opus_int32 sampling_rate;
    int use_vbr;
    int max_payload_bytes;
    int complexity;
    int use_inbandfec;
    int use_dtx;
    int forcechannels;
    int cvbr = 0;
    int packet_loss_perc;
    opus_int32 count=0, count_act=0;
    int k;
    opus_int32 skip=0;
    int stop=0;
    short *in, *out;
    int application=OPUS_APPLICATION_AUDIO;
    double bits=0.0, bits_max=0.0, bits_act=0.0, bits2=0.0, nrg;
    double tot_samples=0;
    opus_uint64 tot_in, tot_out;
    int bandwidth=OPUS_AUTO;
    int lost = 0, lost_prev = 1;
    int toggle = 0;
    opus_uint32 enc_final_range[2];
    opus_uint32 dec_final_range;
    int max_frame_size = 48000*2;
    int sweep_bps = 0;
    int random_framesize=0, newsize=0, delayed_celt=0;
    int random_fec=0;
    const int (*mode_list)[4]=NULL;
    int nb_modes_in_list=0; 
    int mode_switch_time = 48000;
    int nb_encoded=0;
    int variable_duration= OPUS_FRAMESIZE_ARG;   

    tot_in=tot_out=0;
    //fprintf(stderr, "%s\n", opus_get_version_string());

   
    sampling_rate = SAMPLE_RATE;    // sample rate define
    
    frame_size = sampling_rate/50;
 
    channels = CHANNELNUM;       // channel = 1 or 2

    /* defaults: */
    use_vbr = 1;
    max_payload_bytes = MAX_PACKET;
    complexity = 10;
    use_inbandfec = 0;
    forcechannels = OPUS_AUTO;
    use_dtx = 0;
    packet_loss_perc = 0;  

    inFile = "input.txt"; 
    //fin = fopen(inFile, "rb");

    if (mode_list)
    {
       int size;
       //fseek(fin, 0, SEEK_END);
       size = //ftell(fin);
       //fprintf(stderr, "File size is %d bytes\n", size);
       //fseek(fin, 0, SEEK_SET);
       mode_switch_time = size/sizeof(short)/channels/nb_modes_in_list;
       //fprintf(stderr, "Switching mode every %d samples\n", mode_switch_time);
    }

    outFile = "out.pcm";
    //fout = fopen(outFile, "wb+");
  
   
       dec = opus_decoder_create(sampling_rate, channels, &err);
       if (err != OPUS_OK)
       {
          //fprintf(stderr, "Cannot create decoder: %s\n", opus_strerror(err));
          //fclose(fin);
          //fclose(fout);
          return EXIT_FAILURE;
       }
       //fprintf(stderr, "Decoding with %ld Hz output (%d channels)\n",
        //               (long)sampling_rate, channels);
                    

    in = (short*)malloc(max_frame_size*channels*sizeof(short));
    out = (short*)malloc(max_frame_size*channels*sizeof(short));
    /* We need to allocate for 16-bit PCM data, but we store it as unsigned char. */
    fbytes = (unsigned char*)malloc(max_frame_size*channels*sizeof(short));
    data[0] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));
    if ( use_inbandfec ) {
        data[1] = (unsigned char*)calloc(max_payload_bytes,sizeof(unsigned char));
    }

    while (!stop)
    {
        if (delayed_celt)
        {
            frame_size = newsize;
            delayed_celt = 0;
        } else if (random_framesize && rand()%20==0)
        {
            newsize = rand()%6;
            switch(newsize)
            {
            case 0: newsize=sampling_rate/400; break;
            case 1: newsize=sampling_rate/200; break;
            case 2: newsize=sampling_rate/100; break;
            case 3: newsize=sampling_rate/50; break;
            case 4: newsize=sampling_rate/25; break;
            case 5: newsize=3*sampling_rate/50; break;
            }
            while (newsize < sampling_rate/25 && bitrate_bps-abs(sweep_bps) <= 3*12*sampling_rate/newsize)
               newsize*=2;
            if (newsize < sampling_rate/100 && frame_size >= sampling_rate/100)
            {
               // opus_encoder_ctl(enc, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));
                delayed_celt=1;
            } else {
                frame_size = newsize;
            }
        }
        if (random_fec && rand()%30==0)
        {
          // opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(rand()%4==0));
        }

            unsigned char ch[4];
           // err = fread(ch, 1, 4, fin);
            //if (feof(fin))
            //    break;
            len[toggle] = char_to_int(ch);
            if (len[toggle]>max_payload_bytes || len[toggle]<0)
            {
               // fprintf(stderr, "Invalid payload length: %d\n",len[toggle]);
                break;
            }
           // err = fread(ch, 1, 4, fin);
            enc_final_range[toggle] = char_to_int(ch);
           // err = fread(data[toggle], 1, len[toggle], fin);
            if (err<len[toggle])
            {
               // fprintf(stderr, "Ran out of input, "
               //                 "expecting %d bytes got %d\n",
               //                 len[toggle],err);
                break;
            }

       
            int output_samples;
            lost = len[toggle]==0 || (packet_loss_perc>0 && rand()%100 < packet_loss_perc);
            if (lost)
               opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION(&output_samples));
            else
               output_samples = max_frame_size;
            if( count >= use_inbandfec ) {
                /* delay by one packet when using in-band FEC */
                if( use_inbandfec  ) {
                    if( lost_prev ) {
                        /* attempt to decode with in-band FEC from next packet */
                        opus_decoder_ctl(dec, OPUS_GET_LAST_PACKET_DURATION(&output_samples));
                        output_samples = opus_decode(dec, lost ? NULL : data[toggle], len[toggle], out, output_samples, 1);
                    } else {
                        /* regular decode */
                        output_samples = max_frame_size;
                        output_samples = (dec, data[1-toggle], len[1-toggle], out, output_samples, 0);
                    }
                } else {
                    output_samples = opus_decode(dec, lost ? NULL : data[toggle], len[toggle], out, output_samples, 0);
                }
                if (output_samples>0)
                {
                     if (output_samples>skip) {
                       int i;
                       for(i=0;i<(output_samples-skip)*channels;i++)
                       {
                          short s;
                          s=out[i+(skip*channels)];
                          fbytes[2*i]=s&0xFF;
                          fbytes[2*i+1]=(s>>8)&0xFF;
                       }
                      // if (fwrite(fbytes, sizeof(short)*channels, output_samples-skip, fout) != (unsigned)(output_samples-skip)){
                         // fprintf(stderr, "Error writing.\n");
                      //    return EXIT_FAILURE;
                     //  }
                       tot_out += output_samples-skip;
                    }
                    if (output_samples<skip) skip -= output_samples;
                    else skip = 0;
                } else {
                   //fprintf(stderr, "error decoding frame: %s\n",
                   //                opus_strerror(output_samples));
                }
                tot_samples += output_samples;
            }

           opus_decoder_ctl(dec, OPUS_GET_FINAL_RANGE(&dec_final_range));
        /* compare final range encoder rng values of encoder and decoder */
        

        lost_prev = lost;
        if( count >= use_inbandfec ) {
            /* count bits */
            bits += len[toggle]*8;
            bits_max = ( len[toggle]*8 > bits_max ) ? len[toggle]*8 : bits_max;
            bits2 += len[toggle]*len[toggle]*64;           
            
        }
        count++;
        toggle = (toggle + use_inbandfec) & 1;
    }

    /* Print out bitrate statistics */ 
        frame_size = (int)(tot_samples / count);
    count -= use_inbandfec;
    //fprintf (stderr, "average bitrate:             %7.3f kb/s\n",
    //                 1e-3*bits*sampling_rate/tot_samples);
    //fprintf (stderr, "maximum bitrate:             %7.3f kb/s\n",
    //                 1e-3*bits_max*sampling_rate/frame_size);

    //fprintf (stderr, "bitrate standard deviation:  %7.3f kb/s\n",
    //        1e-3*sqrt(bits2/count - bits*bits/(count*(double)count))*sampling_rate/frame_size);
    /* Close any files to which intermediate results were stored */
 

    opus_decoder_destroy(dec);
    free(data[0]);
    if (use_inbandfec)
        free(data[1]);
    //fclose(fin);
   // fclose(fout);
    free(in);
    free(out);
    free(fbytes);
    return EXIT_SUCCESS;
}
