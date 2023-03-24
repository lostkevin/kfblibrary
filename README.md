# kfblibrary
A third-party library for kfb format whole slide image.

## Usage
Compile this project and then replace libkfbslide.so. For the API usage, you can refer to `main.cpp`.

## Why re-implement libkfbslide?

I find there is some memory leak issues in the raw libkfbslide.so, which crashs my model training process. 
Therefore, I re-implement the library, and add deallocation API and auto buffer deallocation in the kfbslide_close.

```
1. libkfbslide.so will load associate_images in kfbslide_open(), which slows down the performance.
2. All image data buffers allocated by libImageOperationLib.so will not be freed by the library.
```

In this project, I only re-implement all apis in `libkfbslide.so`. 
However, some operations in `libImageOperationLib.so` are not been implemented, including:

```
(functions reading properties)
    GetGetImageBarCodeFunc
    GetImageChannelIdentifierFunc
    GetMachineSerialNumFunc
    GetScanLevelInfoFunc
    GetScanTimeDurationFunc
    GetVersionInfoFunc
    
(functions reading data from filepath)
    GetLableInfoPathFunc
    GetPriviewInfoPathFunc	
    GetThumnailImageOnlyPathFunc
    GetThumnailImagePathFunc

(maybe concurrent loading)
    GetLUTImageManyPassageway
    GetImageStreamManyPassagewayFunc
    GetImageDataRoiManyPassagewayFunc

GetLUTImage
GetImageRGBDataStreamFunc
UnCompressBlockDataInfoFunc
```

You can implement this API with the help of IDA Pro.

## Notes

Some codes are from [KFB_Convert_TIFF](https://github.com/babiking/KFB_Convert_TIFF). If there is any bug in the code, please contact me via issue!