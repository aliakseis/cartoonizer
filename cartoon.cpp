/*****************************************************************************
*   cartoon.cpp
*   Create a cartoon-like or painting-like image filter.
******************************************************************************
*   by Shervin Emami, 8th Aug 2016 (shervin.emami@gmail.com)
*   http://www.shervinemami.info/
******************************************************************************
*   Ch1 of the book "Mastering OpenCV with Practical Computer Vision Projects", 2nd Edition.
*   Copyright Packt Publishing 2016.
*   http://www.packtpub.com/cool-projects-with-opencv/book
*****************************************************************************/

#include "cartoon.h"


namespace {

using namespace cv;

// thinning stuff

enum ThinningTypes {
    THINNING_ZHANGSUEN = 0,  // Thinning technique of Zhang-Suen
    THINNING_GUOHALL = 1     // Thinning technique of Guo-Hall
};

// Applies a thinning iteration to a binary image
void thinningIteration(Mat img, int iter, int thinningType) {
    Mat marker = Mat::zeros(img.size(), CV_8UC1);

    if (thinningType == THINNING_ZHANGSUEN) {
        for (int i = 1; i < img.rows - 1; i++) {
            for (int j = 1; j < img.cols - 1; j++) {

                //if (img.at<uchar>(i, j) == 0) continue;

                uchar p2 = img.at<uchar>(i - 1, j);
                uchar p3 = img.at<uchar>(i - 1, j + 1);
                uchar p4 = img.at<uchar>(i, j + 1);
                uchar p5 = img.at<uchar>(i + 1, j + 1);
                uchar p6 = img.at<uchar>(i + 1, j);
                uchar p7 = img.at<uchar>(i + 1, j - 1);
                uchar p8 = img.at<uchar>(i, j - 1);
                uchar p9 = img.at<uchar>(i - 1, j - 1);

                int A = static_cast<int>(p2 == 0 && p3 == 1) + static_cast<int>(p3 == 0 && p4 == 1) +
                        static_cast<int>(p4 == 0 && p5 == 1) + static_cast<int>(p5 == 0 && p6 == 1) +
                        static_cast<int>(p6 == 0 && p7 == 1) + static_cast<int>(p7 == 0 && p8 == 1) +
                        static_cast<int>(p8 == 0 && p9 == 1) + static_cast<int>(p9 == 0 && p2 == 1);
                int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
                int m1 = iter == 0 ? (p2 * p4 * p6) : (p2 * p4 * p8);
                int m2 = iter == 0 ? (p4 * p6 * p8) : (p2 * p6 * p8);

                if (A == 1 && (B >= 2 && B <= 6) && m1 == 0 && m2 == 0) {
                    marker.at<uchar>(i, j) = 1;
                }
            }
        }
    }
    if (thinningType == THINNING_GUOHALL) {
        for (int i = 1; i < img.rows - 1; i++) {
            for (int j = 1; j < img.cols - 1; j++) {
                uchar p2 = img.at<uchar>(i - 1, j);
                uchar p3 = img.at<uchar>(i - 1, j + 1);
                uchar p4 = img.at<uchar>(i, j + 1);
                uchar p5 = img.at<uchar>(i + 1, j + 1);
                uchar p6 = img.at<uchar>(i + 1, j);
                uchar p7 = img.at<uchar>(i + 1, j - 1);
                uchar p8 = img.at<uchar>(i, j - 1);
                uchar p9 = img.at<uchar>(i - 1, j - 1);

                int C = (static_cast<int>(p2 == 0u) & (p3 | p4)) + (static_cast<int>(p4 == 0u) & (p5 | p6)) +
                        (static_cast<int>(p6 == 0u) & (p7 | p8)) + (static_cast<int>(p8 == 0u) & (p9 | p2));
                int N1 = (p9 | p2) + (p3 | p4) + (p5 | p6) + (p7 | p8);
                int N2 = (p2 | p3) + (p4 | p5) + (p6 | p7) + (p8 | p9);
                int N = N1 < N2 ? N1 : N2;
                int m = iter == 0 ? ((p6 | p7 | static_cast<int>(p9 == 0u)) & p8) : ((p2 | p3 | static_cast<int>(p5 == 0u)) & p4);

                if ((C == 1) && ((N >= 2) && ((static_cast<int>((N <= 3)) & static_cast<int>(m == 0)) != 0))) {
                    marker.at<uchar>(i, j) = 1;
                }
            }
        }
    }

    img &= ~marker;
}

// Apply the thinning procedure to a given image
void thinning(InputArray input, OutputArray output, int thinningType = THINNING_ZHANGSUEN) {
    Mat processed = input.getMat().clone();
    // Enforce the range of the input image to be in between 0 - 255
    processed /= 255;

    Mat prev = Mat::zeros(processed.size(), CV_8UC1);
    Mat diff;

    do {
        thinningIteration(processed, 0, thinningType);
        thinningIteration(processed, 1, thinningType);
        absdiff(processed, prev, diff);
        processed.copyTo(prev);
    } while (countNonZero(diff) > 0);

    processed *= 255;

    output.assign(processed);
}

}


