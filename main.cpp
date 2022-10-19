#include <opencv2/core/core.hpp>
#include <opencv2/core/core_c.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/highgui/highgui_c.h>
#include <opencv2/imgproc.hpp>
#include <opencv2/opencv.hpp>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "stereo.h"
#include "cssc132.h"
#include "cmd_parse.h"


#ifdef __cplusplus
}
#endif

using namespace cv;

ImageHandler image_handler[MAX_CAMERA_NUM] = {NULL};

int imageHandler(struct ImageUnit *image)
{
    int ret = 0;

    struct timeval tv[2];
    char image_name[64] = {0};
    long long int time_stamp = 0;
    FILE *fp;
    char time_stamp_buf[64] = {0};

    /* fp = fopen("/home/nvidia/work/calibration/cam0/time_stamp.txt", "a+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open time_stamp file\n",__func__);
		return -1;
    }

    time_stamp = (long long int)(image->time_stamp->time_stamp_local * 1000000000.0);

    snprintf(image_name,63,"/home/nvidia/work/calibration/cam0/data/%lld.jpg",time_stamp);

    snprintf(time_stamp_buf,63,"%lld\n",time_stamp);

    fwrite(time_stamp_buf, strlen(time_stamp_buf) , 1, fp);

    fclose(fp);

    memset(time_stamp_buf,0,64);
    fp = fopen("/home/nvidia/work/calibration/cam0/data.csv", "a+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open time_stamp file\n",__func__);
		return -1;
    }

    snprintf(time_stamp_buf,63,"%lld,%lld.jpg\n",time_stamp,time_stamp);

    fwrite(time_stamp_buf, strlen(time_stamp_buf) , 1, fp);

    fclose(fp);

    gettimeofday(&tv[0],NULL); */

    Mat img(image->image->height,image->image->width,CV_8UC1);
    // convert_UYVY_To_GRAY((unsigned char *)image->image->image,
    //                      img.data,
	// 					 image->image->width,
	// 					 image->image->height);
    img.data = (unsigned char *)image->image->image;
    // imwrite(image_name, img);
    // imshow("Capture", img);
	// waitKey(1);
/*
    gettimeofday(&tv[1],NULL);

    fprintf(stderr, "save jpg: %ldms\n",(tv[1].tv_sec * 1000 + tv[1].tv_usec / 1000) - (tv[0].tv_sec * 1000 + tv[0].tv_usec / 1000));
 */
    printf("\r\n");
    printf("======================= image timestamp[0] start =======================\n");
    printf("* image->time_stamp->time_stamp_local : %lf\n",image->time_stamp->time_stamp_local);
    printf("* image->time_stamp->time_stamp_gnss  : %lf\n",image->time_stamp->time_stamp_gnss);
    printf("* image->time_stamp->counter          : %d\n",image->time_stamp->counter);
    printf("* image->image->number                : %d\n",image->image->number);
    printf("* image->time_stamp->number           : %d\n",image->time_stamp->number);
    printf("======================== image timestamp[0] end ========================\n");

    return ret;
}

int imageHandler1(struct ImageUnit *image)
{
    int ret = 0;

    struct timeval tv[2];
    char image_name[64] = {0};
    long long int time_stamp = 0;
    FILE *fp;
    char time_stamp_buf[64] = {0};

    /* fp = fopen("/home/nvidia/work/calibration/cam0/time_stamp.txt", "a+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open time_stamp file\n",__func__);
		return -1;
    }

    time_stamp = (long long int)(image->time_stamp->time_stamp_local * 1000000000.0);

    snprintf(image_name,63,"/home/nvidia/work/calibration/cam0/data/%lld.jpg",time_stamp);

    snprintf(time_stamp_buf,63,"%lld\n",time_stamp);

    fwrite(time_stamp_buf, strlen(time_stamp_buf) , 1, fp);

    fclose(fp);

    memset(time_stamp_buf,0,64);
    fp = fopen("/home/nvidia/work/calibration/cam0/data.csv", "a+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open time_stamp file\n",__func__);
		return -1;
    }

    snprintf(time_stamp_buf,63,"%lld,%lld.jpg\n",time_stamp,time_stamp);

    fwrite(time_stamp_buf, strlen(time_stamp_buf) , 1, fp);

    fclose(fp);

    gettimeofday(&tv[0],NULL); */

    Mat img(image->image->height,image->image->width,CV_8UC1);
    // convert_UYVY_To_GRAY((unsigned char *)image->image->image,
    //                      img.data,
	// 					 image->image->width,
	// 					 image->image->height);
    img.data = (unsigned char *)image->image->image;
    // imwrite(image_name, img);
    imshow("Capture1", img);
	waitKey(1);
/*
    gettimeofday(&tv[1],NULL);

    fprintf(stderr, "save jpg: %ldms\n",(tv[1].tv_sec * 1000 + tv[1].tv_usec / 1000) - (tv[0].tv_sec * 1000 + tv[0].tv_usec / 1000));
 */

    printf("\r\n");
    printf("======================= image timestamp[1] start =======================\n");
    printf("* image->time_stamp->time_stamp_local : %lf\n",image->time_stamp->time_stamp_local);
    printf("* image->time_stamp->time_stamp_gnss  : %lf\n",image->time_stamp->time_stamp_gnss);
    printf("* image->time_stamp->counter          : %d\n",image->time_stamp->counter);
    printf("* image->image->number                : %d\n",image->image->number);
    printf("* image->time_stamp->number           : %d\n",image->time_stamp->number);
    printf("======================== image timestamp[1] end ========================\n");

    return ret;
}

