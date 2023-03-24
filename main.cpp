#include <iostream>
#include "kfbreader.h"

using namespace std;

int main() {
    ImgHandle* s = kfbslide_open("lib/libImageOperationLib.so", "/nfs3-p1/hkw/PrognosisData/feulgenstain/预后差死亡复发组/A死亡复发组冰对 Feulgen/200615671.kfb");
    s->debug = true;
    cout << "Section 1: Associated Images" << endl;
    const char** sptr = kfbslide_get_associated_image_names(s);
    auto ptr = sptr;
    while(*ptr) {
        string str = *ptr;
        cout << *ptr++ << endl;
        ll w, d, nBytes;
        kfbslide_get_associated_image_dimensions(s, str.c_str(), &w, &d, &nBytes);
        BYTE* buf = kfbslide_read_associated_image(s, str.c_str());

        str += ".jpg";
        FILE *fp = fopen(str.c_str(), "wb+");
	    fwrite(buf, sizeof(BYTE), nBytes, fp);
	    fclose(fp);
    }
    cout << "Section 2: vendor and properties" << endl;
    cout <<  "Vendor:" << kfbslide_detect_vendor("") << endl;
    sptr = kfbslide_property_names(s);
    ptr = sptr;
    while(*ptr) {
        const char * value = kfbslide_property_value(s, *ptr);
        cout << *ptr++ << ":" << value << endl;
    }

    cout << "Section 3: Level related" << endl;
    int level_count = kfbslide_get_level_count(s);
    const char* scanScale = kfbslide_property_value(s, "scanScale");
    int baseScale = stoi(scanScale);
    for(int level = 0; level < level_count; level++) {
        double downsample_rate = kfbslide_get_level_downsample(s, level);
        
        int best_level = kfbslide_get_best_level_for_downsample(s, baseScale / downsample_rate);
        ll w, h;
        w = h = 0;
        kfbslide_get_level_dimensions(s, level, &w, &h);

        cout << "Level: " << level << ";downsample_rate: " << downsample_rate << "; best level: " << best_level 
             << ";width: "<< w << ";height: " << h << endl;
    }
    cout << "#Test best level for downsample" << endl;
    vector<double> vec = {-1.0, 0.0, 0.5, 1.0, 1.5, 4.7, 16.3, 32, 79, 10000};
    for(double item: vec) {
        cout << "Best Level for " << item << " is:" << kfbslide_get_best_level_for_downsample(s, item) << endl;
    }

    cout << "Section 4: ReadRoi" << endl;
    BYTE* buf = nullptr;
    int nBytes = 0;

    if(!kfbslide_get_image_roi_stream(s, 3, 0, 0, 2048, 2048, &nBytes, &buf)) {
        cout << "Read Roi Error!" << endl;
    } else {
        FILE *fp = fopen("roi1.jpg", "wb+");
	    fwrite(buf, sizeof(BYTE), nBytes, fp);
	    fclose(fp);  
    } //! Not manually free

    if(!kfbslide_get_image_roi_stream(s, 3, 0, 0, 1024, 1024, &nBytes, &buf)) {
        cout << "Read Roi Error!" << endl;
    } else {
        FILE *fp = fopen("roi2.jpg", "wb+");
	    fwrite(buf, sizeof(BYTE), nBytes, fp);
	    fclose(fp);  
    }
    kfbslide_buffer_free(s, buf);
    cout << "Manually free 1 buffer" << endl;
    kfbslide_close(s);
    return 0;
}