// Convert the given photo into a cartoon-like or painting-like image.
// Set sketchMode to true if you want a line drawing instead of a painting.
// Set alienMode to true if you want alien skin instead of human.
// Set evilMode to true if you want an "evil" character instead of a "good" character.
// Set debugType to 1 to show where skin color is taken from, and 2 to show the skin mask in a new window (for desktop).
void cartoonifyImage(Mat srcColor, Mat dst, bool sketchMode, bool alienMode, bool evilMode, int debugType)
{
    // Convert from BGR color to Grayscale
    //Mat srcGray;
    //cvtColor(srcColor, srcGray, COLOR_BGR2GRAY);

    /*
    // Remove the pixel noise with a good Median filter, before we start detecting edges.
    medianBlur(srcGray, srcGray, 3);
    */
    Size size = srcColor.size();
    Mat mask = Mat(size, CV_8U);
    Mat edges = Mat(size, CV_8U);
    Mat totalFltEdges = Mat::zeros(size, CV_32F);
    if (!evilMode) {

        std::vector<cv::Mat> bgr;
        split(srcColor, bgr);

        /*

        for (auto& src : bgr)
        {
            cv::Mat img;
            src.convertTo(img, CV_32F);
            img += 4.;
            cv::log(img, img);

            cv::medianBlur(img, img, 3);

            img.copyTo(src);
        }

        cv::Mat ohta[] = {
            (bgr[0] + bgr[1] + bgr[2]) / 3,
            (bgr[2] - bgr[0]) / 2,
            ((bgr[1] * 2) - bgr[2] - bgr[0]) / 4
        };

        for (auto& img : ohta)
        {
            Mat fltEdges = Mat(size, CV_32F);
            Laplacian(img, fltEdges, CV_32F, 3);

            totalFltEdges += fltEdges;
            //totalFltEdges =  cv::max(totalFltEdges, fltEdges);
        }

        mask = totalFltEdges < .3;

        */

        //*

        for (int idx = 0; idx < 3; ++idx)
        {
            auto& channel1 = bgr[(idx + 1) % 3];
            auto& channel2 = bgr[(idx + 2) % 3];

            cv::Mat max = cv::max(channel1, channel2);

            auto res = cv::Mat(size, CV_32F);

            auto& channel = bgr[idx];

            for (int i = 0; i < res.rows; i++)
                for (int j = 0; j < res.cols; j++)
                    res.at<float>(i, j) = std::atan2(channel.at<uchar>(i, j) + .5, max.at<uchar>(i, j) + .5);

            //cv::Mat res;
            //cv::phase(max, bgr[idx], res);

            cv::medianBlur(res, res, 5);
            //GaussianBlur(res, res, cv::Size(7, 7), 0, 0);

            //normalize(res, res, 0, 1, cv::NORM_MINMAX);

            Mat fltEdges = Mat(size, CV_32F);
            Laplacian(res, fltEdges, CV_32F, 5);

            totalFltEdges += fltEdges;
            //totalFltEdges = cv::max(totalFltEdges, fltEdges);
        }

        mask = totalFltEdges < .5;

        mask = 255 - mask;
        thinning(mask, mask);
        mask = 255 - mask;

        //*/

        {
            Mat srcGray;
            cvtColor(srcColor, srcGray, COLOR_BGR2GRAY);

            cv::Mat img;
            srcGray.convertTo(img, CV_32F);
            img += 4.;
            cv::log(img, img);

            medianBlur(img, img, 5);

            Mat fltEdges = Mat(size, CV_32F);
            Laplacian(img, fltEdges, CV_32F, 3);

            //totalFltEdges += fltEdges;
            //totalFltEdges = cv::max(totalFltEdges, fltEdges);

            cv::Mat altMask = fltEdges < .5;

            mask &= altMask;
        }


        cv::normalize(totalFltEdges, totalFltEdges, 0, 255, cv::NORM_MINMAX);
        totalFltEdges.convertTo(edges, CV_8U);


        /*

        // Generate a nice edge mask, similar to a pencil line drawing.
        Laplacian(srcGray, edges, CV_8U, 3);
        threshold(edges, mask, 30, 255, THRESH_BINARY_INV);
        // Tiny cameras usually have lots of noise, so remove small
        // dots of black noise from the black & white edge mask.

        */

        //mask = 255 - mask;
        //thinning(mask, mask);
        //mask = 255 - mask;

        removePepperNoise(mask);
    }
    else {
        // Evil mode, making everything look like a scary bad guy.
        // (Where "srcGray" is the original grayscale image plus a medianBlur of size 7x7).
        // Convert from BGR color to Grayscale
        Mat srcGray;
        cvtColor(srcColor, srcGray, COLOR_BGR2GRAY);

        // Remove the pixel noise with a good Median filter, before we start detecting edges.
        medianBlur(srcGray, srcGray, 3);
        Mat edges2;
        Scharr(srcGray, edges, CV_8U, 1, 0);
        Scharr(srcGray, edges2, CV_8U, 1, 0, -1);
        edges += edges2;
        threshold(edges, mask, 12, 255, THRESH_BINARY_INV);
        medianBlur(mask, mask, 3);
    }
    
    // For sketch mode, we just need the mask!
    if (sketchMode) {
        // The output image has 3 channels, not a single channel.
        cvtColor(mask, dst, COLOR_GRAY2BGR);
        return;
    }

    // Do the bilateral filtering at a shrunken scale, since it
    // runs so slowly but doesn't need full resolution for a good effect.
    Size smallSize;
    smallSize.width = size.width/2;
    smallSize.height = size.height/2;
    Mat smallImg = Mat(smallSize, CV_8UC3);
    resize(srcColor, smallImg, smallSize, 0,0, INTER_LINEAR);

    // Perform many iterations of weak bilateral filtering, to enhance the edges
    // while blurring the flat regions, like a cartoon.
    Mat tmp = Mat(smallSize, CV_8UC3);
    int repetitions = 7;        // Repetitions for strong cartoon effect.
    for (int i=0; i<repetitions; i++) {
        int size = 9;           // Filter size. Has a large effect on speed.
        double sigmaColor = 9;  // Filter color strength.
        double sigmaSpace = 7;  // Positional strength. Effects speed.
        bilateralFilter(smallImg, tmp, size, sigmaColor, sigmaSpace);
        bilateralFilter(tmp, smallImg, size, sigmaColor, sigmaSpace);
    }

    if (alienMode) {
        // Apply an "alien" filter, when given a shrunken image and the full-res edge mask.
        // Detects the color of the pixels in the middle of the image, then changes the color of that region to green.
        changeFacialSkinColor(smallImg, edges, debugType);
    }

    // Go back to the original scale.
    resize(smallImg, srcColor, size, 0,0, INTER_LINEAR);

    // Clear the output image to black, so that the cartoon line drawings will be black (ie: not drawn).
    memset((char*)dst.data, 0, dst.step * dst.rows);

    // Use the blurry cartoon image, except for the strong edges that we will leave black.
    srcColor.copyTo(dst, mask);
}



