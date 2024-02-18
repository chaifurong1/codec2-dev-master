/*---------------------------------------------------------------------------*\

  FILE........: c2demo.c
  AUTHOR......: David Rowe
  DATE CREATED: 15/11/2010

  Encodes and decodes a file of raw speech samples using Codec 2.
  Demonstrates use of Codec 2 function API.
  
  cd codec2/build_linux
  ./demo/c2demo ../raw/hts1a.raw his1a_out.raw
  aplay -f S16_LE hts1a_out.raw
  
\*---------------------------------------------------------------------------*/

/*
  Copyright (C) 2010 David Rowe

  All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU Lesser General Public License version 2.1, as
  published by the Free Software Foundation.  This program is
  distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
  License for more details.

  You should have received a copy of the GNU Lesser General Public License
  along with this program; if not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <stdlib.h>
#include "codec2.h"

int main(int argc, char *argv[])
{
    struct CODEC2 *codec2;
    FILE          *fin;
    FILE          *fout;

    if (argc != 3) {
        printf("usage: %s InputRawSpeechFile OutputRawSpeechFile\n", argv[0]);
        exit(1);
    }   /*检查命令行参数数量是否为3，如果不是则输出提示信息，说明正确的用法，并退出程序。*/

    if ( (fin = fopen(argv[1],"rb")) == NULL ) {
        fprintf(stderr, "Error opening input speech file: %s\n", argv[1]);
        exit(1);
    }

    if ( (fout = fopen(argv[2],"wb")) == NULL ) {
        fprintf(stderr, "Error opening output speech file: %s\n", argv[2]);
         exit(1);
    }  /*打开输入文件和输出文件，如果文件打开失败，则输出错误信息，同时退出程序。*/

    /* Note only one set of Codec 2 states is required for an encoder
       and decoder pair. */
    codec2 = codec2_create(CODEC2_MODE_1300);  /*创建一个Codec 2对象，选择了 CODEC2_MODE_1300 模式，这是Codec 2库支持的一个特定音频编码模式。*/
    size_t nsam = codec2_samples_per_frame(codec2);
    short speech_samples[nsam];
    /* Bits from the encoder are packed into bytes */
    unsigned char compressed_bytes[codec2_bytes_per_frame(codec2)];
	/*获取音频帧的样本数量和每帧的压缩字节数，并分别定义了一个用于存储音频样本的数组和一个用于存储压缩字节的数组。*/

    while(fread(speech_samples, sizeof(short), nsam, fin) == nsam) {
        codec2_encode(codec2, compressed_bytes, speech_samples);
        codec2_decode(codec2, speech_samples, compressed_bytes);
        fwrite(speech_samples, sizeof(short), nsam, fout);
    }
	/*从输入文件读取音频样本，使用Codec 2库对音频进行编码，然后解码，最后将解码后的音频样本写入输出文件。这个过程一直循环进行，直到无法再读取到完整的音频帧。*/

    codec2_destroy(codec2);
    fclose(fin);
    fclose(fout);
	/*在程序结束时销毁Codec 2对象并关闭输入、输出文件。*/
    return 0;
}
