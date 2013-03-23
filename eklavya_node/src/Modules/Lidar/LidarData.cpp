/*
 * File:   LidarData.cpp
 * Author: bhuvnesh, priyanka
 *
 * Created on 18 September, 2012, 6:56 PM
 */

#include "LidarData.h"
#include <mrpt/hwdrivers/CHokuyoURG.h>
#include <mrpt/hwdrivers/CSerialPort.h>
#include <mrpt/gui.h>
#include <mrpt/maps.h>
#include <opencv2/core/types_c.h>
#include "../../eklavya2.h"
#include <cvblob.h>

/*  Filter:
 *  0: No filter
 *  1: Blob filter
 */

#define FILTER 1
#define DEBUG 0
#define DILATE 1

#define CENTERX 100
#define CENTERY 500
#define HOKUYO_SCALE 100
#define RADIUS 40
#define EXPAND_ITER 25

#define intensity(img,i,j,n) *(uchar*)(img->imageData + img->widthStep*i + j*img->nChannels + n) 
#define IMGDATA(image,i,j,k) (((uchar *)image->imageData)[(i)*(image->widthStep) + (j)*(image->nChannels) + (k)])
#define IMGDATAG(image,i,j) (((uchar *)image->imageData)[(i)*(image->widthStep) + (j)])

using namespace mrpt;
using namespace mrpt::hwdrivers;
using namespace mrpt::slam;
using namespace mrpt::gui;
using namespace mrpt::utils;
using namespace std;
using namespace cvb;

LidarData::LidarData(string serial_name) {

}

int checksum(char **localmap, int x, int y) {
    int threshold = 3;
    int win_size = 4;
    int sum = 0;
    for (int i = x - win_size; i < x + win_size; i++) {
        for (int j = y - win_size; j < y + win_size; j++) {
            if (i < 950 - RADIUS && i > RADIUS + 50 && j > RADIUS && j < 875 - RADIUS) {
                if (j > CENTERX + 5) {
                    if (localmap[i][j] == 1)
                        sum++;
                }
            }
        }
    }
    if (sum >= threshold)
        return 1;
    else
        return 0;

}

void LidarData::createCircle(int x, int y, int R) {
    //TODO: optimize using circle drawing alogrithm    

    for (int i = -RADIUS; i < RADIUS; i++) {
        if ((x + i >= 0) && (x + i <= MAP_MAX)) {
            for (int j = -RADIUS; j < RADIUS; j++) {
                if ((y + j >= 0) && (y + j <= MAP_MAX)) {
                    if (i * i + j * j <= RADIUS * RADIUS) {
                        global_map[x + i][y + j] = 255;
                    }
                }
            }
        }
    }
}

void LidarData::update_map(const sensor_msgs::LaserScan& scan) {

    //TODO: Fusion needs to be implemented in the STRATEGY module

    //initialize variables

    IplImage *img, *nblobs, *nblobs1, *labelImg;
    img = cvCreateImage(cvSize(MAP_MAX, MAP_MAX), 8, 1);
    cvSet(img, cvScalar(0));

    //TODO: put kernel initialization in constructor
    IplConvKernel *ker1, *ker2;

    CvBlobs blobs;
    uchar * ptr;

    //initilize variables ended

    size_t size = scan.ranges.size();
    float angle = scan.angle_min;
    float maxRangeForContainer = scan.range_max - 0.1f;

    for (size_t i = 0; i < size; ++i) {
        float dist = scan.ranges[i];
        if ((dist > scan.range_min) && (dist < maxRangeForContainer)) {
            double x1 = cos(angle) * dist;
            double y1 = sin(angle) * dist;
            int x = (int) ((y1 * 100) + CENTERY);
            int y = (int) ((x1 * 100) + CENTERX);

            if (x >= 0 && y >= 0 && (int) x < MAP_MAX && (int) y < MAP_MAX) {
                int x2 = (MAP_MAX - x - 1);
                int y2 = y;

                ptr = (uchar *) (img->imageData + x2 * img->widthStep);
                ptr[y2] = 255;
            }
        }
        angle += scan.angle_increment;
    }

    //Filtering

    switch (FILTER) {
        case 0:
        {
            break;
        }
        case 1:
        {
            labelImg = cvCreateImage(cvSize(MAP_MAX, MAP_MAX), IPL_DEPTH_LABEL, 1);
            nblobs = cvCreateImage(cvSize(MAP_MAX, MAP_MAX), 8, 3);
            nblobs1 = cvCreateImage(cvSize(MAP_MAX, MAP_MAX), 8, 3);

            ker1 = cvCreateStructuringElementEx(9, 9, 4, 4, CV_SHAPE_ELLIPSE);
            ker2 = cvCreateStructuringElementEx(7, 7, 3, 3, CV_SHAPE_ELLIPSE);

            cvDilate(img, img, ker1, 1);
            //cvErode(filtered_img,filtered_img,ker2,1);
            unsigned int result = cvLabel(img, labelImg, blobs);
            cvRenderBlobs(labelImg, blobs, nblobs, nblobs, CV_BLOB_RENDER_COLOR);
            cvFilterByArea(blobs, 500, img->height * img->width);
            cvRenderBlobs(labelImg, blobs, nblobs1, nblobs1, CV_BLOB_RENDER_COLOR);
            //converts nblobs1 to filtered_img(grayscale)
            cvCvtColor(nblobs1, img, CV_RGB2GRAY);
            //thresholds the filtered_img based on threshold value
            cvThreshold(img, img, 125, 255, CV_THRESH_BINARY);

            cvReleaseImage(&labelImg);
            cvReleaseImage(&nblobs);
            cvReleaseImage(&nblobs1);
            cvReleaseBlobs(blobs);

            cvReleaseStructuringElement(&ker1);
            cvReleaseStructuringElement(&ker2);

//            if (DEBUG) {
//                cvNamedWindow("Blob Filter", 0);
//                cvShowImage("Blob Filter", img);
//                cvWaitKey(1);
//            }
            break;
        }
    }

    if (DILATE) {
        ker1 = cvCreateStructuringElementEx(3, 3, 1, 1, CV_SHAPE_ELLIPSE);
        cvDilate(img, img, ker1, EXPAND_ITER);
        cvReleaseStructuringElement(&ker1);
    }
    
    if (DEBUG) {
        cvNamedWindow("Dilate Filter", 0);
        cvShowImage("Dilate Filter", img);
        cvWaitKey(1);
    }

    pthread_mutex_lock(&map_mutex);
    for (int i = 0; i < MAP_MAX; i++) {
        for (int j = 0; j < MAP_MAX; j++) {
            global_map[i][j] = IMGDATA(img, i, j, 0);
        }
    }
    pthread_mutex_unlock(&map_mutex);
    cvReleaseImage(&img);
}

LidarData::~LidarData() {
}