// Apply an "alien" filter, when given a shrunken BGR image and the full-res edge mask.
// Detects the color of the pixels in the middle of the image, then changes the color of that region to green.
void changeFacialSkinColor(Mat smallImgBGR, Mat bigEdges, int debugType)
{
        // Convert to Y'CrCb color-space, since it is better for skin detection and color adjustment.
        Mat yuv = Mat(smallImgBGR.size(), CV_8UC3);
        cvtColor(smallImgBGR, yuv, COLOR_BGR2YCrCb);

        // The floodFill mask has to be 2 pixels wider and 2 pixels taller than the small image.
        // The edge mask is the full src image size, so we will shrink it to the small size,
        // storing into the floodFill mask data.
        int sw = smallImgBGR.cols;
        int sh = smallImgBGR.rows;
        Mat maskPlusBorder = Mat::zeros(sh+2, sw+2, CV_8U);
        Mat mask = maskPlusBorder(Rect(1,1,sw,sh));  // mask is a ROI in maskPlusBorder.
        resize(bigEdges, mask, smallImgBGR.size());

        // Make the mask values just 0 or 255, to remove weak edges.
        threshold(mask, mask, 80, 255, THRESH_BINARY);
        // Connect the edges together, if there was a pixel gap between them.
        dilate(mask, mask, Mat());
        erode(mask, mask, Mat());
        //imshow("constraints for floodFill", mask);

        // YCrCb Skin detector and color changer using multiple flood fills into a mask.
        // Apply flood fill on many points around the face, to cover different shades & colors of the face.
        // Note that these values are dependent on the face outline, drawn in drawFaceStickFigure().
        int const NUM_SKIN_POINTS = 6;
        Point skinPts[NUM_SKIN_POINTS];
        skinPts[0] = Point(sw/2,          sh/2 - sh/6);
        skinPts[1] = Point(sw/2 - sw/11,  sh/2 - sh/6);
        skinPts[2] = Point(sw/2 + sw/11,  sh/2 - sh/6);
        skinPts[3] = Point(sw/2,          sh/2 + sh/16);
        skinPts[4] = Point(sw/2 - sw/9,   sh/2 + sh/16);
        skinPts[5] = Point(sw/2 + sw/9,   sh/2 + sh/16);
        // Skin might be fairly dark, or slightly less colorful.
        // Skin might be very bright, or slightly more colorful but not much more blue.
        const int LOWER_Y = 60;
        const int UPPER_Y = 80;
        const int LOWER_Cr = 25;
        const int UPPER_Cr = 15;
        const int LOWER_Cb = 20;
        const int UPPER_Cb = 15;
        Scalar lowerDiff = Scalar(LOWER_Y, LOWER_Cr, LOWER_Cb);
        Scalar upperDiff = Scalar(UPPER_Y, UPPER_Cr, UPPER_Cb);
        // Instead of drawing into the "yuv" image, just draw 1's into the "maskPlusBorder" image, so we can apply it later.
        // The "maskPlusBorder" is initialized with the edges, because floodFill() will not go across non-zero mask pixels.
        Mat edgeMask = mask.clone();    // Keep an duplicate copy of the edge mask.
        for (auto i=0; i<NUM_SKIN_POINTS; i++) {
            // Use the floodFill() mode that stores to an external mask, instead of the input image.
            const int flags = 4 | FLOODFILL_FIXED_RANGE | FLOODFILL_MASK_ONLY;
            floodFill(yuv, maskPlusBorder, skinPts[i], Scalar(), NULL, lowerDiff, upperDiff, flags);
            if (debugType >= 1)
                circle(smallImgBGR, skinPts[i], 5, CV_RGB(0, 0, 255), 1, LINE_AA);
        }
        if (debugType >= 2)
            imshow("flood mask", mask*120); // Draw the edges as white and the skin region as grey.

        // After the flood fill, "mask" contains both edges and skin pixels, whereas
        // "edgeMask" just contains edges. So to get just the skin pixels, we can remove the edges from it.
        mask -= edgeMask;
        // "mask" now just contains 1's in the skin pixels and 0's for non-skin pixels.

        // Change the color of the skin pixels in the given BGR image.
        auto Red = 0;
        auto Green = 70;
        auto Blue = 0;
        add(smallImgBGR, Scalar(Blue, Green, Red), smallImgBGR, mask);
}


