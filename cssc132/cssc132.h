#ifndef __CSSC132_H
#define __CSSC132_H

#include <sys/ioctl.h>


#define CSSC132_MAX_SUPPORT_FORMAT_NUM              10
#define CSSC132_MIN_USER_CONFIG_FILE_LEN            24
#define CSSC132_MAX_USER_CONFIG_FILE_LEN            1024

/****************************************** ioctl cmd ******************************************/
#define IOC_CSSC132_CTRL_MAGIC                           			'C'
#define IOC_CSSC132_CTRL_INIT                            			_IO(IOC_CSSC132_CTRL_MAGIC, 0)
#define IOC_CSSC132_CTRL_R_DEVICE_ID                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 1, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_VER                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 2, int)
#define IOC_CSSC132_CTRL_R_FIRMWARE_VER                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 3, int)
#define IOC_CSSC132_CTRL_R_CAM_CAP                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 4, int)
#define IOC_CSSC132_CTRL_R_PRODUCT_MODULE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 5, int)
#define IOC_CSSC132_CTRL_R_SUPPORT_FORMAT                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 6, int)
#define IOC_CSSC132_CTRL_R_CURRENT_FORMAT                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 7, int)
#define IOC_CSSC132_CTRL_W_CURRENT_FORMAT                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 8, int)
#define IOC_CSSC132_CTRL_R_ISP_CAP                       	 		_IOR(IOC_CSSC132_CTRL_MAGIC, 9, int)
#define IOC_CSSC132_CTRL_W_PARAMS_SAVE                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 10, int)
#define IOC_CSSC132_CTRL_R_POWER_HZ                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 11, int)
#define IOC_CSSC132_CTRL_W_POWER_HZ                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 12, int)
#define IOC_CSSC132_CTRL_W_SYS_RESET                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 13, int)
#define IOC_CSSC132_CTRL_R_I2C_ADDR                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 14, int)
#define IOC_CSSC132_CTRL_W_I2C_ADDR                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 15, int)
#define IOC_CSSC132_CTRL_R_STREAM_MODE                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 16, int)
#define IOC_CSSC132_CTRL_W_STREAM_MODE                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 17, int)
#define IOC_CSSC132_CTRL_R_DAY_NIGHT_MODE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 18, int)
#define IOC_CSSC132_CTRL_W_DAY_NIGHT_MODE                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 19, int)
#define IOC_CSSC132_CTRL_R_HUE                        				_IOR(IOC_CSSC132_CTRL_MAGIC, 20, int)
#define IOC_CSSC132_CTRL_W_HUE                        				_IOW(IOC_CSSC132_CTRL_MAGIC, 21, int)
#define IOC_CSSC132_CTRL_R_CONTRAST                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 22, int)
#define IOC_CSSC132_CTRL_W_CONTRAST                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 23, int)
#define IOC_CSSC132_CTRL_R_STATURATION                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 24, int)
#define IOC_CSSC132_CTRL_W_STATURATION                         		_IOW(IOC_CSSC132_CTRL_MAGIC, 25, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_STATE                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 26, int)
#define IOC_CSSC132_CTRL_R_WB_STATE                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 27, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_FRAME_MODE                       _IOR(IOC_CSSC132_CTRL_MAGIC, 28, int)
#define IOC_CSSC132_CTRL_W_EXPOSURE_FRAME_MODE                       _IOW(IOC_CSSC132_CTRL_MAGIC, 29, int)
#define IOC_CSSC132_CTRL_R_SLOW_SHUTTER_GAIN                        	_IOR(IOC_CSSC132_CTRL_MAGIC, 30, int)
#define IOC_CSSC132_CTRL_W_SLOW_SHUTTER_GAIN                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 31, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_MODE                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 32, int)
#define IOC_CSSC132_CTRL_W_EXPOSURE_MODE                       		_IOW(IOC_CSSC132_CTRL_MAGIC, 33, int)
#define IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_TIME                      _IOR(IOC_CSSC132_CTRL_MAGIC, 34, int)
#define IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_TIME                      _IOW(IOC_CSSC132_CTRL_MAGIC, 35, int)
#define IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_AGAIN                     _IOR(IOC_CSSC132_CTRL_MAGIC, 36, int)
#define IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_AGAIN                     _IOW(IOC_CSSC132_CTRL_MAGIC, 37, int)
#define IOC_CSSC132_CTRL_R_MANUAL_EXPOSURE_DGAIN                     _IOR(IOC_CSSC132_CTRL_MAGIC, 38, int)
#define IOC_CSSC132_CTRL_W_MANUAL_EXPOSURE_DGAIN                     _IOW(IOC_CSSC132_CTRL_MAGIC, 39, int)
#define IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_TIME               _IOR(IOC_CSSC132_CTRL_MAGIC, 40, int)
#define IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_TIME               _IOW(IOC_CSSC132_CTRL_MAGIC, 41, int)
#define IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_AGAIN              _IOR(IOC_CSSC132_CTRL_MAGIC, 42, int)
#define IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_AGAIN              _IOW(IOC_CSSC132_CTRL_MAGIC, 43, int)
#define IOC_CSSC132_CTRL_R_DIRECT_MANUAL_EXPOSURE_DGAIN              _IOR(IOC_CSSC132_CTRL_MAGIC, 44, int)
#define IOC_CSSC132_CTRL_W_DIRECT_MANUAL_EXPOSURE_DGAIN				_IOW(IOC_CSSC132_CTRL_MAGIC, 45, int)
#define IOC_CSSC132_CTRL_R_AWB_MODE                      			_IOR(IOC_CSSC132_CTRL_MAGIC, 46, int)
#define IOC_CSSC132_CTRL_W_AWB_MODE                      			_IOW(IOC_CSSC132_CTRL_MAGIC, 47, int)
#define IOC_CSSC132_CTRL_R_MWB_COLOR_TEMPERATURE         			_IOR(IOC_CSSC132_CTRL_MAGIC, 48, int)
#define IOC_CSSC132_CTRL_W_MWB_COLOR_TEMPERATURE             		_IOW(IOC_CSSC132_CTRL_MAGIC, 49, int)
#define IOC_CSSC132_CTRL_R_MWB_GAIN                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 50, int)
#define IOC_CSSC132_CTRL_W_MWB_GAIN                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 51, int)
#define IOC_CSSC132_CTRL_R_IMAGE_DIRECTION                   		_IOR(IOC_CSSC132_CTRL_MAGIC, 52, int)
#define IOC_CSSC132_CTRL_W_IMAGE_DIRECTION                         	_IOW(IOC_CSSC132_CTRL_MAGIC, 53, int)
#define IOC_CSSC132_CTRL_R_EXPOSURE_TARGET_BRIGHTNESS                _IOR(IOC_CSSC132_CTRL_MAGIC, 54, int)
#define IOC_CSSC132_CTRL_W_EXPOSURE_TARGET_BRIGHTNESS                _IOW(IOC_CSSC132_CTRL_MAGIC, 55, int)
#define IOC_CSSC132_CTRL_R_AUTO_EXPOSURE_MAX_TIME                    _IOR(IOC_CSSC132_CTRL_MAGIC, 56, int)
#define IOC_CSSC132_CTRL_W_AUTO_EXPOSURE_MAX_TIME                    _IOW(IOC_CSSC132_CTRL_MAGIC, 57, int)
#define IOC_CSSC132_CTRL_R_AUTO_EXPOSURE_MAX_GAIN                    _IOR(IOC_CSSC132_CTRL_MAGIC, 58, int)
#define IOC_CSSC132_CTRL_W_AUTO_EXPOSURE_MAX_GAIN                    _IOW(IOC_CSSC132_CTRL_MAGIC, 59, int)
#define IOC_CSSC132_CTRL_W_SOFTWARE_TRIGGER_ONE                      _IOW(IOC_CSSC132_CTRL_MAGIC, 60, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_EDGE                     _IOR(IOC_CSSC132_CTRL_MAGIC, 61, int)
#define IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_EDGE                     _IOW(IOC_CSSC132_CTRL_MAGIC, 62, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_DELETE_BOUNCER_TIME		_IOR(IOC_CSSC132_CTRL_MAGIC, 63, int)
#define IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_DELETE_BOUNCER_TIME      _IOW(IOC_CSSC132_CTRL_MAGIC, 64, int)
#define IOC_CSSC132_CTRL_R_HARDWARE_TRIGGER_DELAY                    _IOR(IOC_CSSC132_CTRL_MAGIC, 65, int)
#define IOC_CSSC132_CTRL_W_HARDWARE_TRIGGER_DELAY                    _IOW(IOC_CSSC132_CTRL_MAGIC, 66, int)
#define IOC_CSSC132_CTRL_R_PICK_MODE                        			_IOR(IOC_CSSC132_CTRL_MAGIC, 67, int)
#define IOC_CSSC132_CTRL_W_PICK_MODE                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 68, int)
#define IOC_CSSC132_CTRL_W_PICK_ONE                        			_IOW(IOC_CSSC132_CTRL_MAGIC, 69, int)
#define IOC_CSSC132_CTRL_R_MIPI_STATUS                        		_IOR(IOC_CSSC132_CTRL_MAGIC, 70, int)
#define IOC_CSSC132_CTRL_W_SYSTEM_REBOOT                        		_IOW(IOC_CSSC132_CTRL_MAGIC, 71, int)
#define IOC_CSSC132_CTRL_W_LED_STROBE_ENABLE                        	_IOW(IOC_CSSC132_CTRL_MAGIC, 72, int)
#define IOC_CSSC132_CTRL_R_YUV_SEQUENCE                       		_IOR(IOC_CSSC132_CTRL_MAGIC, 73, int)
#define IOC_CSSC132_CTRL_W_YUV_SEQUENCE                        	    _IOW(IOC_CSSC132_CTRL_MAGIC, 74, int)