int imuSyncHandler(struct SyncImuData *sync_imu_data)
{
    int ret = 0;
    FILE *fp;
    char imu_data[128] = {0};
    long long int time_stamp = 0;

    fp = fopen("/home/nvidia/work/calibration/imu0/imu0.csv", "a+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open imu_data file\n",__func__);
		return -1;
    }

    time_stamp = (long long int)(sync_imu_data->time_stamp_local * 1000000000.0);

    if(sync_imu_data->imu_module == IMU_BMI088)
    {
        snprintf(imu_data,127,"%lld,%f,%f,%f,%f,%f,%f\n",
                 time_stamp,
                 (double)(short)sync_imu_data->gx / (float)32768.0f * (float)sync_imu_data->gyro_range * 3.1415926f / 180.0f,
                 (double)(short)sync_imu_data->gy / (float)32768.0f * (float)sync_imu_data->gyro_range * 3.1415926f / 180.0f,
                 (double)(short)sync_imu_data->gz / (float)32768.0f * (float)sync_imu_data->gyro_range * 3.1415926f / 180.0f,
                 (double)(short)sync_imu_data->ax / (float)32768.0f * (float)sync_imu_data->acc_range * 1.5f * 9.81f,
                 (double)(short)sync_imu_data->ay / (float)32768.0f * (float)sync_imu_data->acc_range * 1.5f * 9.81f,
                 (double)(short)sync_imu_data->az / (float)32768.0f * (float)sync_imu_data->acc_range * 1.5f * 9.81f);
    }
    else if(sync_imu_data->imu_module == IMU_ADIS16505)
    {
        snprintf(imu_data,127,"%lld,%f,%f,%f,%f,%f,%f\n",
                 time_stamp,
                 (double)sync_imu_data->gx * 3.814697265625000e-07 * 3.1415926f / 180.0f,
                 (double)sync_imu_data->gy * 3.814697265625000e-07 * 3.1415926f / 180.0f,
                 (double)sync_imu_data->gz * 3.814697265625000e-07 * 3.1415926f / 180.0f,
                 (double)sync_imu_data->ax * 3.7384033203125e-08,
                 (double)sync_imu_data->ay * 3.7384033203125e-08,
                 (double)sync_imu_data->az * 3.7384033203125e-08);
    }
    else
    {
        snprintf(imu_data,127,"%lld,%f,%f,%f,%f,%f,%f\n",
                 time_stamp,
                 (double)(short)sync_imu_data->gx / (32768.0f / 250.0f) * 3.1415926f / 180.0f,
                 (double)(short)sync_imu_data->gy / (32768.0f / 250.0f) * 3.1415926f / 180.0f,
                 (double)(short)sync_imu_data->gz / (32768.0f / 250.0f) * 3.1415926f / 180.0f,
                 (double)(short)sync_imu_data->ax / (32768.0f / 8.0f) * 9.81f,
                 (double)(short)sync_imu_data->ay / (32768.0f / 8.0f) * 9.81f,
                 (double)(short)sync_imu_data->az / (32768.0f / 8.0f) * 9.81f);
    }

    fwrite(imu_data, strlen(imu_data) , 1, fp);

    fclose(fp);

    return ret;
}

int odb2ObjectsHandler(struct ODB2_Objects *odb2_objects)
{
    fprintf(stderr, "%s: odb2_objects->vehicle_speed = %d\n",__func__,odb2_objects->vehicle_speed);

    return 0;
}

int main(int argc, char **argv)
{
    int ret = 0;
    FILE *fp;
    char temp_data[150] = {0};

    delete_file("/home/nvidia/work/calibration");

    ret = mkdir("/home/nvidia/work/calibration/imu0",0755);
    if(ret != 0)
    {
        fprintf(stderr, "%s: mkdir /home/nvidia/work/calibration/imu0 failed\n",__func__);
    }

    ret = mkdir("/home/nvidia/work/calibration/cam0",0755);
    if(ret != 0)
    {
        fprintf(stderr, "%s: mkdir /home/nvidia/work/calibration/cam0 failed\n",__func__);
    }

    ret = mkdir("/home/nvidia/work/calibration/cam0/data",0755);
    if(ret != 0)
    {
        fprintf(stderr, "%s: mkdir /home/nvidia/work/calibration/cam0/data failed\n",__func__);
    }

    fp = fopen("/home/nvidia/work/calibration/imu0/imu0.csv", "w+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open imu_data file\n",__func__);
        return -1;
    }
    snprintf(temp_data,149,"#timestamp [ns],w_RS_S_x [rad s^-1],w_RS_S_y [rad s^-1],w_RS_S_z [rad s^-1],a_RS_S_x [m s^-2],a_RS_S_y [m s^-2],a_RS_S_z [m s^-2]\n");
    fwrite(temp_data, strlen(temp_data) , 1, fp);
    fclose(fp);

    memset(temp_data,0,150);
    fp = fopen("/home/nvidia/work/calibration/cam0/data.csv", "w+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open time_stamp file\n",__func__);
        return -1;
    }
    snprintf(temp_data,149,"#timestamp [ns],filename\n");
    fwrite(temp_data, strlen(temp_data) , 1, fp);
    fclose(fp);

    fp = fopen("/home/nvidia/work/calibration/cam0/time_stamp.txt", "w+");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open time_stamp file\n",__func__);
        return -1;
    }
    fclose(fp);

    monocular_sdk_init(argc, argv);

    image_handler[0] = imageHandler;
    image_handler[1] = imageHandler1;
    image_handler[2] = NULL;

    monocular_sdk_register_handler(image_handler,imuSyncHandler,NULL,NULL,NULL,NULL,odb2ObjectsHandler);

    // cvNamedWindow("Capture",CV_WINDOW_AUTOSIZE);

    while(1)
    {
        usleep(1000 * 1000);
    }

	return 0;
}