// Remove black dots (upto 3x3 in size) of noise from a pure black & white image.
// ie: The input image should be mostly white (255) and just contains some black (0) noise
// in addition to the black (0) edges.
// Note this can be done using erode & dilate, but the effect isn't as nice.
void removePepperNoise(Mat &mask)
{
    // For simplicity, ignore the top & bottom row border.
    for (auto y=2; y<mask.rows-2; y++) {
        // Get access to each of the 5 rows near this pixel.
        uchar *pThis = mask.ptr(y);
        uchar *pUp1 = mask.ptr(y-1);
        uchar *pUp2 = mask.ptr(y-2);
        uchar *pDown1 = mask.ptr(y+1);
        uchar *pDown2 = mask.ptr(y+2);

        // For simplicity, ignore the left & right row border.
        pThis += 2;
        pUp1 += 2;
        pUp2 += 2;
        pDown1 += 2;
        pDown2 += 2;
        for (auto x=2; x<mask.cols-2; x++) {
            uchar v = *pThis;   // Get the current pixel value (either 0 or 255).
            // If the current pixel is black, but all the pixels on the 2-pixel-radius-border are white
            // (ie: it is a small island of black pixels, surrounded by white), then delete that island.
            if (v == 0) {
                bool allAbove = *(pUp2 - 2) && *(pUp2 - 1) && *(pUp2) && *(pUp2 + 1) && *(pUp2 + 2);
                bool allLeft = *(pUp1 - 2) && *(pThis - 2) && *(pDown1 - 2);
                bool allBelow = *(pDown2 - 2) && *(pDown2 - 1) && *(pDown2) && *(pDown2 + 1) && *(pDown2 + 2);
                bool allRight = *(pUp1 + 2) && *(pThis + 2) && *(pDown1 + 2);
                bool surroundings = allAbove && allLeft && allBelow && allRight;
                if (surroundings == true) {
                    // Fill the whole 5x5 block as white. Since we know the 5x5 borders
                    // are already white, just need to fill the 3x3 inner region.
                    *(pUp1 - 1) = 255;
                    *(pUp1 + 0) = 255;
                    *(pUp1 + 1) = 255;
                    *(pThis - 1) = 255;
                    *(pThis + 0) = 255;
                    *(pThis + 1) = 255;
                    *(pDown1 - 1) = 255;
                    *(pDown1 + 0) = 255;
                    *(pDown1 + 1) = 255;
                }
                // Since we just covered the whole 5x5 block with white, we know the next 2 pixels
                // won't be black, so skip the next 2 pixels on the right.

                x += 2;

                pThis += 2;
                pUp1 += 2;
                pUp2 += 2;
                pDown1 += 2;
                pDown2 += 2;
            }
            // Move to the next pixel.
            pThis++;
            pUp1++;
            pUp2++;
            pDown1++;
            pDown2++;
        }
    }
}