#define IOC_CSSC132_CTRL_MAX_NUM                         			74

/****************************************** structs ******************************************/
struct Cssc132Format
{
	unsigned short width;
    unsigned short height;
    unsigned short frame_rate;
};

struct Cssc132SupportFormat
{
	unsigned char number;
    struct Cssc132Format format[CSSC132_MAX_SUPPORT_FORMAT_NUM];
};

struct WhiteBalanceState
{
	char rgain;
	char ggain;
	char bgain;
	int color_temp;
};

struct GainDisassemble
{
	unsigned char inter;
	unsigned char dec;
};

struct ExposureState
{
	unsigned int expo_time; 
	struct GainDisassemble again;
	struct GainDisassemble dgain;
};

struct HardwareTriggerDeleteBouncerTime
{
	unsigned char enable;
    unsigned int time;
};

struct MipiStatus {
	unsigned int count;
	unsigned char start;
};

struct FrameBuffer
{
    void *start;
	unsigned int length;
};

struct Cssc132Config
{
    char *camera;                                                                   //??????????????????
    char *ctrl;                                                                     //????????????????????????
    int camera_fd;                                                                  //?????????????????????
    int ctrl_fd;                                                                    //?????????????????????????????????
    unsigned char camera_index;                                                     //????????????
    unsigned char i2c_addr;                                                         //??????I2C????????????
    unsigned char device_id;                                                        //??????ID
    unsigned char hardware_ver;                                                     //????????????
    unsigned char firmware_ver;                                                     //????????????
    unsigned short camera_capability;                                               //?????????????????????
    unsigned short product_module;                                                  //????????????
    struct Cssc132SupportFormat support_format;                                     //?????????????????????
    struct Cssc132Format current_format;                                            //??????????????????
    unsigned int isp_capability;                                                    //?????????ISP???????????????
    unsigned char power_hz;                                                         //????????????(?????????????????????)
    unsigned char stream_mode;                                                      //??????????????????
    unsigned char day_night_mode;                                                   //??????????????????
    unsigned char hue;                                                              //??????
    unsigned char contrast;                                                         //?????????
    unsigned char saturation;                                                       //?????????
    struct ExposureState exposure_state;                                            //????????????
    struct WhiteBalanceState wb_sate;                                               //?????????????????????
    unsigned char exposure_frame_mode;                                              //???????????????????????????/???????????????
    struct GainDisassemble slow_shutter_gain;                                       //??????????????????,?????????????????????????????????,??????dB
    unsigned char exposure_mode;                                                    //????????????
    unsigned int manual_exposure_time;                                              //??????????????????us
    struct GainDisassemble manual_exposure_again;                                   //????????????????????????dB
    struct GainDisassemble manual_exposure_dgain;                                   //????????????????????????dB
    unsigned int direct_manual_exposure_time;                                       //????????????????????????us
    struct GainDisassemble direct_manual_exposure_again;                            //??????????????????????????????dB
    struct GainDisassemble direct_manual_exposure_dgain;                            //??????????????????????????????dB
    unsigned char awb_mode;                                                         //?????????????????????
    unsigned int mwb_color_temperature;                                             //?????????????????????
    struct GainDisassemble mwb_gain;                                                //???????????????????????? rgain???bgain,ggain is always 1
    unsigned char image_direction;                                                  //??????????????????
    unsigned char auto_exposure_target_brightness;                                  //?????????????????????????????????
    unsigned int auto_exposure_max_time;                                            //?????????????????????????????????
    struct GainDisassemble auto_exposure_max_gain;                                  //??????????????????????????????????????????
    unsigned char hardware_trigger_edge;                                            //Hardware Trigger?????????????????????
    struct HardwareTriggerDeleteBouncerTime hardware_trigger_delete_bouncer_time;   //Hardware Trigger??????????????????????????????????????????
    unsigned int hardware_trigger_delay;                                            //Hardware Trigger????????????????????????us
    unsigned char pick_one_mode;                                                    //??????pickone???????????????????????????
    struct MipiStatus mipi_status;                                                  //mipi??????
    unsigned char led_strobe_enable;                                                //????????????????????????
    unsigned char yuv_sequence;                                                     //????????????yuv??????
    unsigned short frame_num;                                                       //???????????????
    unsigned short image_heap_depth;                                                //?????????????????????????????????
    double trigger_frame_rate;                                                      //??????????????????
    struct FrameBuffer *frame_buf;                                                  //????????????
    unsigned short capture_timeout;                                                 //??????????????????ms
};


void *thread_cssc132(void *arg);

#endif
