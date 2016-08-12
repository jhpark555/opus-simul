/* Copyright (c) 2013 Jean-Marc Valin */
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
/* This is meant to be a simple example of encoding and decoding audio
   using Opus. It should make it easy to understand how the Opus API
   works. For more information, see the full API documentation at:
   http://www.opus-codec.org/docs/ */
   
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include "opus.h"
#include "opus_private.h"

/*The frame size is hardcoded for this sample code but it doesn't have to be*/
#define FRAME_SIZE 960
#define SAMPLE_RATE 48000
#define CHANNELS 2
#define COMPLEXITY 10
#define APPLICATION  OPUS_APPLICATION_RESTRICTED_LOWDELAY
#define BITRATE 256000
#define MAX_FRAME_SIZE 6*960
#define MAX_PACKET_SIZE (3*1276)

static opus_uint32 char_to_int(unsigned char ch[4])
{
    return ((opus_uint32)ch[0]<<24) | ((opus_uint32)ch[1]<<16)
         | ((opus_uint32)ch[2]<< 8) |  (opus_uint32)ch[3];
}

int main(int argc, char **argv)
{
   char *inFile;
   FILE *fin;
   char *outFile;
   FILE *fout;
   opus_int16 in[FRAME_SIZE*CHANNELS];
   opus_int16 out[MAX_FRAME_SIZE*CHANNELS];
   unsigned char cbits[MAX_PACKET_SIZE];
   int nbBytes;
   /*Holds the state of the encoder and decoder */
   OpusEncoder *encoder;
   OpusDecoder *decoder;
   int err;
   
   extern unsigned char pcm_stream[];
   unsigned char *data_ptr;
   int  data_count;  

   
   int i;
   unsigned char pcm_bytes[MAX_FRAME_SIZE*CHANNELS*2]={0};
   int frame_size;
   int MAX_data;
   
   //if (argc != 3)
   //{
   //   fprintf(stderr, "usage: trivial_example input.pcm output.pcm\n");
   //   fprintf(stderr, "input and output are 16-bit little-endian raw files\n");
   //   return EXIT_FAILURE;
   //}
   
   /*Create a new encoder state */
   encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, APPLICATION, &err);
   if (err<0)
   {
      fprintf(stderr, "failed to create an encoder: %s\n", opus_strerror(err));
      return EXIT_FAILURE;
   }
   /* Set the desired bit-rate. You can also set other parameters if needed.
      The Opus library is designed to have good defaults, so only set
      parameters you know you need. Doing otherwise is likely to result
      in worse quality, but better. */
      
   opus_encoder_ctl(encoder, OPUS_SET_BITRATE(BITRATE));
   opus_encoder_ctl(encoder, OPUS_SET_BITRATE(256000));	 //256k
   opus_encoder_ctl(encoder, OPUS_SET_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND));
   opus_encoder_ctl(encoder, OPUS_SET_VBR(1));
   opus_encoder_ctl(encoder, OPUS_SET_COMPLEXITY(COMPLEXITY ));  //level 10
   opus_encoder_ctl(encoder, OPUS_SET_MAX_BANDWIDTH(OPUS_BANDWIDTH_FULLBAND)); // full band? ? ???? ???.
   opus_encoder_ctl(encoder, OPUS_SET_SIGNAL(OPUS_SIGNAL_MUSIC)); // music mode? ?????.
   opus_encoder_ctl(encoder, OPUS_SET_APPLICATION(OPUS_APPLICATION_RESTRICTED_LOWDELAY)); // ??? OPUS_APPLICATION_RESTRICTED_LOWDELAY? ?????. ?? ?? ?? ????.
   opus_encoder_ctl(encoder, OPUS_SET_INBAND_FEC(0));
   opus_encoder_ctl(encoder, OPUS_SET_DTX(0));
   opus_encoder_ctl(encoder, OPUS_SET_PACKET_LOSS_PERC(0));
   opus_encoder_ctl(encoder, OPUS_SET_LSB_DEPTH(24));
   opus_encoder_ctl(encoder, OPUS_SET_EXPERT_FRAME_DURATION(OPUS_FRAMESIZE_2_5_MS)); // 2.5ms, ?? ???? ?? ?? ??????.
   opus_encoder_ctl(encoder, OPUS_SET_PREDICTION_DISABLED(0));
   opus_encoder_ctl(encoder, OPUS_SET_FORCE_MODE(MODE_CELT_ONLY));

   inFile =  "check.pcm";//argv[1];
   fin = fopen(inFile, "r");
   if (fin==NULL)
   {
      fprintf(stderr, "failed to open input file: %s\n", strerror(errno));
      return EXIT_FAILURE;
   }
   /* Create a new decoder state. */
   decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &err);
   if (err<0)
   {
      fprintf(stderr, "failed to create decoder: %s\n", opus_strerror(err));
      return EXIT_FAILURE;
   }
   outFile = "test1.pcm";//argv[2];
   fout = fopen(outFile, "w");
   if (fout==NULL)
   {
      fprintf(stderr, "failed to open output file: %s\n", strerror(errno));
      return EXIT_FAILURE;
   }

 #if 1  
    data_ptr= pcm_stream;
    data_count=0;