// Draw an anti-aliased face outline, so the user knows where to put their face.
// Note that the skin detector for "alien" mode uses points around the face based on the face
// dimensions shown by this function.
void drawFaceStickFigure(Mat dst)
{
    Size size = dst.size();
    int sw = size.width;
    int sh = size.height;

    // Draw the face onto a color image with black background.
    Mat faceOutline = Mat::zeros(size, CV_8UC3);
    Scalar color = CV_RGB(255,255,0);   // Yellow
    auto thickness = 4;
    // Use 70% of the screen height as the face height.
    int faceH = sh/2 * 70/100;  // "faceH" is actually half the face height (ie: radius of the ellipse).
    // Scale the width to be the same nice shape for any screen width (based on screen height).
    int faceW = faceH * 72/100; // Use a face with an aspect ratio of 0.72
    // Draw the face outline.
    ellipse(faceOutline, Point(sw/2, sh/2), Size(faceW, faceH), 0, 0, 360, color, thickness, LINE_AA);
    // Draw the eye outlines, as 2 half ellipses.
    int eyeW = faceW * 23/100;
    int eyeH = faceH * 11/100;
    int eyeX = faceW * 48/100;
    int eyeY = faceH * 13/100;
    // Set the angle and shift for the eye half ellipses.
    auto eyeA = 15; // angle in degrees.
    auto eyeYshift = 11;
    // Draw the top of the right eye.
    ellipse(faceOutline, Point(sw/2 - eyeX, sh/2 - eyeY), Size(eyeW, eyeH), 0, 180+eyeA, 360-eyeA, color, thickness, LINE_AA);
    // Draw the bottom of the right eye.
    ellipse(faceOutline, Point(sw/2 - eyeX, sh/2 - eyeY - eyeYshift), Size(eyeW, eyeH), 0, 0+eyeA, 180-eyeA, color, thickness, LINE_AA);
    // Draw the top of the left eye.
    ellipse(faceOutline, Point(sw/2 + eyeX, sh/2 - eyeY), Size(eyeW, eyeH), 0, 180+eyeA, 360-eyeA, color, thickness, LINE_AA);
    // Draw the bottom of the left eye.
    ellipse(faceOutline, Point(sw/2 + eyeX, sh/2 - eyeY - eyeYshift), Size(eyeW, eyeH), 0, 0+eyeA, 180-eyeA, color, thickness, LINE_AA);

    // Draw the bottom lip of the mouth.
    int mouthY = faceH * 53/100;
    int mouthW = faceW * 45/100;
    int mouthH = faceH * 6/100;
    ellipse(faceOutline, Point(sw/2, sh/2 + mouthY), Size(mouthW, mouthH), 0, 0, 180, color, thickness, LINE_AA);

    // Draw anti-aliased text.
    auto fontFace = FONT_HERSHEY_COMPLEX;
    auto fontScale = 1.0f;
    auto fontThickness = 2;
    putText(faceOutline, "Put your face here", Point(sw * 23/100, sh * 10/100), fontFace, fontScale, color, fontThickness, LINE_AA);
    //imshow("faceOutline", faceOutline);

    // Overlay the outline with alpha blending.
    addWeighted(dst, 1.0, faceOutline, 0.7, 0, dst, CV_8UC3);
}
