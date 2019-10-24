// based on https://ffmpeg.org/doxygen/trunk/remuxing_8c-example.html

#include "TransformVideo.h"
#include "cartoon.h"

#include <functional>


int main(int argc, char **argv)
{
    if (argc < 3) {
        printf("You need to pass at least two parameters.\n");
        return -1;
    }
    
    const char *in_filename = argv[1];
    const char *out_filename = argv[2];
    

    bool sketchMode = false;
    bool alienMode = false;
    bool evilMode = false;
    int debugType = 0;

    auto lam = [sketchMode, alienMode, evilMode, debugType](cv::Mat& img) {
        cv::Mat displayedFrame = cv::Mat(img.size(), CV_8UC3);
        cartoonifyImage(img, displayedFrame, sketchMode, alienMode, evilMode, debugType);
        img = displayedFrame;
    };

    return TransformVideo(in_filename, out_filename, lam);
}
