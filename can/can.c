#include "can.h"

int canInit(void)
{
    int s;
	static struct sockaddr_can addr;
	static struct ifreq ifr;
    int flags;

	s = socket(PF_CAN,SOCK_RAW,CAN_RAW);

	strcpy(ifr.ifr_name,"can0");
	ioctl(s,SIOCGIFINDEX,&ifr);

	addr.can_family = AF_CAN;
	addr.can_ifindex = ifr.ifr_ifindex;
	bind(s,(struct sockaddr *)&addr,sizeof(addr));

    return s;
}

void canDeInit(int fd)
{
    close(fd);
}

int canFrameSend(int fd,struct can_frame frame)
{
	int ret = 0;
    int i = 0;

    // fprintf(stdout, "%s: #%04X-send[%d]\n",__func__,frame.can_id,frame.can_dlc);
	for(i = 0; i < frame.can_dlc && i < 8; i ++)
	{
        printf("%02X ",frame.data[i]);
    }
	printf("\n");

//	ret = write(fd,&frame,sizeof(frame));
    ret = send(fd, &frame, sizeof(frame), 0);
	if(ret < 0)
    {
        fprintf(stderr, "%s: can write failed:[%d]-%s\n",__func__,errno,strerror(errno));
    }
	else if(ret != sizeof(frame))
    {
        fprintf(stderr, "%s: can write() return value is %d\n",__func__,ret);
    }

    return ret;
}

int canSend(int fd,unsigned int id,unsigned char *buf,unsigned short len)
{
    int ret = 0;
	unsigned char i = 0;
    unsigned short j = 0;
    unsigned int frame_id = id;
    unsigned short length = 0;
    unsigned short length1 = 0;
	struct can_frame frame;

    frame_id &= ~(1 << ERR_FRAME_FLAG_BIT);
    frame_id &= ~(1 << RTR_FRAME_BIT);

    if(len <= 8)
    {
        frame_id &= ~(1 << FRAME_FORMAT_BIT);
    }
    else
    {
        frame_id |= (1 << FRAME_FORMAT_BIT);
    }

    frame.can_id = frame_id;

    if(len <= 8)
    {
        length = len;
    }
    else
    {
        length = 8;
    }

    while(1)
    {
        frame.can_dlc = length;

        // fprintf(stdout, "%s: #%04X-send[%d]\n",__func__,id,length);

        for(i = 0; i < length && i < 8; i ++)
        {
            frame.data[i] = buf[length1 + i];
            /* printf("%02X ",buf[length1 + i]); */
        }
        /* printf("\n"); */

        ret = send(fd, &frame, sizeof(frame), 0);

        if(ret < 0)
        {
            //fprintf(stderr, "%s: can write failed:[%d]-%s\n",__func__,errno,strerror(errno));
        }
        else if(ret != sizeof(frame))
        {
            fprintf(stderr, "%s: can write() return value is %d\n",__func__,ret);
        }

        length1 += length;
		length = len - length1;

        if(length == 0)
		{
			break;
		}

        if(length > 8)
		{
			length = 8;
		}

        usleep(100);
    }

    return ret;
}

unsigned short canFrameReceive(int fd,struct can_frame *frame)
{
	int ret = 0;
    int i = 0;
	struct pollfd fds[1];

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	ret = poll(fds,1,1000);
	if(ret<0)
    {
        fprintf(stderr, "%s: can poll failed: %s\n",__func__,strerror(errno));
    }
	else if(ret==0)
    {
        // fprintf(stderr, "%s: can poll timeout: %d\n",__func__,ret);
    }
	else
	{
		ret = read(fd,frame,sizeof(struct can_frame));
		if(ret < 0)
        {
            fprintf(stderr, "%s: can read failed: %s\n",__func__,strerror(errno));
        }

		/* fprintf(stderr, "%s: #%02X-recv[%d]:\n",__func__,frame->can_id,frame->can_dlc);

		for(i = 0; i < frame->can_dlc && i <= 8; i ++)
		{
            printf("%02X ",frame->data[i]);
        }
        printf("\n"); */

		return frame->can_dlc;
	}

	return 0;
}
