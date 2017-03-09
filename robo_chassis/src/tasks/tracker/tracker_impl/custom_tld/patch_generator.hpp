/*M///////////////////////////////////////////////////////////////////////////////////////
//
//  IMPORTANT: READ BEFORE DOWNLOADING, COPYING, INSTALLING OR USING.
//
//  By downloading, copying, installing or using the software you agree to this license.
//  If you do not agree to this license, do not download, install,
//  copy or use the software.
//
//
//                        Intel License Agreement
//                For Open Source Computer Vision Library
//
// Copyright (C) 2000, Intel Corporation, all rights reserved.
// Third party copyrights are property of their respective owners.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//   * Redistribution's of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//   * Redistribution's in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//   * The name of Intel Corporation may not be used to endorse or promote products
//     derived from this software without specific prior written permission.
//
// This software is provided by the copyright holders and contributors "as is" and
// any express or implied warranties, including, but not limited to, the implied
// warranties of merchantability and fitness for a particular purpose are disclaimed.
// In no event shall the Intel Corporation or contributors be liable for any direct,
// indirect, incidental, special, exemplary, or consequential damages
// (including, but not limited to, procurement of substitute goods or services;
// loss of use, data, or profits; or business interruption) however caused
// and on any theory of liability, whether in contract, strict liability,
// or tort (including negligence or otherwise) arising in any way out of
// the use of this software, even if advised of the possibility of such damage.
//
//M*/

#ifndef __OPENCV_LEGACY_HPP__
#define __OPENCV_LEGACY_HPP__

#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/ml/ml.hpp"

#ifdef __cplusplus
extern "C" {
#endif

namespace cv
{

class CV_EXPORTS PatchGenerator
{
public:
    PatchGenerator();
    PatchGenerator(double _backgroundMin, double _backgroundMax,
                   double _noiseRange, bool _randomBlur=true,
                   double _lambdaMin=0.6, double _lambdaMax=1.5,
                   double _thetaMin=-CV_PI, double _thetaMax=CV_PI,
                   double _phiMin=-CV_PI, double _phiMax=CV_PI );
    void operator()(const Mat& image, Point2f pt, Mat& patch, Size patchSize, RNG& rng) const;
    void operator()(const Mat& image, const Mat& transform, Mat& patch,
                    Size patchSize, RNG& rng) const;
    void warpWholeImage(const Mat& image, Mat& matT, Mat& buf,
                        CV_OUT Mat& warped, int border, RNG& rng) const;
    void generateRandomTransform(Point2f srcCenter, Point2f dstCenter,
                                 CV_OUT Mat& transform, RNG& rng,
                                 bool inverse=false) const;
    void setAffineParam(double lambda, double theta, double phi);

    double backgroundMin, backgroundMax;
    double noiseRange;
    bool randomBlur;
    double lambdaMin, lambdaMax;
    double thetaMin, thetaMax;
    double phiMin, phiMax;
};
}
#ifdef __cplusplus
}
#endif

#endif

/* End of file. */
