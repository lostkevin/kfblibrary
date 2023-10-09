#include "kfbreader.h"

const char* names[] = {"openslide.mpp-x", "openslide.mpp-t", "openslide.vendor", "scanScale", nullptr};
const char* assoNames[] = {"label", "macro", "thumbnail", nullptr};

ImgHandle* kfbslide_open(const char * dllPath, const char* filename) {
	void *handle = NULL;
	handle = dlopen(dllPath, RTLD_LAZY);
    if(!handle){
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    ImgHandle* s = new ImgHandle;
    s->handle = handle;
    DLLInitImageFileFunc InitImageFile = (DLLInitImageFileFunc)dlsym(s->handle,"InitImageFileFunc");
    DLLGetHeaderInfoFunc GetHeaderInfo = (DLLGetHeaderInfoFunc) dlsym(s->handle,"GetHeaderInfoFunc");
	if(!InitImageFile || !GetHeaderInfo)
    {   
    	printf("%s\n", "Error: dlsym failed.");
        exit(EXIT_FAILURE);
    }
    if(!InitImageFile(s->imgStruct, filename)) {
        delete s;
        printf("%s%s\n", "Error: Initialization failure. File: ", filename);
        return nullptr;
    }
    HeaderInfoStruct headerInfo;
    int getHeaderInfoRtn = GetHeaderInfo(s->imgStruct, 
                                            &(headerInfo.Height), 
                                            &(headerInfo.Width), 
                                            &(headerInfo.ScanScale), 
                                            &(headerInfo.SpendTime),
                                            &(headerInfo.ScanTime),
                                            &(headerInfo.CapRes),
                                            &(headerInfo.BlockSize));
    if(!getHeaderInfoRtn) {
        delete s;
        printf("%d %d %d %f %lf %f %d\n", headerInfo.Height, 
                                            headerInfo.Width, 
                                            headerInfo.ScanScale, 
                                            headerInfo.SpendTime, 
                                            headerInfo.ScanTime,
                                            headerInfo.CapRes,
                                            headerInfo.BlockSize);
        return nullptr;
    }
    s->properties["openslide.mpp-x"] = to_string(headerInfo.CapRes);
    s->properties["openslide.mpp-t"] = to_string(headerInfo.CapRes);
    s->properties["openslide.vendor"] = string("Kfbio");
    s->properties["scanScale"] = to_string(headerInfo.ScanScale);
    s->scanScale = headerInfo.ScanScale;

    
    s->height = headerInfo.Height;
    s->width = headerInfo.Width;
    s->maxLevel = min(6, (int)(log(max(s->height, s->width)) / log(2)));
    return s;
}

void kfbslide_close(ImgHandle* s) {
    DLLUnInitImageFileFunc UnInitImageFile = (DLLUnInitImageFileFunc)dlsym(s->handle, "UnInitImageFileFunc");
    if(!UnInitImageFile) {
    	printf("%s\n", "Error: dlsym failed.");
        exit(EXIT_FAILURE);        
    }
    UnInitImageFile(s->imgStruct);
    delete s;
}

const char * kfbslide_detect_vendor(const char *) {
    return "kfbio";
}

const char * kfbslide_property_value(ImgHandle* preader, const char * attribute_name) {
    map<string, string> & m = preader->properties;
    string s = attribute_name;
    auto iter = m.find(s);
    if(iter != m.end()) {
        return iter->second.c_str();
    }
    return nullptr;
}

const char** kfbslide_property_names(ImgHandle* preader) {
    return names;
}

/*
    Level Related...
*/
double kfbslide_get_level_downsample(ImgHandle* s, int level) {
    if(s->maxLevel > level && level >= 0) return double(1LL << level);
    return 0.0;
}

int kfbslide_get_best_level_for_downsample(ImgHandle* s, double downsample) {
    if(downsample < 1) return 0;
    double downsample_factor = downsample;
    for(int i = 0; i < s->maxLevel; i++) {
        if((1LL << (i + 1)) > downsample_factor ) return i;
    }
    return s->maxLevel - 1;
}

int kfbslide_get_level_count(ImgHandle* s) {
    return int(s->maxLevel);
}

ll kfbslide_get_level_dimensions(ImgHandle* s, int level, ll* width, ll* height) {
    if(level >= s->maxLevel || level < 0) return 0;
    if(!width || !height) {
        printf("You must pass width and height ptr ByRef!");
        return 0;
    }
    *width = s->width >> level;
    *height = s->height >> level;
    return *height;
}

/*
    Associated Image
*/
//! 希望对读出数据的任何改变都不会影响下一次读取, 因此必须拷贝
BYTE* kfbslide_read_associated_image(ImgHandle* s, const char* name) {
    string n(name);
    auto iter = s->assoImages.find(n);
    if(iter != s->assoImages.end()) {
        BYTE* buf = new BYTE[iter->second.nBytes];
        memcpy(buf, iter->second.buf.get(), iter->second.nBytes);
        s->alloc_mem.push_back(buf);
        return buf;
    }
    return nullptr;
}

void kfbslide_get_associated_image_dimensions(ImgHandle* s, const char* name, ll* width, ll*height, ll*nBytes) {
    if(!width || !height || !nBytes) {
        printf("You must pass width, height and nBytes ptr ByRef!");
        return;
    }
    
    string n(name);
    auto iter = s->assoImages.find(n);
    if(iter != s->assoImages.end()) {
        *width = iter->second.width;
        *height = iter->second.height;
        *nBytes = iter->second.nBytes;
        return;
    }
    *width = 0;
    *height = 0;
    *nBytes = 0;
}

const char** kfbslide_get_associated_image_names(ImgHandle* s) {
    if(s->assoNames) return s->assoNames;
    s->assoNames = new const char*[4]{nullptr};

    // Try to call getThumb, getPreview, getLabel
    DLLGetImageFunc GetThumbnailImageFunc, GetPreviewImageFunc, GetLabelImageFunc;
    GetThumbnailImageFunc = (DLLGetImageFunc)dlsym(s->handle,"GetThumnailImageFunc");
    GetPreviewImageFunc = (DLLGetImageFunc)dlsym(s->handle,"GetPriviewInfoFunc");
    GetLabelImageFunc = (DLLGetImageFunc)dlsym(s->handle,"GetLableInfoFunc");

    if(!GetThumbnailImageFunc || !GetPreviewImageFunc || !GetLabelImageFunc) {
        printf("%s\n", "Error: dlsym failed.");
        exit(EXIT_FAILURE);
    }

    BYTE* buf = nullptr;
    // bytesize width height
    int ret[3] = {0, 0, 0};
    int cnt = 0;
    if(GetLabelImageFunc(s->imgStruct, &buf, ret, ret + 1, ret + 2)) {
        s->assoImages["label"] = AssoImage{ret[0], ret[1], ret[2], shared_ptr<BYTE>(buf, default_delete<BYTE []>())};
        s->assoNames[cnt++] = "label";
    }

    if(GetThumbnailImageFunc(s->imgStruct, &buf, ret, ret + 1, ret + 2)) {
        s->assoImages["thumbnail"] = AssoImage{ret[0], ret[1], ret[2],shared_ptr<BYTE>(buf, default_delete<BYTE []>())};
        s->assoNames[cnt++] = "thumbnail";
    }

    if(GetPreviewImageFunc(s->imgStruct, &buf, ret, ret + 1, ret + 2)) {
        s->assoImages["macro"] = AssoImage{ret[0], ret[1], ret[2],shared_ptr<BYTE>(buf, default_delete<BYTE []>())};
        s->assoNames[cnt++] = "macro";
    }
    return s->assoNames;
}


/*
    Load Data
*/
bool kfbslide_read_region(ImgHandle* s, int level, int x, int y, int* nBytes, BYTE** buf) {
    if(level < 0 || level >= s->maxLevel) return false;
    if(!buf  || !nBytes) {
        printf("You must pass nBytes and buf ptr ByRef!");
        return false;
    }    
	DLLGetImageStreamFunc GetImageStreamFunc = (DLLGetImageStreamFunc)dlsym(s->handle,"GetImageStreamFunc");
	if(GetImageStreamFunc == NULL)
	{
		printf("%s\n", "Error: dlsym failed.");
        exit(EXIT_FAILURE);
	}    
    float fScale = s->scanScale / kfbslide_get_level_downsample(s, level);
    void* ptr = GetImageStreamFunc(s->imgStruct, fScale, x, y, nBytes, buf);
    s->alloc_mem.push_back(*buf);
    return *nBytes > 0;
}

bool kfbslide_get_image_roi_stream(ImgHandle* s, int level, int x, int y, int width, int height, int* nBytes, BYTE** buf) {
    if(level < 0 || level >= s->maxLevel) return false;
    if(!buf || !nBytes) {
        printf("You must pass nBytes and buf ptr ByRef!");
        return false;
    }   
	DLLGetImageDataRoiFunc GetImageDataRoi = (DLLGetImageDataRoiFunc)dlsym(s->handle,"GetImageDataRoiFunc");
	if(GetImageDataRoi == NULL)
	{
		printf("%s\n", "Error: dlsym failed.");
        exit(EXIT_FAILURE);
	}

    double downsample_factor = kfbslide_get_level_downsample(s, level);
    float fScale = s->scanScale / downsample_factor;
    x = x / downsample_factor;
    y = y / downsample_factor;
    
    bool ret = GetImageDataRoi(s->imgStruct, fScale, x, y, width, height, buf, nBytes, true);
    s->alloc_mem.push_back(*buf);
    return ret;
}

/*
    free resource
*/
bool kfbslide_buffer_free(ImgHandle* s, BYTE* buf) {
    if(!buf) return false;
    for(auto iter=s->alloc_mem.begin(); iter != s->alloc_mem.end(); iter++) {
        if(*iter == buf) {
            delete [] buf;
            s->alloc_mem.erase(iter);
            return true;
        }
    }
    return false;
}