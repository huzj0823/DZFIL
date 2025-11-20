/*
 * @Author: gaoy
 * @Date: 2023-03-09 00:06:14
 * @LastEditors: gaoy
 * @LastEditTime: 2023-03-09 00:37:39
 * @Description: file content
 */
#include <stdio.h>
#include <zlib.h>
#include "compress-fpga.h"
#include <cstring>


void gunzip(char* uncompressed_data, int &uncompressed_size, char* compressed_data, int compressed_size) {
    z_stream zs;                        
    memset(&zs, 0, sizeof(zs));
    // 用 zlib 解压 gzip 要用 init2
    if (inflateInit2(&zs, 16+MAX_WBITS) != Z_OK) {
        std::cerr << "inflateInit2 failed while decompressing." << std::endl;
        return;
    }

    zs.next_in = (Bytef *)compressed_data;
    zs.avail_in = (uInt)compressed_size;
    zs.next_out = (Bytef *)uncompressed_data;
    zs.avail_out = (uInt)uncompressed_size;

    int ret = inflate(&zs, Z_FINISH);
    if (ret != Z_STREAM_END) {
        inflateEnd(&zs);
        std::cerr << "Failed to decompress the data: " << ret << std::endl;
        return;
    }
    uncompressed_size = uncompressed_size - zs.avail_out; 

    inflateEnd(&zs);
}


 
int main(int argc,char **args) {
    // source data
    unsigned char strsrc[] = "这些是测试数据。123456789 abcdefghigklmnopqrstuvwxyz\n\t\0abcdefghijklmnopqrstuvwxyz\n"; //包含\0字符
    unsigned long srclen = sizeof(strsrc);
    
    // zlib compress buffer
    unsigned char buf_zlib[1024] = {0};
    unsigned long buflen_zlib = sizeof(buf_zlib);

    // qz compress buffer
    // unsigned char buf_qz[1024] = {0};
    char* buf_qz = NULL;
    int buflen_qz;

    // qz compress without header buffer
    char buf_deflate[1024] = {0};
    int buflen_deflate = sizeof(buf_deflate);
    
    // zlib decompress buffer
    unsigned char strdst_zlib[1024] = {0};
    unsigned long dstlen_zlib = sizeof(strdst_zlib);
    
    // qz decompress buffer
    char strdst_qz[1024] = {0};
    int dstlen_qz = sizeof(strdst_qz);

    // qz decompress without header buffer
    char strdst_deflate[1024] = {0};
    int dstlen_deflate = sizeof(strdst_deflate);

    
    
    int i;
    FILE *fp = NULL;
 
    printf("Source string:\n");
    for(i=0;i<srclen;++i) {
        printf("%c", strsrc[i]);
    }
    printf("\nData length before compression: %ld\n", srclen); 
    printf("Compress bound: %ld\n",compressBound(srclen));
	
    // compress zlib
    compress(buf_zlib, &buflen_zlib, strsrc, srclen);
    printf("\n****** Zlib(Gzip) compression finished ******\n");

	printf("buf start, buflen = %ld\n", buflen_zlib);
    for(i=0; i<buflen_zlib; ++i) {
        printf("%02X", buf_zlib[i]);
    }	
	printf("\nbuf end\n");

    // compress qz
    compressdata_fpga(&buf_qz, &buflen_qz, (char *)strsrc, srclen);
    printf("\n****** QZ compression finished ******\n");
	printf("buf start, buflen = %d\n", buflen_qz);
    for(i=0; i<buflen_qz; ++i) {
        printf("%02X", buf_qz[i]);
    }	
	printf("\nbuf end\n");
	
	// compress deflate(tailor)
    buflen_deflate = buflen_qz-12-8;
    memcpy((char*)buf_deflate, (char*)buf_qz+12, buflen_deflate);
    printf("\n****** QZ deflate tailor finished ******\n");
    printf("buf start, buflen = %d\n", buflen_deflate);
    for(i=0; i<buflen_deflate; ++i) {
        printf("%02X", buf_deflate[i]);
    }	
	printf("\nbuf end\n");
    
    
    // decompress zlib
    uncompress(strdst_zlib, &dstlen_zlib, buf_zlib, buflen_zlib);
    printf("\n****** Zlib(Gzip) decompress finished ****** \n");
 
    printf("Zlib(Gzip) decompression string:\n");
    for(i=0; i<dstlen_zlib; ++i) {
        printf("%c", strdst_zlib[i]);
    }

    // decompress qz
    gunzip(strdst_qz, dstlen_qz, buf_qz, buflen_qz);
    printf("\n****** QZ decompress finished ******\n");
 
    printf("QZ decompression string:\n");
    for(i=0;i<dstlen_qz;++i) {
        printf("%c", strdst_qz[i]);                                        
    }

    // decompress deflate
    // 去掉 gzip 文件头和文件尾的压缩数据也可以按照与完整的 gz 压缩文件相同的方式解压出来
    gunzip(strdst_deflate, dstlen_deflate, buf_deflate, buflen_deflate);
    printf("\n****** QZ inflate decompress finished ****** \n");
 
    printf("QZ inflate decompression string:\n");
    for(i=0; i<dstlen_deflate; ++i) {
        printf("%c", strdst_zlib[i]);
    }

    if(buf_qz) {
        free(buf_qz);
        printf("\nFree mem pointer buf_qz\n");
    }
        

 
    return 0;
}
