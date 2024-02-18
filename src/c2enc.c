/*---------------------------------------------------------------------------*\

  FILE........: c2enc.c
  AUTHOR......: David Rowe
  DATE CREATED: 23/8/2010

  Encodes a file of raw speech samples using codec2 and outputs a file
  of bits.

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

#include "codec2.h"
#include "c2file.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

int main(int argc, char *argv[])
{
    int            mode;
    void          *codec2;
    FILE          *fin;
    FILE          *fout;
    short         *buf;
    unsigned char *bits;
    int            nsam, nbit, nbyte, gray, softdec, bitperchar;
    float         *unpacked_bits_float;
    char          *unpacked_bits_char;
    int            bit, byte,i;
    int            report_var = 0;
    int            eq = 0;
    
    if (argc < 4) {
	printf("usage: c2enc 3200|2400|1600|1400|1300|1200|900|700C|450|450PWB InputRawspeechFile OutputBitFile [--natural] [--softdec] [--bitperchar] [--mlfeat f32File modelFile] [--loadcb stageNum Filename] [--var] [--eq]\n");
	printf("e.g. (headerless)    c2enc 1300 ../raw/hts1a.raw hts1a.bin\n");
	printf("e.g. (with header to detect mode)   c2enc 1300 ../raw/hts1a.raw hts1a.c2\n");
	exit(1);
    }  /*检查命令行参数数量是否符合要求，如果不符合则输出用法信息，然后退出程序。*/

    if (strcmp(argv[1],"3200") == 0)   // strcmp 是 C 语言标准库 <string.h> 中的一个函数，用于比较两个字符串的内容是否相等。
	mode = CODEC2_MODE_3200;
    else if (strcmp(argv[1],"2400") == 0)
	mode = CODEC2_MODE_2400;
    else if (strcmp(argv[1],"1600") == 0)
	mode = CODEC2_MODE_1600;
    else if (strcmp(argv[1],"1400") == 0)
	mode = CODEC2_MODE_1400;
    else if (strcmp(argv[1],"1300") == 0)
	mode = CODEC2_MODE_1300;
    else if (strcmp(argv[1],"1200") == 0)
	mode = CODEC2_MODE_1200;
	else if (strcmp(argv[1],"900") == 0)
	mode = CODEC2_MODE_900;
    else if (strcmp(argv[1],"700C") == 0)
	mode = CODEC2_MODE_700C;
    else if (strcmp(argv[1],"450") == 0)
	mode = CODEC2_MODE_450;
    else if (strcmp(argv[1],"450PWB") == 0)
	mode = CODEC2_MODE_450;
    else {
	fprintf(stderr, "Error in mode: %s.  Must be 3200, 2400, 1600, 1400, 1300, 1200, 900, 700C, 450, 450PWB or WB\n", argv[1]);
	exit(1);
    }  //根据命令行参数选择Codec 2音频编码模式。

	/*打开输入和输出文件：打开输入和输出文件，如果文件打开失败，则输出错误信息，同时退出程序。如果输入输出文件参数为'-'，则使用标准输入输出流。*/
    if (strcmp(argv[2], "-")  == 0) fin = stdin;
    else if ( (fin = fopen(argv[2],"rb")) == NULL ) {
	fprintf(stderr, "Error opening input speech file: %s: %s.\n",
         argv[2], strerror(errno));  // 打开输入文件失败，输出错误信息，然后退出程序
	exit(1);
    }

    if (strcmp(argv[3], "-") == 0) fout = stdout;
    else if ( (fout = fopen(argv[3],"wb")) == NULL ) {
	fprintf(stderr, "Error opening output compressed bit file: %s: %s.\n",
         argv[3], strerror(errno));// 打开输出文件失败，输出错误信息，然后退出程序
	exit(1);
    }  
    
    // Write a header if we're writing to a .c2 file
	/* 写入输出文件头部信息：如果输出文件的扩展名为.c2，写入一个头部信息，该信息包含了Codec 2的模式、版本等信息。*/
    char *ext = strrchr(argv[3], '.');
    if (ext != NULL) {
        if (strcmp(ext, ".c2") == 0) { 
            struct c2_header out_hdr;
            memcpy(out_hdr.magic,c2_file_magic,sizeof(c2_file_magic));
            out_hdr.mode = mode;
            out_hdr.version_major = CODEC2_VERSION_MAJOR;
            out_hdr.version_minor = CODEC2_VERSION_MINOR;
            // TODO: Handle flags (this block needs to be moved down)
            out_hdr.flags = 0;
            fwrite(&out_hdr,sizeof(out_hdr),1,fout);
        };
    };

    codec2 = codec2_create(mode); // Codec 2初始化：创建Codec 2对象，选择了之前确定的音频编码模式。
	/*内存分配：获取音频帧的样本数量和每帧的比特数，然后分配内存用于存储音频样本、编码比特、解码后的浮点数比特和解码后的字符比特。*/
    nsam = codec2_samples_per_frame(codec2);
    nbit = codec2_bits_per_frame(codec2);
    buf = (short*)malloc(nsam*sizeof(short));
    nbyte = (nbit + 7) / 8;

    bits = (unsigned char*)malloc(nbyte*sizeof(char));
    unpacked_bits_float = (float*)malloc(nbit*sizeof(float));
    unpacked_bits_char = (char*)malloc(nbit*sizeof(char));

	/*命令行参数处理：根据命令行参数设置一些程序的选项，比如是否使用灰度编码、是否使用软解码、是否按字符存储比特等。*/
    gray = 1; softdec = 0; bitperchar = 0;
    for (i=4; i<argc; i++) {
        if (strcmp(argv[i], "--natural") == 0) {
            gray = 0;
        }
        if (strcmp(argv[i], "--softdec") == 0) {
            softdec = 1;
        }
        if (strcmp(argv[i], "--bitperchar") == 0) {
            bitperchar = 1;
        }
        if (strcmp(argv[i], "--mlfeat") == 0) {
            /* dump machine learning features (700C only) */
            codec2_open_mlfeat(codec2, argv[i+1], argv[i+2]);
        }
        if (strcmp(argv[i], "--loadcb") == 0) {
            /* load VQ stage (700C only) */
            codec2_load_codebook(codec2, atoi(argv[i+1])-1, argv[i+2]);
        }
        if (strcmp(argv[i], "--var") == 0) {
            report_var = 1;
        }
        if (strcmp(argv[i], "--eq") == 0) {
            eq = 1;
        }
        
    }
    codec2_set_natural_or_gray(codec2, gray);
    codec2_700c_eq(codec2, eq);
    
    //fprintf(stderr,"gray: %d softdec: %d\n", gray, softdec);
	/*主编码循环：读取输入文件的音频样本，使用Codec 2库进行编码，然后根据选项进行相应的输出操作。*/
    while(fread(buf, sizeof(short), nsam, fin) == (size_t)nsam) {

	codec2_encode(codec2, bits, buf);

	if (softdec || bitperchar) { //如果使用软解码或按字符存储比特，进行解包操作
            /* unpack bits, MSB first, send as soft decision float */

            bit = 7; byte = 0;
            for(i=0; i<nbit; i++) {
                unpacked_bits_float[i] = 1.0 - 2.0*((bits[byte] >> bit) & 0x1);
                unpacked_bits_char[i] = (bits[byte] >> bit) & 0x1;
                bit--;
                if (bit < 0) {
                    bit = 7;
                    byte++;
                }
            }
            if (softdec) {
                fwrite(unpacked_bits_float, sizeof(float), nbit, fout);
            }
            if (bitperchar) {
                fwrite(unpacked_bits_char, sizeof(char), nbit, fout);
            }
        }
        else // 直接写入编码比特
            fwrite(bits, sizeof(char), nbyte, fout);

	    // if this is in a pipeline, we probably don't want the usual
        // buffering to occur
	    // 如果输出到stdout，刷新输出缓冲区
        if (fout == stdout) fflush(stdout);
    }
	//如果选择了--var选项，输出音频样本方差及标准差的报告。
    if (report_var) {
        float var = codec2_get_var(codec2);
        fprintf(stderr, "%s var: %5.2f std: %5.2f\n", argv[2], var, sqrt(var));
    }
    codec2_destroy(codec2);
	//在程序结束时销毁Codec 2对象并释放分配的内存，关闭输入、输出文件。
    free(buf);
    free(bits);
    free(unpacked_bits_float);
    free(unpacked_bits_char);
    fclose(fin);
    fclose(fout);

    return 0;
}
