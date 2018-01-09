#define SS_DOCID_BITS 128
#include <iostream>
#include <unistd.h>
#include "stdlib.h"
#include "string.h"
#include <string>  
#include <fstream>  
#include <streambuf>
#include <sstream>
#include <getopt.h>
#include "time.h"
#include "iconv.h" 
#include "lz4.h"
#include "python2.6/Python.h"

using namespace std;

struct option long_options[] = {
    { "compress", 2, NULL, 'c'},
    { "decompress", 2, NULL, 'd'},
    { "manual", 0, NULL, 'm'}
};

#pragma pack(1)
struct compress_result{
    char dataType[8];
    int originalSize;
    int contentSize;
    char data[0];
};
#pragma pack()

int s_iconv(const std::string &src, std::string &result,std::string in,std::string out)    
{
    int max_len = src.length()*2+1;
    char *buf = new char[max_len];
    if (buf == NULL)
        return -1; 
    iconv_t cd = iconv_open(out.c_str(),in.c_str());
    char *inbuff = const_cast<char *>(src.c_str());
    size_t inbytesleft = src.size();
    char *outbuff = buf;
    size_t outbytesleft = max_len;
    size_t ret = iconv(cd, &inbuff, &inbytesleft, &outbuff, &outbytesleft);
    if(ret == size_t(-1))
    {   
        printf("iconv failed:%s,len=%d\n", src.c_str(),src.length());
    }   
    else
    {   
        size_t content_len = max_len - outbytesleft;
        result.assign(buf,content_len);
    }   
    iconv_close(cd);
    delete []buf;
    return ret;
}

extern "C"
std::string compress(std::string src) {
    std::string outxml = "";
    std::string compressxml = "";
    int src_size = src.size();
    const size_t max_dst_size = LZ4_compressBound(src_size);
    char *compressed_data = (char*)malloc(max_dst_size);
    int return_value = 0;
    if(compressed_data != NULL)
    {
        memset(compressed_data, 0 , max_dst_size);
        return_value = LZ4_compress_fast(src.c_str(), compressed_data, src_size, max_dst_size, 1);
        if(return_value > 0)
        {
            compressxml.assign(compressed_data, return_value);
        }
        free(compressed_data);
    }

    compress_result* result = (compress_result*) malloc( sizeof(compress_result) + compressxml.length());
    if(result != NULL)
    {
        memcpy(result->dataType, "COMPRESS", 8);
        result->originalSize = (int)src.size();
        result->contentSize = compressxml.length();
        memcpy(result->data, compressxml.c_str(), result->contentSize);
    
        outxml.assign((char *)result, sizeof(compress_result) + result->contentSize);
    
        free(result);
        result = NULL;
    }
    return outxml;
}



std::string inline decompress_with_length(std::string src, int decompressLen) {
    int src_size = src.size();
    const size_t max_dst_size = decompressLen + 1;
    char *decompressed_data = (char*)malloc(max_dst_size);
    int return_value = 0;
    std::string ret;
    if(decompressed_data != NULL)
    {
        memset(decompressed_data, 0 , max_dst_size);
        return_value = LZ4_decompress_safe(src.c_str(), decompressed_data, src_size, max_dst_size);
        if(return_value > 0)
        {
            ret.assign(decompressed_data, return_value);
        }
        free(decompressed_data);
    }
    return ret;
}


extern "C"
std::string decompress(std::string src)
{
    const char* srcIndex = src.c_str();
    int srcLength = src.length();                           
    int unCompressedLen = 0, compressLen = 0;
    std::string judgeType = "COMPRESS";
    int judgeLen = judgeType.length();
    if(srcLength > judgeLen)
    {       
        compress_result* result = (compress_result*)(src.c_str());
        std::string dataType(result->dataType, judgeLen);
        if(dataType == judgeType)
        {
            //获取数据int长度;
            memcpy(&unCompressedLen, &(result->originalSize), sizeof(int));
            memcpy(&compressLen, &(result->contentSize), sizeof(int));
            if(unCompressedLen == 0)
            {
                return "";
            }
            else
            {
                std::string data = "";
                data.assign(result->data, compressLen);
                return decompress_with_length(data, unCompressedLen);
            }
        }
        else
        {
            return src;
        }
    }
    else
    {                       
        return src;                     
    }
}

static PyObject * _decompress(PyObject *self, PyObject *args)
{
    char* _a; 
    int i;
    std::string res;
    if (!PyArg_ParseTuple(args, "s#", &_a, &i))
        return NULL;
    std::string input(_a,i);
    res = decompress(input);
    return PyString_FromStringAndSize(res.c_str(), res.size());
}

static PyObject * _compress(PyObject *self, PyObject *args)
{
    char* _a;
    int i;
    std::string res;

    if (!PyArg_ParseTuple(args, "s#", &_a, &i))
        return NULL;
    std::string input(_a,i);
    res = compress(input);
    return PyString_FromStringAndSize(res.c_str(), res.size());
}

static PyMethodDef Methods[] = {
    {
        "compress",
        _compress,
        METH_VARARGS,
        ""
    },
    {
        "decompress",
        _decompress,
        METH_VARARGS,
        ""
    },
    {NULL, NULL, 0, NULL}
};

PyMODINIT_FUNC initinstantlz4(void) {
    (void) Py_InitModule("instantlz4", Methods);
}


int main(int argc, char *argv[]) 
{
    std::string compressdata;
    std::string decompressdata;
    std::string dout,cout;
    FILE * fp;
    int size;
    char c;
    char buf[1024];
    const char * const short_options = "c:d:m";
    while((c = getopt_long (argc, argv, short_options, long_options, NULL)) != -1)
    {
        printf("argument:%s\n",optarg);
                
        switch(c)
        {
            case 'c':
                compressdata.assign(optarg);
                cout = compress(compressdata);
                fp = fopen("filename","w");
                fwrite(cout.c_str(), cout.length(), 1, fp);
                printf("save to file:%s\n","filename");
                fclose(fp);
                break;
            case 'd':
                // decompressdata.assign(optarg);
                fp = fopen(optarg,"r");
                while((size = fread(buf, 1, 1024, fp)) != 0) {
                    decompressdata.append(buf, size);
                }
                fclose(fp);

                dout = decompress(decompressdata);
                // s_iconv(dout,dest,"utf-16","gb18030//ignore");
                printf("the file %s means:%s\n",optarg, (char*)dout.c_str());
                break;
	        case 'm':
                printf("%s","--help\ncommand: ./instantlz4 -ccompdata | -ddecompdata");
                break;
        }
    }
    return 0;
}
