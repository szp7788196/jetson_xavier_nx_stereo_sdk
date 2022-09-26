#include <pthread.h>
#include "odb2.h"
#include "can.h"


void *thread_odb2(void *arg)
{
    int ret = 0;
    int fd = -1;
    unsigned char send_buf[8] = {0x02,0x01,0x0D,0x00,0x00,0x00,0x00,0x00};
    struct can_frame frame;

    fd = canInit();
    if(fd == -1)
    {
        fprintf(stderr, "%s: init can socket failed\n",__func__);
        goto THREAD_EXIT;
    }

    while(1)
    {
        canSend(fd, 0x7DF, send_buf, 8);
        usleep(1000 * 100);

        ret = canFrameReceive(fd,&frame);
        if(ret > 0)
        {
            fprintf(stdout, "%s: recv can frame id = 0x%08X; len = %d\n",__func__,frame.can_id,frame.can_dlc);
            fprintf(stdout, "%s: recv can frame buf : %02X %02X %02X %02X %02X %02X %02X %02X\n",
            __func__,
            frame.data[0],frame.data[1],frame.data[2],frame.data[3],
            frame.data[4],frame.data[5],frame.data[6],frame.data[7]);
            fprintf(stdout, "%s: VEHICLE_SPEED = %d\n",__func__,frame.data[3]);
        }

        usleep(1000 * 900);
    }

THREAD_EXIT:
    pthread_exit("thread_odb2: init can socket failed\n");
}