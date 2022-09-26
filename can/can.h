#ifndef __CAN_H
#define __CAN_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <getopt.h>
#include <poll.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>


#define FRAME_FORMAT_BIT                        31
#define RTR_FRAME_BIT                           30
#define ERR_FRAME_FLAG_BIT                      29


int canInit(void);
void canDeInit(int fd);
int canFrameSend(int fd,struct can_frame frame);
int canSend(int fd,unsigned int id,unsigned char *buf,unsigned short len);
unsigned short canFrameReceive(int fd,struct can_frame *frame);


#endif
