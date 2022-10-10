#include <pthread.h>
#include "odb2.h"
#include "can.h"
#include "stereo.h"
#include "cmd_parse.h"


void *thread_odb2(void *arg)
{
    int ret = 0;
    int fd = -1;
    struct CmdArgs *args = (struct CmdArgs *)arg;
    unsigned short sample_period = 0;
    unsigned char send_buf[8] = {0x02,0x01,0x0D,0x00,0x00,0x00,0x00,0x00};
    struct can_frame frame;

    sample_period = 1000 / args->vehicle_speed_rate;

    fd = canInit();
    if(fd == -1)
    {
        fprintf(stderr, "%s: init can socket failed\n",__func__);
        goto THREAD_EXIT;
    }

    while(1)
    {
        canSend(fd, 0x7DF, send_buf, 8);
        usleep(1000 * sample_period);

        ret = canFrameReceive(fd,&frame);
        if(ret > 0)
        {
            if(frame.can_id == 0x7E8 && frame.data[0] == 0x03 && frame.data[1] == 0x4D && frame.data[2] == 0x0D)
            {
                struct ODB2_Objects *odb2_objects;

                odb2_objects = (struct ODB2_Objects *)malloc(sizeof(struct ODB2_Objects));
                if(odb2_objects != NULL)
                {
                    odb2_objects->vehicle_speed = frame.data[3];

                    ret = xQueueSend((key_t)KEY_VEHICLE_SPEED_MSG,odb2_objects,MAX_QUEUE_MSG_NUM);
                    if(ret == -1)
                    {
                        free(odb2_objects);
                        odb2_objects = NULL;

                        fprintf(stderr, "%s: send odb2_objects queue msg failed\n",__func__);
                    }
                }
            }
        }
    }

THREAD_EXIT:
    pthread_exit("thread_odb2: init can socket failed\n");
}