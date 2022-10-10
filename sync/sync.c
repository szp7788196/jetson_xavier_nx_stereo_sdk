#include "sync.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <semaphore.h>
#include "stereo.h"
#include "cmd_parse.h"

static int syncImageAndCameraTimeStamp(unsigned char index)
{
    int ret = 0;
    int res = 0;
    int i = 0;
    static struct SyncCamTimeStamp time_stamp[MAX_CAMERA_NUM];
    static char sync_1hz_success[MAX_CAMERA_NUM] = {0};
    static char got_time_stamp[MAX_CAMERA_NUM] = {0};
    static char got_image[MAX_CAMERA_NUM] = {0};
    struct timespec start_tm;
	struct timespec end_tm;
    int timeout_ms = 5;

    if(got_time_stamp[index] == 0)
    {
        clock_gettime(CLOCK_REALTIME, &start_tm);
        end_tm = ns_to_tm(tm_to_ns(start_tm) + timeout_ms * 1000000);

        if(sem_timedwait(&sem_t_SyncCamTimeStampHeap[index], &end_tm) == 0)
        {
            res = syncCamTimeStampHeapGet(index,&time_stamp[index]);
            if(res == 0)
            {
                got_time_stamp[index] = 1;
            }
        }
        else
        {
            // fprintf(stderr, "%s: wait time_stamp[%d] cond timeout or error\n",__func__,index);
        }
    }

    if(got_image[index] == 0)
    {
        clock_gettime(CLOCK_REALTIME, &start_tm);
        end_tm = ns_to_tm(tm_to_ns(start_tm) + timeout_ms * 1000000);

        if(sem_timedwait(&sem_t_ImageHeap[index], &end_tm) == -1)
        {
            res = imageHeapGet(index,&imageBuffer[index]);
            if(res == 0)
            {
                got_image[index] = 1;
            }
        }
        else
        {
            // fprintf(stderr, "%s: wait imageBuffer cond timeout or error\n",__func__);
        }
    }

    if(got_time_stamp[index] == 1 && got_image[index] == 1)
    {
        got_time_stamp[index] = 0;
        got_image[index] = 0;

        if(time_stamp[index].number != imageBuffer[index].number)
        {
            fprintf(stderr, "%s:sync failed  time_stamp[%d].number = %d, imageBuffer[%d].number = %d\n",
                    __func__,
                    index,
                    time_stamp[index].number,
                    index,
                    imageBuffer[index].number);

            imageHeap[index].put_ptr = 0;
            imageHeap[index].get_ptr = 0;

            for(i = 0; i < imageHeap[index].depth; i ++)
            {
                imageHeap[index].heap[i]->number = 0;
            }

            syncCamTimeStampHeap[index].put_ptr = 0;
            syncCamTimeStampHeap[index].get_ptr = 0;

            for(i = 0; i < syncCamTimeStampHeap[index].depth; i ++)
            {
                syncCamTimeStampHeap[index].heap[i]->number = 0;
            }

            sync_1hz_success[index] = 0;
            ret = -1;

            goto GET_OUT;
        }
        else
        {
            if(sync_1hz_success[index] < 10)
            {
                sync_1hz_success[index] ++;

                if(sync_1hz_success[index] == 10)
                {
                    ret = xQueueSend((key_t)KEY_SYNC_1HZ_SUCCESS_MSG,&sync_1hz_success[index],MAX_QUEUE_MSG_NUM);
                    if(ret == -1)
                    {
                        fprintf(stderr, "%s: send sync_1hz_success[%d] queue msg failed\n",__func__,index);
                    }
                }
            }

            imageUnitHeapPut(index,&imageBuffer[index], &time_stamp[index]);

            sem_post(&sem_t_ImageUnitHeap[index]);
        }
    }

GET_OUT:
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
            free(camera_reset);
            camera_reset = NULL;

            fprintf(stderr, "%s: send camera[%d] reset queue msg failed\n",__func__,index);
        }
    }

    sync_module_reset = (unsigned char *)malloc(sizeof(unsigned char));
    if(sync_module_reset != NULL)
    {
        *sync_module_reset = 1;

        ret = xQueueSend((key_t)KEY_SYNC_MODULE_RESET_MSG,sync_module_reset,MAX_QUEUE_MSG_NUM);
        if(ret == -1)
        {
            free(sync_module_reset);
            sync_module_reset = NULL;

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
            // sendQueueMsgToResetCameraAndSyncModule(index);

            fprintf(stderr, "%s: send queue msg to reset camera[%d] and sync module\n",__func__,index);
        }
    }
}