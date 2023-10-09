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
#ifdef __cplusplus
extern "C" {
#endif
/**
 * Open a whole slide image.
 *
 * This function can be expensive; avoid calling it unnecessarily.  For
 * example, a tile server should not call openslide_open() on every tile
 * request.  Instead, it should maintain a cache of OpenSlide objects and
 * reuse them when possible.
 *
 * @param filename The filename to open.  On Windows, this must be in UTF-8.
 * @return
 *         On success, a new OpenSlide object.
 *         If the file is not recognized by OpenSlide, NULL.
 *         If the file is recognized but an error occurred, an OpenSlide
 *         object in error state.
 */

ImgHandle* kfbslide_open(const char * dllPath, const char* filename);

/**
 * Close an OpenSlide object.
 * No other threads may be using the object.
 * After this function returns, the object cannot be used anymore.
 *
 * @param osr The OpenSlide object.
 */
void kfbslide_close(ImgHandle* s);

/**
 * Quickly determine whether a whole slide image is recognized.
 *
 * If OpenSlide recognizes the file referenced by @p filename, return a
 * string identifying the slide format vendor.  This is equivalent to the
 * value of the #OPENSLIDE_PROPERTY_NAME_VENDOR property.  Calling
 * openslide_open() on this file will return a valid OpenSlide object or
 * an OpenSlide object in error state.
 *
 * Otherwise, return NULL.  Calling openslide_open() on this file will also
 * return NULL.
 *
 * @param filename The filename to check.  On Windows, this must be in UTF-8.
 * @return An identification of the format vendor for this file, or NULL.
 * @since 3.4.0
 */
const char * kfbslide_detect_vendor(const char *);

/**
 * Get the NULL-terminated array of property names.
 *
 * This function returns an array of strings naming properties available
 * in the whole slide image.
 *
 * @param osr The OpenSlide object.
 * @return A NULL-terminated string array of property names, or
 *         an empty array if an error occurred.
 */
// 逆向: 固定值 "openslide.mpp-x", "openslide.mpp-t", "openslide.vendor"
const char** kfbslide_property_names(ImgHandle* preader);
/**
 * Get the value of a single property.
 *
 * This function returns the value of the property given by @p name.
 *
 * @param osr The OpenSlide object.
 * @param name The name of the desired property. Must be
               a valid name as given by openslide_get_property_names().
 * @return The value of the named property, or NULL if the property
 *         doesn't exist or an error occurred.
 */
// 逆向: mpp为headerInfo.CapRes, vendor为Kfbio
const char * kfbslide_property_value(ImgHandle* preader, const char * attribute_name);

/**
 * Get the downsampling factor of a given level.
 *
 * @param osr The OpenSlide object.
 * @param level The desired level.
 * @return The downsampling factor for this level, or -1.0 if an error occurred
 *         or the level was out of range.
 * @since 3.3.0
 */
double kfbslide_get_level_downsample(ImgHandle* s, int level);


/**
 * Get the best level to use for displaying the given downsample.
 *
 * @param osr The OpenSlide object.
 * @param downsample The downsample factor.
 * @return The level identifier, or -1 if an error occurred.
 * @since 3.3.0
 */
int kfbslide_get_best_level_for_downsample(ImgHandle* s, double downsample);


/**
 * Get the number of levels in the whole slide image.
 *
 * @param osr The OpenSlide object.
 * @return The number of levels, log2(header.BlockSize).
 */
int kfbslide_get_level_count(ImgHandle* s);

/**
 * Get the dimensions of a level.
 *
 * @param osr The OpenSlide object.
 * @param level The desired level.
 * @param[out] w The width of the image, or -1 if an error occurred
 *               or the level was out of range.
 * @param[out] h The height of the image, or -1 if an error occurred
 *               or the level was out of range.
 * @since 3.3.0
 */
ll kfbslide_get_level_dimensions(ImgHandle* s, int level, ll* width, ll* height);

/**
 * Get the dimensions of level 0 (the largest level). Exactly
 * equivalent to calling openslide_get_level_dimensions(osr, 0, w, h).
 *
 * @param osr The OpenSlide object.
 * @param[out] w The width of the image, or -1 if an error occurred.
 * @param[out] h The height of the image, or -1 if an error occurred.
 * @since 3.3.0
 */
ll kfbslide_get_level0_dimensions(ImgHandle* s, ll* width, ll* height);



/**
 * Copy pre-multiplied ARGB data from an associated image.
 *
 * This function reads and decompresses an associated image associated
 * with a whole slide image. @p dest must be a valid pointer to enough
 * memory to hold the image, at least (width * height * 4) bytes in
 * length.  Get the width and height with
 * openslide_get_associated_image_dimensions().
 *
 * If an error occurs or has occurred, then the memory pointed to by @p dest
 * will be cleared. In versions prior to 4.0.0, this function did nothing
 * if an error occurred.
 *
 * For more information about processing pre-multiplied pixel data, see
 * the [OpenSlide website](https://openslide.org/docs/premultiplied-argb/).
 *
 * @param osr The OpenSlide object.
 * @param dest The destination buffer for the ARGB data.
 * @param name The name of the desired associated image. Must be
 *             a valid name as given by openslide_get_associated_image_names().
 */
BYTE* kfbslide_read_associated_image(ImgHandle* s, const char* name);
/**
 * Get the dimensions of an associated image.
 *
 * This function returns the width and height of an associated image
 * associated with a whole slide image. Once the dimensions are known,
 * use openslide_read_associated_image() to read the image.
 *
 * @param osr The OpenSlide object.
 * @param name The name of the desired associated image. Must be
 *            a valid name as given by openslide_get_associated_image_names().
 * @param[out] w The width of the associated image, or -1 if an error occurred.
 * @param[out] h The height of the associated image, or -1 if an error occurred.
 */
void kfbslide_get_associated_image_dimensions(ImgHandle* s, const char* name, ll* width, ll*height, ll*nBytes);
/**
 * Get the NULL-terminated array of associated image names.
 *
 * This function returns an array of strings naming associated images
 * available in the whole slide image.
 *
 * @param osr The OpenSlide object.
 * @return A NULL-terminated string array of associated image names, or
           an empty array if an error occurred.
 */
const char** kfbslide_get_associated_image_names(ImgHandle* s);


bool kfbslide_read_region(ImgHandle* s, int level, int x, int y, int* nBytes, BYTE** buf);

/**
 * Copy pre-multiplied ARGB data from a whole slide image.
 *
 * This function reads and decompresses a region of a whole slide
 * image into the specified memory location. @p dest must be a valid
 * pointer to enough memory to hold the region, at least (@p w * @p h * 4)
 * bytes in length. If an error occurs or has occurred, then the memory
 * pointed to by @p dest will be cleared.
 *
 * For more information about processing pre-multiplied pixel data, see
 * the [OpenSlide website](https://openslide.org/docs/premultiplied-argb/).
 *
 * @param osr The OpenSlide object.
 * @param dest The destination buffer for the ARGB data.
 * @param x The top left x-coordinate, in the level 0 reference frame.
 * @param y The top left y-coordinate, in the level 0 reference frame.
 * @param level The desired level.
 * @param w The width of the region. Must be non-negative.
 * @param h The height of the region. Must be non-negative.
 */
bool kfbslide_get_image_roi_stream(ImgHandle* s, int level, int x, int y, int width, int height, int* nBytes, BYTE** buf);

// 内存管理: Python无法释放C库中malloc的资源, 因此必须允许库自身释放
// 我们在关闭handle时释放所有malloc的资源
bool kfbslide_buffer_free(ImgHandle* s, BYTE* buf);
#ifdef __cplusplus
}
#endif
#endif