#endif  
   while (1)
   {
      /* Read a 16 bits/sample audio frame. */
#if 0  
      fread(pcm_bytes, sizeof(short)*CHANNELS, FRAME_SIZE, fin);
      if (feof(fin))
	  	 break;
#else
   for (i=0;i<sizeof(short)*CHANNELS*FRAME_SIZE;i++){
	   	pcm_bytes[i]=*data_ptr++;	
        data_count++;
		if(*data_ptr =='\0') MAX_data=data_count;
#if 0
		if(i%16==0) printf("\n");	
	    printf("%8x ",pcm_bytes[i]);
#endif				
   	}    

#endif	  	
      /* Convert from little-endian ordering. */
    for (i=0;i<CHANNELS*FRAME_SIZE;i++){
         in[i]=pcm_bytes[2*i+1]<<8|pcm_bytes[2*i];
#if 0	
		 if(i%16==0) printf("\n");	 
	     printf("%8x ",in[i]);
#endif		 
     }
      	
      /* Encode the frame. */
      nbBytes = opus_encode(encoder, in, FRAME_SIZE, cbits, MAX_PACKET_SIZE);
      if (nbBytes<0)
      {
         fprintf(stderr, "encode failed: %s\n", opus_strerror(nbBytes));
         return EXIT_FAILURE;
      }
#if 0 
	  for (i=0;i<CHANNELS*FRAME_SIZE;i++){
	  	if(i%16==0) printf("\n");	
		printf("%8x ",cbits[i]);
	  	}
#endif			  
      /* Decode the data. In this example, frame_size will be constant because
         the encoder is using a constant frame size. However, that may not
         be the case for all encoders, so the decoder must always check
         the frame size returned. */
         
      frame_size = opus_decode(decoder, cbits, nbBytes, out, MAX_FRAME_SIZE, 0);
      if (frame_size<0)
      {
         fprintf(stderr, "decoder failed: %s\n", opus_strerror(frame_size));
         return EXIT_FAILURE;
      }
#if 0 
			for (i=0;i<CHANNELS*frame_size;i++){
			  if(i%16==0) printf("\n");				
			  printf("%8x ",out[i]);
			  }
#endif			  
	  
      /* Convert to little-endian ordering. */
      for(i=0;i<CHANNELS*frame_size;i++)
      {
         pcm_bytes[2*i]=out[i]&0xFF;
         pcm_bytes[2*i+1]=(out[i]>>8)&0xFF;
      }
#if  1	  
	 for(i=0;i<CHANNELS*frame_size;i++){
	  if(i%16==0) printf("\n");	
	  printf("%8x ",pcm_bytes[i]);
	 }
#endif	 
	  
      /* Write the decoded audio to file. */
      fwrite(pcm_bytes, sizeof(short), frame_size*CHANNELS, fout);

   if( *data_ptr=='\n' || data_count>0xe96) {
   printf("data_count=%x  \n",data_count);
   break;
   }

   }

   
   /*Destroy the encoder state*/
   opus_encoder_destroy(encoder);
   opus_decoder_destroy(decoder);
   fclose(fin);
   fclose(fout);
   return EXIT_SUCCESS;
}
