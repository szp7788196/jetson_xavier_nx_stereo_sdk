#include "sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "monocular.h"
#include "cmd_parse.h"

static int syncImageAndCameraTimeStamp(unsigned char index)
{
    int ret = 0;
    int i = 0;
    struct SyncCamTimeStamp *cam_time_stamp = NULL;
    struct SyncCamTimeStamp time_stamp;
    static unsigned int image_counter[MAX_CAMERA_NUM] = {0};
    static char first_sync[MAX_CAMERA_NUM] = {0};

    ret = xQueueReceive((key_t)KEY_SYNC_CAM_TIME_STAMP_MSG + index,(void **)&cam_time_stamp,1);
    if(ret == -1)
    {
        return 0;
    }

    memcpy(&time_stamp,cam_time_stamp,sizeof(struct SyncCamTimeStamp));
    free(cam_time_stamp);
    cam_time_stamp = NULL;

    pthread_mutex_lock(&mutexImageHeap[index]);

    pthread_cond_wait(&condImageHeap[index], &mutexImageHeap[index]);

    memcpy(imageHeap[index].heap[imageHeap[index].put_ptr]->time_stamp,&time_stamp,sizeof(struct SyncCamTimeStamp));

    if(first_sync[index] == 1)
    {
        first_sync[index] = 0;

        do
        {
            ret = xQueueReceive((key_t)KEY_SYNC_CAM_TIME_STAMP_MSG + index,(void **)&cam_time_stamp,0);
            if(ret != -1)
            {
                image_counter[index] = cam_time_stamp->counter;

                free(cam_time_stamp);
                cam_time_stamp = NULL;

                fprintf(stderr, "%s: +\n",__func__);
            }
        }
        while(ret != -1);

        imageHeap[index].get_ptr = 0;

        for(i = 0; i < imageHeap[index].depth; i ++)
        {
            imageHeap[index].heap[i]->image->counter = 0;
        }

        ret = 0;

//        image_counter[index] = imageHeap[index].heap[imageHeap[index].put_ptr]->time_stamp->counter;

        pthread_mutex_unlock(&mutexImageHeap[index]);
    }
    else
    {
        image_counter[index] ++;
        imageHeap[index].heap[imageHeap[index].put_ptr]->image->counter = image_counter[index];

        if(imageHeap[index].heap[imageHeap[index].put_ptr]->image->counter != 
           imageHeap[index].heap[imageHeap[index].put_ptr]->time_stamp->counter)
        {
            if(abs(imageHeap[index].heap[imageHeap[index].put_ptr]->time_stamp->counter - 
                   imageHeap[index].heap[imageHeap[index].put_ptr]->image->counter) >= NOT_SYNC_THRESHOLD)
            {
                fprintf(stderr, "%s: sync image and camera trigger time stamp failed\n",__func__);
                first_sync[index] = 1;
                ret = -1;
            }
        }
        else
        {
//            fprintf(stderr, "%s: sync image and camera trigger time stamp success\n",__func__);
        }

        ret = xQueueSend((key_t)KEY_IMAGE_HANDLER_MSG + index,imageHeap[index].heap[imageHeap[index].put_ptr],imageHeap[index].depth);
        if(ret == -1)
        {
            fprintf(stderr, "%s: send imageHeap[%d].heap[imageHeap[%d].put_ptr] queue msg failed\n",__func__,index,index);
        }
    }

    imageHeap[index].put_ptr = (imageHeap[index].put_ptr + 1) % imageHeap[index].depth;

	imageHeap[index].cnt += 1;
	if(imageHeap[index].cnt >= imageHeap[index].depth)
	{
		imageHeap[index].cnt = imageHeap[index].depth;

		imageHeap[index].get_ptr = imageHeap[index].put_ptr;
	}

    pthread_mutex_unlock(&mutexImageHeap[index]);

    return ret;
}

static int sendQueueMsgToResetCameraAndSyncModule(unsigned char index)
{
    int ret = 0;
    unsigned char *camera_reset = NULL;
    unsigned char *sync_module_reset = NULL;

    camera_reset = (unsigned char *)malloc(sizeof(unsigned char));
    if(camera_reset != NULL)
    {
        *camera_reset = 1;

        ret = xQueueSend((key_t)KEY_CAMERA_RESET_MSG + index,camera_reset,MAX_QUEUE_MSG_NUM);
        if(ret == -1)
        {
            fprintf(stderr, "%s: send camera reset queue msg failed\n",__func__);
        }
    }

    sync_module_reset = (unsigned char *)malloc(sizeof(unsigned char));
    if(sync_module_reset != NULL)
    {
        *sync_module_reset = 1;

        ret = xQueueSend((key_t)KEY_SYNC_MODULE_RESET_MSG,sync_module_reset,MAX_QUEUE_MSG_NUM);
        if(ret == -1)
        {
            fprintf(stderr, "%s: send sync module reset queue msg failed\n",__func__);
        }
    }

    return ret;
}

void *thread_sync(void *arg)
{
    int ret = 0;
    unsigned char index = 0;
    struct CmdArgs *args = (struct CmdArgs *)arg;

    index = args->camera_index;

    while(1)
    {
        ret = syncImageAndCameraTimeStamp(index);
        if(ret == -1)
        {
            sendQueueMsgToResetCameraAndSyncModule(index);

            fprintf(stderr, "%s: send queue msg to reset camera and sync module\n",__func__);
        }
    }
}