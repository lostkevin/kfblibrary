#ifndef __KFBREADER__
#define __KFBREADER__
#include <iostream>
#include <map>
#include <dlfcn.h>
#include <cmath>
#include <vector>
#include <memory>
#include "KFB.h"


using ll=long long int;
using BYTE=unsigned char;
using namespace std;

struct AssoImage {
    int nBytes;
    int width;
    int height;
    shared_ptr<BYTE> buf;

    AssoImage(){
        nBytes = width = height = 0;
        buf = nullptr;
    }

    AssoImage(int nBytes, int width, int height, shared_ptr<BYTE> buf){
        this->nBytes = nBytes;
        this->width = width;
        this->height = height;
        this->buf = buf;
    }

    ~AssoImage()=default;
};

struct ImgHandle {
    void* handle;
    ImageInfoStruct* imgStruct;
    map<string, string> properties;
    int maxLevel;
    int scanScale;
    int width;
    int height;
    const char** assoNames;
    map<string, AssoImage> assoImages;
    vector<BYTE*> alloc_mem;
    bool debug;

    ImgHandle() {
        properties = map<string, string>();
        assoImages = map<string, AssoImage>();
        alloc_mem = vector<BYTE*>();
        handle = nullptr;
        imgStruct = new ImageInfoStruct;
        maxLevel = 0;
        scanScale = 0;
        width = height = 0;
        assoNames = nullptr;
        debug = false;
    }

    ~ImgHandle() {
        delete this->imgStruct;
        if(assoNames) delete [] assoNames;
        if(debug)
            cout << "free " << alloc_mem.size() << " objects" << endl;
        for(BYTE* ptr:alloc_mem) delete [] ptr;
        dlclose(handle);
    }

};

// Input: dll library path and KFB Slide Path, allocate and return a new reader
ImgHandle* kfbslide_open(const char * dllPath, const char* filename);

// Input: close reader and release all memory
void kfbslide_close(ImgHandle* s);

// 逆向: 返回"kfbio"字符串
const char * kfbslide_detect_vendor(const char *);

// names: 返回字符串数组(nullptr end), 表示WSI拥有的属性
// 逆向: 固定值 "openslide.mpp-x", "openslide.mpp-t", "openslide.vendor"
const char** kfbslide_property_names(ImgHandle* preader);
// 查询对应的value
// 逆向: mpp为headerInfo.CapRes, vendor为Kfbio
const char * kfbslide_property_value(ImgHandle* preader, const char * attribute_name);

// 逆向: 返回level对应的下采样率
double kfbslide_get_level_downsample(ImgHandle* s, int level);
// 逆向: 返回最接近scale_factor的level
int kfbslide_get_best_level_for_downsample(ImgHandle* s, double scale_factor);
// 逆向: 返回log2(header.BlockSize)
int kfbslide_get_level_count(ImgHandle* s);
// 逆向: 向width和height指向的内存写入当前level的大小: [width >> level, height >> level]
ll kfbslide_get_level_dimensions(ImgHandle* s, int level, ll* width, ll* height);

// 逆向: 根据name返回一个图像数据流(jpg格式)
BYTE* kfbslide_read_associated_image(ImgHandle* s, const char* name);
// 逆向: 查询指定图片大小, 写入对应内存
void kfbslide_get_associated_image_dimensions(ImgHandle* s, const char* name, ll* width, ll*height, ll*nBytes);
// 逆向: 获得所有可用的图片名并读取图片
const char** kfbslide_get_associated_image_names(ImgHandle* s);

// 逆向: 读取指定区域?
bool kfbslide_read_region(ImgHandle* s, int level, int x, int y, int* nBytes, BYTE** buf);
bool kfbslide_get_image_roi_stream(ImgHandle* s, int level, int x, int y, int width, int height, int* nBytes, BYTE** buf);

// 内存管理: Python无法释放C库中malloc的资源, 因此必须允许库自身释放
// 我们在关闭handle时释放所有malloc的资源
bool kfbslide_buffer_free(ImgHandle* s, BYTE* buf);
#endif