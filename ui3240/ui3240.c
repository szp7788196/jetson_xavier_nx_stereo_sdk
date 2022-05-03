#include "ui3240.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h> 
#include <unistd.h>
#include <sys/types.h>    
#include <sys/stat.h>
#include <fcntl.h>
#include "monocular.h"
#include "cmd_parse.h"


static bool isConnected(struct Ui3240Config config)
{
    return (config.camera_handle != (HIDS)0); 
}

static bool freeRunModeActive(struct Ui3240Config config)
{
    return ((config.camera_handle != (HIDS) 0) &&
           (is_SetExternalTrigger(config.camera_handle, IS_GET_EXTERNALTRIGGER) == IS_SET_TRIGGER_OFF));
}

static bool extTriggerModeActive(struct Ui3240Config config)
{
    return ((config.camera_handle != (HIDS) 0) &&
           (is_SetExternalTrigger(config.camera_handle, IS_GET_EXTERNALTRIGGER) == IS_SET_TRIGGER_HI_LO || 
            is_SetExternalTrigger(config.camera_handle, IS_GET_EXTERNALTRIGGER) == IS_SET_TRIGGER_LO_HI));
}

static bool softTriggerModeActive(struct Ui3240Config config)
{
    return (( config.camera_handle != (HIDS) 0) &&
           (is_SetExternalTrigger(config.camera_handle, IS_GET_EXTERNALTRIGGER) == IS_SET_TRIGGER_SOFTWARE));
}

static bool isCapturing(struct Ui3240Config config)
{
    return ((config.camera_handle != (HIDS) 0) &&
           (is_CaptureVideo(config.camera_handle, IS_GET_LIVE) == TRUE));
}

static int gpioPwmConfig(HIDS hCam, double frame_rate, bool active)
{
    // Set GPIO2 as PWM output
    unsigned int nMode = IO_GPIO_2;
    int is_err = is_IO(hCam, IS_IO_CMD_PWM_SET_MODE,(void*)&nMode, sizeof(nMode));
    IO_PWM_PARAMS m_pwmParams;

    // Set the values of the PWM parameters
    m_pwmParams.dblFrequency_Hz = frame_rate;
    m_pwmParams.dblDutyCycle = (active ? 0.1:0.0); //TODO: What does the duty change?(active ? 0.1:0)

    is_err = is_IO(hCam, IS_IO_CMD_PWM_SET_PARAMS,(void*)&m_pwmParams, sizeof(m_pwmParams));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: pwm not set,error code is %d\n",__func__,is_err);
    }

    return is_err;
}

static int gpioInputConfig(HIDS hCam)
{
    // FOR THE GPIO 1 : INPUT
    IO_GPIO_CONFIGURATION gpioConfiguration;
    int is_err = IS_SUCCESS;

    // Set configuration of GPIO1
    gpioConfiguration.u32Gpio = IO_GPIO_1;
    gpioConfiguration.u32Configuration = IS_GPIO_TRIGGER;
    gpioConfiguration.u32State = 0;

    is_err = is_IO(hCam, IS_IO_CMD_GPIOS_SET_CONFIGURATION, (void*)&gpioConfiguration,sizeof(gpioConfiguration));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: GPIO1 config not done,error code is %d\n",__func__,is_err);
    }

    // // set configuration of GPIO2
    gpioConfiguration.u32Gpio = IO_GPIO_2;
    gpioConfiguration.u32Configuration = IS_GPIO_FLASH;
    gpioConfiguration.u32State = 0;
    is_err = is_IO(hCam, IS_IO_CMD_GPIOS_SET_CONFIGURATION, (void*)&gpioConfiguration,sizeof(gpioConfiguration));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: GPIO2 config not done,error code is %d\n",__func__,is_err);
    }

    return is_err;
}

static int setStandbyMode(struct Ui3240Config config)
{
    int is_err = IS_SUCCESS;
    unsigned int nMode = IO_FLASH_MODE_OFF;
    unsigned int event = IS_SET_EVENT_FRAME;
    IS_INIT_EVENT init_event = {IS_SET_EVENT_FRAME, TRUE, FALSE};

    if(!isConnected(config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    if(extTriggerModeActive(config))
    {
        /* set the GPIO2 to generate PWM */
        is_err = gpioPwmConfig(config.camera_handle, config.frame_rate, false);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set GPIO2 as output (PWM),error code is %d\n",__func__,is_err);
            return is_err;
        }

        is_err = is_SetExternalTrigger(config.camera_handle, IS_SET_TRIGGER_OFF);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not disable external trigger mode,error code is %d\n",__func__,is_err);
            return is_err;
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_INIT, &init_event, sizeof(IS_INIT_EVENT));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not init frame event 1,error code is %d\n",__func__,is_err);
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_DISABLE, &event, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not disable frame event 1,error code is %d\n",__func__,is_err);
            return is_err;
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_EXIT, &event, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not exit frame event,error code is %d\n",__func__,is_err);
        }

        // documentation seems to suggest that this is needed to disable external trigger mode (to go into free-run mode)
        is_SetExternalTrigger(config.camera_handle, IS_GET_TRIGGER_STATUS);
        
        is_err = is_StopLiveVideo(config.camera_handle, IS_WAIT);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not stop live video mode,error code is %d\n",__func__,is_err);
            return is_err;
        }

        fprintf(stderr, "%s: Stopped external trigger mode\n",__func__);
    }
    else if(freeRunModeActive(config))
    {
        is_err = is_IO(config.camera_handle, IS_IO_CMD_FLASH_SET_MODE,(void*)&nMode, sizeof(nMode));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not disable flash output,error code is %d\n",__func__,is_err);
            return is_err;
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_INIT, &init_event, sizeof(IS_INIT_EVENT));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not init frame event 2,error code is %d\n",__func__,is_err);
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_DISABLE, &event, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not disable frame event 2,error code is %d\n",__func__,is_err);
            return is_err;
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_EXIT, &event, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not exit frame event,error code is %d\n",__func__,is_err);
        }

        is_err = is_StopLiveVideo(config.camera_handle, IS_WAIT);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not stop live video mode,error code is %d\n",__func__,is_err);
            return is_err;
        }

        fprintf(stderr, "%s: Stopped free-run live video mode\n",__func__);
    }
    else if(softTriggerModeActive(config))
    {
        
    }

    is_err = is_CameraStatus(config.camera_handle, IS_STANDBY, IS_GET_STATUS);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not set standby mode,error code is %d\n",__func__,is_err);
        return is_err;
    }

    return is_err;
}

static int connectCamrea(int camera_id,struct Ui3240Config *config)
{
    int is_err = IS_SUCCESS;
    int num_cameras = 0;

    // Terminate any existing opened cameras
    setStandbyMode(*config);

    is_err = is_GetNumberOfCameras(&num_cameras);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed query for number of connected UEye cameras,error code is %d\n",__func__,is_err);
        return is_err;
    }
    else
    {
        if(num_cameras < 1)
        {
            fprintf(stderr, "%s: No UEye cameras are connected\n",__func__);
            return IS_NO_SUCCESS;
        }
    }

    config->camera_handle = (HIDS)config->camera_id;

    is_err = is_InitCamera(&config->camera_handle, NULL);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not open UEye camera ID %d\n",__func__,config->camera_id);
        return is_err;
    }

    is_err = is_SetDisplayMode(config->camera_handle, IS_SET_DM_DIB);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: does not support Device Independent Bitmap mode,error code is %d\n",__func__,is_err);
        return is_err;
    }

    is_err = is_GetSensorInfo(config->camera_handle, &config->camera_sensor_info);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not poll sensor information,error code is %d\n",__func__,is_err);
        return is_err;
    }
    
    return is_err;
}

static int disconnectCamera(struct Ui3240Config *config)
{
  int is_err = IS_SUCCESS;
  int i = 0;

  if(isConnected(*config))
  {
    setStandbyMode(*config);

    // Exit the image queue and clear sequence
    is_err = is_ImageQueue(config->camera_handle, IS_IMAGE_QUEUE_CMD_EXIT, NULL, 0);
    is_err = is_ClearSequence(config->camera_handle);

    // Release existing camera buffers
    if(config->frame_buf != NULL)
    {
        for(i = 0; i < config->frame_num; i ++)
        {
            if(config->frame_buf[i] != NULL)
            {
                is_err = is_FreeImageMem(config->camera_handle, config->frame_buf[i], config->frame_buf_id[i]);
                if(is_err != IS_SUCCESS)
                {
                    fprintf(stderr, "%s: Failed to free frame_buf[%d]\n",__func__,i);
                }
            }

            config->frame_buf[i] = NULL;
        }

        free(config->frame_buf);
        config->frame_buf = NULL;
    }

    if(config->frame_buf_id != NULL)
    {
        free(config->frame_buf_id);
        config->frame_buf_id = NULL;
    }

    // Release camera handle
    is_err = is_ExitCamera(config->camera_handle);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to release camera handle,error code is %d\n",__func__,is_err);
    }

    config->camera_handle = (HIDS)0;

    fprintf(stderr, "%s: camera disconnected success\n",__func__);
  }

  return is_err;
}

static int loadCameraDefaultConfig(struct Ui3240Config config, wchar_t *filename, bool ignore_load_failure)
{
    int is_err = IS_SUCCESS;

    if(!isConnected(config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    is_err = is_ParameterSet(config.camera_handle, IS_PARAMETERSET_CMD_LOAD_FILE,filename, 0);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not load camera default parameters file,error code is %d\n",__func__,is_err);

        if(ignore_load_failure)
        {
            is_err = IS_SUCCESS;
        }

        return is_err;
    }
    else
    {
        fprintf(stdout, "%s: ========================= Congratulations!!! =========================\n",__func__);
        fprintf(stdout, "%s: Successfully loaded camera default parameter file\n",__func__);
    }

    return is_err;
}

static int loadCameraUserConfig(struct Ui3240Config def_config,struct Ui3240Config *user_config,char *filename)
{
    int have_diff = -1;
    FILE *fp;
    char str[128] = {0};
    unsigned int file_len = 0l;
    char *file_buf = NULL;
    char *msg = NULL;
    unsigned short pos = 0;
    char temp_buf[32] = {0};
    long int temp = 0;
    double temp_f = 0.0;
    char *endptr = NULL;

    if(!isConnected(def_config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    fp = fopen(filename, "rt");
    if(fp == NULL)
    {
        fprintf(stderr, "%s: Could not open camera user parameters file\n",__func__);
		return IS_NO_SUCCESS;
    }

    fseek(fp,0,SEEK_END);

    file_len = ftell(fp);
    if(file_len < 1)
    {
        fprintf(stderr, "%s: query camera user parameters file length failed\n",__func__);
		return IS_NO_SUCCESS;
    }

    if(file_len > UI3240_MAX_USER_CONFIG_FILE_LEN || file_len < UI3240_MIN_USER_CONFIG_FILE_LEN)
    {
        fprintf(stderr, "%s: camera user parameters file length error\n",__func__);
		return IS_NO_SUCCESS;
    }

    file_buf = (char *)malloc(sizeof(char) * (file_len + 1));
    if(file_buf == NULL)
    {
        fprintf(stderr, "%s: alloc user parameters file buf failed\n",__func__);
		return IS_NO_SUCCESS;
    }

    memset(file_buf,0,file_len + 1);

    fseek(fp,0,SEEK_SET);

    while(fgets(str, 100, fp) != NULL)
    {
        strcat(file_buf,str);
    }

    fclose(fp);

    if(strstr(file_buf, "start:") == NULL && strstr(file_buf, "end;") == NULL)
    {
        fprintf(stderr, "%s: camera user parameters file missing head or tail\n",__func__);
		return IS_NO_SUCCESS;
    }

    memcpy(user_config,&def_config,sizeof(struct Ui3240Config));

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"master", file_len, strlen("master"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 1)
            {
                user_config->master = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"stereo", file_len, strlen("stereo"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 1)
            {
                user_config->stereo = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"capture_mode", file_len, strlen("capture_mode"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 1)
            {
                user_config->capture_mode = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"capture_timeout", file_len, strlen("capture_timeout"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->capture_timeout = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"image_width", file_len, strlen("image_width"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->image_width = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"image_height", file_len, strlen("image_height"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->image_height = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"image_left", file_len, strlen("image_left"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->image_left = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"image_top", file_len, strlen("image_top"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->image_top = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"color_mode", file_len, strlen("color_mode"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "mono8") != NULL)
            {
                user_config->color_mode = IS_CM_MONO8;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "bayer_rggb8") != NULL)
            {
                user_config->color_mode = IS_CM_SENSOR_RAW8;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "rgb8") != NULL)
            {
                user_config->color_mode = IS_CM_RGB8_PACKED;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "bgr8") != NULL)
            {
                user_config->color_mode = IS_CM_BGR8_PACKED;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"subsampling", file_len, strlen("subsampling"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->subsampling = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"binning", file_len, strlen("binning"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->binning = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"binning", file_len, strlen("binning"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->binning = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"sensor_scaling", file_len, strlen("sensor_scaling"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp_f = atof(temp_buf);
            if(temp_f <= 65535)
            {
                user_config->sensor_scaling = temp_f;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"auto_gain", file_len, strlen("auto_gain"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->auto_gain = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->auto_gain = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"master_gain", file_len, strlen("master_gain"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 255)
            {
                user_config->master_gain = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"red_gain", file_len, strlen("red_gain"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 255)
            {
                user_config->red_gain = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"green_gain", file_len, strlen("green_gain"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 255)
            {
                user_config->green_gain = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"blue_gain", file_len, strlen("blue_gain"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 255)
            {
                user_config->blue_gain = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"gain_boost", file_len, strlen("gain_boost"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->gain_boost = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->gain_boost = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"auto_exposure", file_len, strlen("auto_exposure"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->auto_exposure = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->auto_exposure = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"exposure_ms", file_len, strlen("exposure_ms"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp_f = atof(temp_buf);
            if(temp_f <= 65535)
            {
                user_config->exposure = temp_f;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"auto_white_balance", file_len, strlen("auto_white_balance"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->auto_white_balance = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->auto_white_balance = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"white_balance_red_offset", file_len, strlen("white_balance_red_offset"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 127 && temp > -127)
            {
                user_config->white_balance_red_offset = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"white_balance_blue_offset", file_len, strlen("white_balance_blue_offset"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 127 && temp > -127)
            {
                user_config->white_balance_blue_offset = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"auto_frame_rate", file_len, strlen("auto_frame_rate"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->auto_frame_rate = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->auto_frame_rate = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"frame_rate_hz", file_len, strlen("frame_rate_hz"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp_f = atof(temp_buf);
            if(temp_f <= 65535)
            {
                user_config->frame_rate = temp_f;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"pixel_clock", file_len, strlen("pixel_clock"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 65535)
            {
                user_config->pixel_clock = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"ext_trigger_mode", file_len, strlen("ext_trigger_mode"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->ext_trigger_mode = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->ext_trigger_mode = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"ext_trigger_delay", file_len, strlen("ext_trigger_delay"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp <= 4000000 && temp >= 15)
            {
                user_config->ext_trigger_delay = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"flash_delay", file_len, strlen("flash_delay"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp > 0)
            {
                user_config->flash_delay = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"flash_duration", file_len, strlen("flash_duration"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            temp = strtol(temp_buf, &endptr, 10);
            if(temp > 0)
            {
                user_config->flash_duration = temp;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"flip_upd", file_len, strlen("flip_upd"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->flip_upd = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->flip_upd = false;
                have_diff = IS_SUCCESS;
            }
        }
    }

    msg = file_buf;
    pos = mystrstr((unsigned char *)file_buf, (unsigned char *)"flip_lr", file_len, strlen("flip_lr"));
    if(pos != 0xFFFF && pos < file_len)
    {
        msg += pos;

        memset(temp_buf,0,32);

        get_str1((unsigned char *)msg, (unsigned char *)" = ", 1, (unsigned char *)";", 1, (unsigned char *)temp_buf);
        if(strlen(temp_buf) != 0 && strlen(temp_buf) < 32)
        {
            if(strstr(msg, "true") != NULL)
            {
                user_config->flip_lr = true;
                have_diff = IS_SUCCESS;
            }
            else if(strstr(msg, "false") != NULL)
            {
                user_config->flip_lr = false;
            }
        }
    }

    return have_diff;
}

static int reallocateCameraBuffer(struct Ui3240Config *config)
{
    int is_err = IS_SUCCESS;
    int i = 0;
    int frame_width = 0;
    int frame_height = 0;
    int cam_buffer_pitch = 0;

    // Stop capture to prevent access to memory buffer
    setStandbyMode(*config);

    // Free existing memory from previous calls to reallocateCamBuffer()
    if(config->frame_buf != NULL)
    {
        for(i = 0; i < config->frame_num; i ++)
        {
            if(config->frame_buf[i] != NULL)
            {
                is_err = is_FreeImageMem(config->camera_handle, config->frame_buf[i], config->frame_buf_id[i]);
                if(is_err != IS_SUCCESS)
                {
                    fprintf(stderr, "%s: Failed to free frame_buf[%d]\n",__func__,i);
                }
            }

            config->frame_buf[i] = NULL;
        }

        free(config->frame_buf);
        config->frame_buf = NULL;
    }

    if(config->frame_buf_id != NULL)
    {
        free(config->frame_buf_id);
        config->frame_buf_id = NULL;
    }

    config->frame_buf = malloc(config->frame_num * sizeof(char *));
    if(config->frame_buf == NULL)
    {
        fprintf(stderr, "%s: Failed to malloc config->frame_buf\n",__func__);
        return is_err;
    }

    config->frame_buf_id = (int *)malloc(config->frame_num * sizeof(int));
    if(config->frame_buf_id == NULL)
    {
        fprintf(stderr, "%s: Failed to malloc config->frame_buf_id\n",__func__);
        return is_err;
    }

    for(i = 0; i < config->frame_num; i ++)
    {
        config->frame_buf[i] = NULL;
        config->frame_buf_id[i] = 0;
    }

    // Allocate new memory section for IDS driver to use as frame buffer
    frame_width = config->image_width / (config->sensor_scaling * config->subsampling);
    frame_height = config->image_height / (config->sensor_scaling * config->subsampling);

    is_err = is_ClearSequence(config->camera_handle);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to clear sequence,error code is %d\n",__func__,is_err);
    }

    for(i = 0; i < config->frame_num; i ++)
    {
        is_err = is_AllocImageMem(config->camera_handle, frame_width, frame_height,
                                  config->bits_per_pixel,
                                  &config->frame_buf[i],
                                  &config->frame_buf_id[i]);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to allocate image buffer[%d]\n",__func__,i);
            return is_err;
        }

        is_err = is_SetImageMem(config->camera_handle, config->frame_buf[i], config->frame_buf_id[i]);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to associate image buffer to IDS driver,error code is %d\n",__func__,is_err);
            return is_err;
        }

        is_err = is_AddToSequence(config->camera_handle, config->frame_buf[i], config->frame_buf_id[i]);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to add sequence image buffer,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }

    // Initialize the image queue
    is_err = is_ImageQueue(config->camera_handle, IS_IMAGE_QUEUE_CMD_INIT, NULL, 0);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to initialize image queue,error code is %d\n",__func__,is_err);
        return is_err;
    }

    // Flush image queue
    is_err = is_ImageQueue(config->camera_handle, IS_IMAGE_QUEUE_CMD_FLUSH, NULL, 0);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to flush image queue,error code is %d\n",__func__,is_err);
        return is_err;
    }

    // Synchronize internal settings for buffer step size and overall buffer size
    // NOTE: assume that sensor_scaling_rate, subsampling_rate, and cam_binning_rate_
    //       have all been previously validated and synchronized by syncCamConfig()
    is_err = is_GetImageMemPitch(config->camera_handle, &cam_buffer_pitch);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query buffer step size / pitch / stride,error code is %d\n",__func__,is_err);
        return is_err;
    }

    if(cam_buffer_pitch < frame_width)
    {
        fprintf(stderr, "%s: Frame buffer's queried step size is smaller than buffer's expected width\n",__func__);
        fprintf(stderr, "%s: (THIS IS A CODING ERROR, PLEASE CONTACT PACKAGE AUTHOR)\n",__func__);
    }

    config->frame_buf_size = cam_buffer_pitch * frame_height;

    freeImageHeap(config->camera_index);

    is_err = allocateImageHeap(config->camera_index,config->image_heap_depth,config->frame_buf_size);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: allocate image heap failed\n",__func__);
    }

    return is_err;
}

static int setColorMode(struct Ui3240Config config,char *mode) 
{
    int is_err = IS_SUCCESS;

    if(!isConnected(config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Stop capture to prevent access to memory buffer
    setStandbyMode(config);

    // Set to specified color mode
    if(strstr(mode,"rgb8") != NULL)
    {
        is_err = is_SetColorMode(config.camera_handle, IS_CM_RGB8_PACKED);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set color mode to RGB8,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }
    else if(strstr(mode,"bgr8") != NULL)
    {
        is_err = is_SetColorMode(config.camera_handle, IS_CM_BGR8_PACKED);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set color mode to BGR8,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }
    else if(strstr(mode,"bayer_rggb8") != NULL)
    {
        is_err = is_SetColorMode(config.camera_handle, IS_CM_SENSOR_RAW8);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set color mode to BAYER_RGGB8,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }
    else
    {   // Default to MONO8
        is_err = is_SetColorMode(config.camera_handle, IS_CM_MONO8);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set color mode to MONO8,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }

    fprintf(stdout, "%s: Updated color mode to %s\n",__func__,mode);

    return is_err;
}

static int setResolution(struct Ui3240Config *config,unsigned short image_width, unsigned short image_height,short image_left, short image_top)
{
    int is_err = IS_SUCCESS;
    IS_RECT camera_aio;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Validate arguments
    CAP(image_width, UI3240_MIN_IMAGE_WIDTH, (int)config->camera_sensor_info.nMaxWidth);
    CAP(image_height, UI3240_MIN_IMAGE_HEIGHT, (int)config->camera_sensor_info.nMaxHeight);

    if(image_left >= 0 && (int)config->camera_sensor_info.nMaxWidth - image_width - image_left < 0)
    {
        fprintf(stderr, "%s: Cannot set AOI left index to %d with a frame width of %d and sensor max width of %d\n",
                __func__,image_left,image_width,config->camera_sensor_info.nMaxWidth);

        image_left = -1;
    }

    if(image_top >= 0 && (int)config->camera_sensor_info.nMaxHeight - image_height - image_top < 0)
    {
        fprintf(stderr, "%s: Cannot set AOI top index to %d with a frame height of %d and sensor max height of %d\n",
                __func__,image_top,image_height,config->camera_sensor_info.nMaxHeight);

        image_top = -1;
    }

    camera_aio.s32X = (image_left < 0) ? (config->camera_sensor_info.nMaxWidth - image_width) / 2 : image_left;
    camera_aio.s32Y = (image_top < 0) ? (config->camera_sensor_info.nMaxHeight - image_height) / 2 : image_top;
    camera_aio.s32Width = image_width;
    camera_aio.s32Height = image_height;

    config->image_width = camera_aio.s32Width;
    config->image_height = camera_aio.s32Height;
    config->image_left = camera_aio.s32X;
    config->image_top = camera_aio.s32Y;

    is_err = is_AOI(config->camera_handle, IS_AOI_IMAGE_SET_AOI, &camera_aio, sizeof(camera_aio));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to set Area Of Interest (AOI),error code is %d\n",__func__,is_err);
        return is_err;
    }

    fprintf(stdout, "%s: Updated Area Of Interest (AOI)\n",__func__);

    return is_err;
}

static int setSubsampling(struct Ui3240Config *config,int rate)
{
    int is_err = IS_SUCCESS;
    int rate_flag;
    int supportedRates;
    int currRate;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Stop capture to prevent access to memory buffer
    setStandbyMode(*config);

    supportedRates = is_SetSubSampling(config->camera_handle, IS_GET_SUPPORTED_SUBSAMPLING);
    switch(rate)
    {
        case 1:
            rate_flag = IS_SUBSAMPLING_DISABLE;
        break;

        case 2:
            rate_flag = IS_SUBSAMPLING_2X_VERTICAL;
        break;

        case 4:
            rate_flag = IS_SUBSAMPLING_4X_VERTICAL;
        break;

        case 8:
            rate_flag = IS_SUBSAMPLING_8X_VERTICAL;
        break;

        case 16:
            rate_flag = IS_SUBSAMPLING_16X_VERTICAL;
        break;

        default:
            rate = 1;
            rate_flag = IS_SUBSAMPLING_DISABLE;

            fprintf(stderr, "%s: currently has unsupported this subsampling rate,resetting to 1X\n",__func__);
        break;
    }

    if((supportedRates & rate_flag) == rate_flag)
    {
        is_err = is_SetSubSampling(config->camera_handle, rate_flag);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set subsampling rate,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }
    else
    {
        fprintf(stderr, "%s: does not support requested sampling rate of %dX\n",__func__,rate);

        // Query current rate
        currRate = is_SetSubSampling(config->camera_handle, IS_GET_SUBSAMPLING);
        if(currRate == IS_SUBSAMPLING_DISABLE)
        {
            rate = 1;
        }
        else if(currRate == IS_SUBSAMPLING_2X_VERTICAL)
        {
            rate = 2;
        }
        else if(currRate == IS_SUBSAMPLING_4X_VERTICAL)
        {
            rate = 4;
        }
        else if(currRate == IS_SUBSAMPLING_8X_VERTICAL)
        {
            rate = 8;
        }
        else if(currRate == IS_SUBSAMPLING_16X_VERTICAL)
        {
            rate = 16;
        }
        else
        {
            fprintf(stderr, "%s: currently has an unsupported sampling rate %dX,resetting to 1X\n",__func__,currRate);

            is_err = is_SetBinning(config->camera_handle, IS_SUBSAMPLING_DISABLE);
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Failed to set subsampling rate to 1X,error code is %d\n",__func__,is_err);
                return is_err;
            }
        }

        return IS_SUCCESS;
    }

    fprintf(stdout, "%s: Updated subsampling rate to %dX\n",__func__,rate);

    config->subsampling = rate;

    return is_err;
}

static int setBinning(struct Ui3240Config *config,int rate)
{
    int is_err = IS_SUCCESS;
    int rate_flag;
    int supportedRates;
    int currRate;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Stop capture to prevent access to memory buffer
    setStandbyMode(*config);

    supportedRates = is_SetBinning(config->camera_handle, IS_GET_SUPPORTED_BINNING);
    switch(rate) 
    {
        case 1:
            rate_flag = IS_BINNING_DISABLE;
        break;

        case 2:
            rate_flag = IS_BINNING_2X_VERTICAL;
        break;

        case 4:
            rate_flag = IS_BINNING_4X_VERTICAL;
        break;

        case 8:
            rate_flag = IS_BINNING_8X_VERTICAL;
        break;

        case 16:
            rate_flag = IS_BINNING_16X_VERTICAL;
        break;

        default:
            rate = 1;
            rate_flag = IS_BINNING_DISABLE;

            fprintf(stderr, "%s: currently has unsupported binning rate: %dX,resetting to 1X\n",__func__,rate);
        break;
    }

    if((supportedRates & rate_flag) == rate_flag)
    {
        is_err = is_SetBinning(config->camera_handle, rate_flag);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set binning rate to %dX\n",__func__,rate);
            return is_err;
        }
    }
    else
    {
        fprintf(stderr, "%s: does not support requested binning rate of %dX\n",__func__,rate);

        // Query current rate
        currRate = is_SetBinning(config->camera_handle, IS_GET_BINNING);
        if(currRate == IS_BINNING_DISABLE)
        {
            rate = 1;
        }
        else if(currRate == IS_BINNING_2X_VERTICAL)
        {
            rate = 2;
        }
        else if(currRate == IS_BINNING_4X_VERTICAL)
        {
            rate = 4;
        }
        else if(currRate == IS_BINNING_8X_VERTICAL)
        {
            rate = 8;
        }
        else if(currRate == IS_BINNING_16X_VERTICAL)
        {
            rate = 16;
        }
        else
        {
            fprintf(stderr, "%s: currently has an unsupported binning rate %dX,resetting to 1X\n",__func__,currRate);

            is_err = is_SetBinning(config->camera_handle, IS_BINNING_DISABLE);
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Failed to set binning rate to 1X,error code is %d\n",__func__,is_err);
                return is_err;
            }
        }

        return IS_SUCCESS;
    }

    fprintf(stdout, "%s: Updated binning rate to %dX\n",__func__,rate);

    config->binning = rate;

    return is_err;
}

static int setSensorScaling(struct Ui3240Config *config,double rate)
{
    int is_err = IS_SUCCESS;
    SENSORSCALERINFO sensorScalerInfo;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Stop capture to prevent access to memory buffer
    setStandbyMode(*config);

    is_err = is_GetSensorScalerInfo(config->camera_handle, &sensorScalerInfo, sizeof(sensorScalerInfo));
    if(is_err == IS_NOT_SUPPORTED)
    {
        rate = 1.0;
        config->sensor_scaling = 1.0;

        fprintf(stderr, "%s: does not support internal image scaling,error code is %d\n",__func__,is_err);

        return IS_SUCCESS;
    }
    else if(is_err != IS_SUCCESS)
    {
        rate = 1.0;
        config->sensor_scaling = 1.0;

        fprintf(stderr, "%s: Failed to obtain supported internal image scaling information,error code is %d\n",__func__,is_err);

        return is_err;
    }
    else
    {
        if(rate < sensorScalerInfo.dblMinFactor || rate > sensorScalerInfo.dblMaxFactor)
        {
            rate = sensorScalerInfo.dblCurrFactor;

            fprintf(stderr, "%s: Requested internal image scaling rate of %f is not within supported range %f to %f\n",
                    __func__,rate,sensorScalerInfo.dblMinFactor,sensorScalerInfo.dblMaxFactor);

            return IS_SUCCESS;
        }
    }

    is_err = is_SetSensorScaler(config->camera_handle, IS_ENABLE_SENSOR_SCALER, rate);
    if(is_err != IS_SUCCESS)
    {
        rate = 1.0;

        fprintf(stderr, "%s: Failed to set internal image scaling rate to %f,resetting to 1X\n",__func__,rate);

        is_err = is_SetSensorScaler(config->camera_handle, IS_ENABLE_SENSOR_SCALER, rate);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set internal image scaling rate to 1X,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }

    fprintf(stdout, "%s: Updated internal image scaling rate to %fX\n",__func__,rate);

    config->sensor_scaling = rate;

    return is_err;
}

static int setGain(struct Ui3240Config *config,bool auto_gain, int master_gain, int red_gain,int green_gain, int blue_gain, bool gain_boost)
{
    int is_err = IS_SUCCESS;
    double pval1 = 0;
    double pval2 = 0;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Validate arguments
    CAP(master_gain, 0, 100);
    CAP(red_gain, 0, 100);
    CAP(green_gain, 0, 100);
    CAP(blue_gain, 0, 100);

    if(auto_gain)
    {
        // Set auto gain
        pval1 = 1;

        is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_SENSOR_GAIN,&pval1, &pval2);
        if(is_err != IS_SUCCESS)
        {
            if ((is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_GAIN,&pval1, &pval2)) != IS_SUCCESS)
            {
                fprintf(stderr, "%s: does not support auto gain mode 1,error code is %d\n",__func__,is_err);
                auto_gain = false;
            }
        }
    }
    else
    {
        // Disable auto gain
        is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_SENSOR_GAIN,&pval1, &pval2);
        if(is_err != IS_SUCCESS)
        {
            is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_GAIN,&pval1, &pval2);
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: does not support auto gain mode 2,error code is %d\n",__func__,is_err);
            }
        }

        // Set gain boost
        is_err = is_SetGainBoost(config->camera_handle, IS_GET_SUPPORTED_GAINBOOST);
        if(is_err != IS_SET_GAINBOOST_ON)
        {
            gain_boost = false;
        }
        else
        {
            is_err = is_SetGainBoost(config->camera_handle,(gain_boost) ? IS_SET_GAINBOOST_ON : IS_SET_GAINBOOST_OFF);
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Failed to set gain boost,error code is %d\n",__func__,is_err);
            }
        }

        // Set manual gain parameters
        is_err = is_SetHardwareGain(config->camera_handle,master_gain,red_gain,green_gain,blue_gain);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set manual gains,error code is %d\n",__func__,is_err);
        }
    }

    config->master_gain = master_gain;
    config->red_gain = red_gain;
    config->green_gain = green_gain;
    config->blue_gain = blue_gain;
    config->auto_gain = auto_gain;
    config->gain_boost = gain_boost;

    if(auto_gain)
    {
        fprintf(stderr, "%s: Updated gain to auto\n",__func__);
    }
    else
    {
        fprintf(stderr, "%s: Updated gain to manual\n",__func__);
    }

    return is_err;
}

static int setExposure(struct Ui3240Config *config,bool auto_exposure, double exposure_ms)
{
    int is_err = IS_SUCCESS;
    int is_err1 = IS_SUCCESS;
    double minExposure, maxExposure;
    double pval1 = auto_exposure;
    double pval2 = 0;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_SENSOR_SHUTTER,&pval1, &pval2);
    if(is_err != IS_SUCCESS)
    {
        is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_SHUTTER,&pval1, &pval2);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Auto exposure mode is not supported,error code is %d\n",__func__,is_err);
            auto_exposure = false;
        }
    }

    // Set manual exposure timing
    if(!auto_exposure)
    {
        // Make sure that user-requested exposure rate is achievable
        is_err  = is_Exposure(config->camera_handle, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MIN,(void*)&minExposure, sizeof(minExposure));
        is_err1 = is_Exposure(config->camera_handle, IS_EXPOSURE_CMD_GET_EXPOSURE_RANGE_MAX,(void*)&maxExposure, sizeof(maxExposure));
        if(is_err != IS_SUCCESS || is_err1 != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to query valid exposure range,error code is %d\n",__func__,is_err);
            return is_err;
        }

        CAP(exposure_ms, minExposure, maxExposure);

        // Update exposure
        is_err = is_Exposure(config->camera_handle, IS_EXPOSURE_CMD_SET_EXPOSURE,(void*)&(exposure_ms), sizeof(exposure_ms));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set exposure to %f ms\n",__func__,exposure_ms);
            return is_err;
        }
    }

    config->auto_exposure = auto_exposure;
    config->exposure = exposure_ms;

    fprintf(stdout, "%s: Updated exposure success\n",__func__);

    return is_err;
}

static int setWhiteBalance(struct Ui3240Config *config,bool auto_white_balance, int red_offset,int blue_offset)
{
    int is_err = IS_SUCCESS;
    double pval1 = auto_white_balance;
    double pval2 = 0;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    CAP(red_offset, -50, 50);
    CAP(blue_offset, -50, 50);

    // TODO: 9 bug: enabling auto white balance does not seem to have an effect; in ueyedemo it seems to change R/G/B gains automatically
    is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_SENSOR_WHITEBALANCE, &pval1, &pval2);
    if(is_err != IS_SUCCESS)
    {
        is_err = is_SetAutoParameter(config->camera_handle, IS_SET_AUTO_WB_ONCE,&pval1, &pval2);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Auto white balance mode is not supported,error code is %d\n",__func__,is_err);
            auto_white_balance = false;
        }
    }
    if(auto_white_balance)
    {
        pval1 = red_offset;
        pval2 = blue_offset;

        is_err = is_SetAutoParameter(config->camera_handle, IS_SET_AUTO_WB_OFFSET,&pval1, &pval2);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set white balance red/blue offsets to %d/%d\n",__func__,red_offset,blue_offset);
        }
    }

    config->auto_white_balance = auto_white_balance;
    config->white_balance_red_offset = red_offset;
    config->white_balance_blue_offset = blue_offset;

    fprintf(stdout, "%s: Updated white balance success\n",__func__);

    return is_err;
}

static int setFrameRate(struct Ui3240Config *config,bool auto_frame_rate, double frame_rate_hz)
{
    int is_err = IS_SUCCESS;
    double pval1 = 0;
    double pval2 = 0;
    double minFrameTime;
    double maxFrameTime;
    double intervalFrameTime;
    double newFrameRate;
    bool autoShutterOn = false;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    // Make sure that auto shutter is enabled before enabling auto frame rate
    is_SetAutoParameter(config->camera_handle, IS_GET_ENABLE_AUTO_SENSOR_SHUTTER, &pval1, &pval2);
    autoShutterOn |= (pval1 != 0);

    is_SetAutoParameter(config->camera_handle, IS_GET_ENABLE_AUTO_SHUTTER, &pval1, &pval2);
    autoShutterOn |= (pval1 != 0);

    if(!autoShutterOn)
    {
        auto_frame_rate = false;
    }

    // Set frame rate / auto
    pval1 = auto_frame_rate;

    is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_SENSOR_FRAMERATE,&pval1, &pval2);
    if(is_err != IS_SUCCESS)
    {
        is_err = is_SetAutoParameter(config->camera_handle, IS_SET_ENABLE_AUTO_FRAMERATE,&pval1, &pval2);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Auto frame rate mode is not supported,error code is %d\n",__func__,is_err);
            auto_frame_rate = false;
        }
    }
    if(!auto_frame_rate)
    {
        // Make sure that user-requested frame rate is achievable
        is_err = is_GetFrameTimeRange(config->camera_handle, &minFrameTime,&maxFrameTime, &intervalFrameTime);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to query valid frame rate range,error code is %d\n",__func__,is_err);
            return is_err;
        }

        CAP(frame_rate_hz, 1.0/maxFrameTime, 1.0/minFrameTime);

        // Update frame rate
        is_err = is_SetFrameRate(config->camera_handle, frame_rate_hz, &newFrameRate);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set frame rate to %lfHz\n",__func__,frame_rate_hz);
            return is_err;
        }
        else if(frame_rate_hz != newFrameRate)
        {
            frame_rate_hz = newFrameRate;
        }
    }

    config->auto_frame_rate = auto_frame_rate;
    config->frame_rate = frame_rate_hz;

    fprintf(stdout, "%s: Updated frame rate success\n",__func__);

    return is_err;
}

static int setPixelClockRate(struct Ui3240Config *config,int clock_rate_mhz)
{
    int is_err = IS_SUCCESS;
    unsigned int pixelClockList[150];  // No camera has more than 150 different pixel clocks (uEye manual)
    unsigned int numberOfSupportedPixelClocks = 0;
    int minPixelClock = 0;
    int maxPixelClock = 0;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    is_err = is_PixelClock(config->camera_handle, IS_PIXELCLOCK_CMD_GET_NUMBER,
                          (void*)&numberOfSupportedPixelClocks,sizeof(numberOfSupportedPixelClocks));
    if(is_err!= IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query number of supported pixel clocks,error code is %d\n",__func__,is_err);
        return is_err;
    }

    if(numberOfSupportedPixelClocks > 0)
    {
        ZeroMemory(pixelClockList, sizeof(pixelClockList));

        is_err = is_PixelClock(config->camera_handle, IS_PIXELCLOCK_CMD_GET_LIST,
                              (void*)pixelClockList, numberOfSupportedPixelClocks * sizeof(int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to query list of supported pixel clocks,error code is %d\n",__func__,is_err);
            return is_err;
        }
    }
    
    minPixelClock = (int)pixelClockList[0];
    maxPixelClock = (int)pixelClockList[numberOfSupportedPixelClocks - 1];

    CAP(clock_rate_mhz, minPixelClock, maxPixelClock);

    // As list is sorted smallest to largest...
    for(UINT i = 0; i < numberOfSupportedPixelClocks; i ++)
    {
        if(clock_rate_mhz <= (int) pixelClockList[i])
        {
            clock_rate_mhz = pixelClockList[i];  // ...get the closest-larger-or-equal from the list
            break;
        }
    }

    is_err = is_PixelClock(config->camera_handle, IS_PIXELCLOCK_CMD_SET,(void*)&(clock_rate_mhz), sizeof(clock_rate_mhz));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to set pixel clock to %dMHz\n",__func__,clock_rate_mhz);
        return is_err;
    }

    config->pixel_clock = clock_rate_mhz;

    fprintf(stdout, "%s: Updated pixel clock success\n",__func__);

    return IS_SUCCESS;
}

static int setFlashParams(struct Ui3240Config *config,int delay_us, unsigned int duration_us)
{
    int is_err = IS_SUCCESS;
    // Make sure parameters are within range supported by camera
    IO_FLASH_PARAMS minFlashParams;
    IO_FLASH_PARAMS maxFlashParams;
    IO_FLASH_PARAMS newFlashParams;

    is_err = is_IO(config->camera_handle, IS_IO_CMD_FLASH_GET_PARAMS_MIN,(void*)&minFlashParams, sizeof(IO_FLASH_PARAMS));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not retrieve flash parameter info (min),error code is %d\n",__func__,is_err);
        return is_err;
    }

    is_err = is_IO(config->camera_handle, IS_IO_CMD_FLASH_GET_PARAMS_MAX,(void*)&maxFlashParams, sizeof(IO_FLASH_PARAMS));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not retrieve flash parameter info (max),error code is %d\n",__func__,is_err);
        return is_err;
    }

    delay_us = (delay_us < minFlashParams.s32Delay) ? minFlashParams.s32Delay : 
               ((delay_us > maxFlashParams.s32Delay) ? maxFlashParams.s32Delay : delay_us);
    duration_us = (duration_us < minFlashParams.u32Duration && duration_us != 0) ? 
                  minFlashParams.u32Duration : ((duration_us > maxFlashParams.u32Duration) ? 
                  maxFlashParams.u32Duration : duration_us);

    newFlashParams.s32Delay = delay_us;
    newFlashParams.u32Duration = duration_us;

    // WARNING: Setting s32Duration to 0, according to documentation, means
    //          setting duration to total exposure time. If non-ext-triggered
    //          camera is operating at fastest grab rate, then the resulting
    //          flash signal will APPEAR as active LO when set to active HIGH,
    //          and vice versa. This is why the duration is set manually.
    is_err = is_IO(config->camera_handle, IS_IO_CMD_FLASH_SET_PARAMS,(void*)&newFlashParams, sizeof(IO_FLASH_PARAMS));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not set flash parameter info,error code is %d\n",__func__,is_err);
        return is_err;
    }

    config->flash_delay = delay_us;
    config->flash_duration = duration_us;

    return is_err;
}

static int setMirrorUpsideDown(struct Ui3240Config *config,bool flip_horizontal)
{
    int is_err = IS_SUCCESS;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    if(flip_horizontal)
    {
        is_err = is_SetRopEffect(config->camera_handle,IS_SET_ROP_MIRROR_UPDOWN,1,0);
    }
    else
    {
        is_err = is_SetRopEffect(config->camera_handle,IS_SET_ROP_MIRROR_UPDOWN,0,0);
    }

    config->flip_upd = flip_horizontal;

    return is_err;
}

static int setMirrorLeftRight(struct Ui3240Config *config,bool flip_vertical)
{
    int is_err = IS_SUCCESS;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    if(flip_vertical)
    {
        is_err = is_SetRopEffect(config->camera_handle,IS_SET_ROP_MIRROR_LEFTRIGHT,1,0);
    }
    else
    {
        is_err = is_SetRopEffect(config->camera_handle,IS_SET_ROP_MIRROR_LEFTRIGHT,0,0);
    }

    config->flip_lr = flip_vertical;

    return is_err;
}

static int setCameraUserConfig(struct Ui3240Config *config,struct Ui3240Config user_config)
{
    int is_err = IS_SUCCESS;

    if(config->master != user_config.master)
    {
        config->master = user_config.master;
    }

    if(config->stereo != user_config.stereo)
    {
        config->stereo = user_config.stereo;
    }

    if(config->capture_mode != user_config.capture_mode)
    {
        config->capture_mode = user_config.capture_mode;
    }

    if(config->capture_timeout != user_config.capture_timeout)
    {
        config->capture_timeout = user_config.capture_timeout;
    }

    if(config->image_width != user_config.image_width || 
       config->image_height != user_config.image_height || 
       config->image_left != user_config.image_left || 
       config->image_top != user_config.image_top)
    {
        is_err = setResolution(config,
                               user_config.image_width,user_config.image_height,
                               user_config.image_left,user_config.image_top);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Failed to set user resolution,error code is %d\n",__func__,is_err);
        }
    }

    if(config->color_mode != user_config.color_mode)
    {
        config->color_mode = user_config.color_mode;

        is_err = is_SetColorMode(user_config.camera_handle, user_config.color_mode);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user color mode,error code is %d\n",__func__,is_err);
        }

        if(user_config.color_mode == IS_CM_BGR8_PACKED || user_config.color_mode == IS_CM_RGB8_PACKED)
        {
            config->bits_per_pixel = 24;
        }
        else if(user_config.color_mode == IS_CM_MONO8 || user_config.color_mode == IS_CM_SENSOR_RAW8)
        {
            config->bits_per_pixel = 8;
        }
    }

    if(config->subsampling != user_config.subsampling)
    {
        is_err = setSubsampling(config,user_config.subsampling);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user subsampling,error code is %d\n",__func__,is_err);
        }
    }

    if(config->binning != user_config.binning)
    {
        is_err = setBinning(config,user_config.binning);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user binning,error code is %d\n",__func__,is_err);
        }
    }

    if(config->sensor_scaling != user_config.sensor_scaling)
    {
        is_err = setSensorScaling(config,user_config.sensor_scaling);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user scaling,error code is %d\n",__func__,is_err);
        }
    }

    if(config->auto_gain != user_config.auto_gain || 
       config->master_gain != user_config.master_gain || 
       config->red_gain != user_config.red_gain || 
       config->green_gain != user_config.green_gain || 
       config->blue_gain != user_config.blue_gain || 
       config->gain_boost != user_config.gain_boost)
    {
        is_err = setGain(config,
                         user_config.auto_gain,user_config.master_gain,user_config.red_gain,
                         user_config.green_gain,user_config.blue_gain,user_config.gain_boost);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user gain,error code is %d\n",__func__,is_err);
        }
    }

    if(config->auto_exposure != user_config.auto_exposure ||  
       config->exposure != user_config.exposure)
    {
        is_err = setExposure(config,user_config.auto_exposure,user_config.exposure);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user exposure,error code is %d\n",__func__,is_err);
        }
    }

    if(config->auto_white_balance != user_config.auto_white_balance || 
       config->white_balance_red_offset != user_config.white_balance_red_offset ||  
       config->white_balance_blue_offset != user_config.white_balance_blue_offset)
    {
        is_err = setWhiteBalance(config,
                                 user_config.auto_white_balance,
                                 user_config.white_balance_red_offset,
                                 user_config.white_balance_blue_offset);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user white_balance,error code is %d\n",__func__,is_err);
        }
    }

    if(config->auto_frame_rate != user_config.auto_frame_rate ||  
       config->frame_rate != user_config.frame_rate)
    {
        is_err = setFrameRate(config,user_config.auto_frame_rate,user_config.frame_rate);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user frame_rate,error code is %d\n",__func__,is_err);
        }
    }

    if(config->pixel_clock != user_config.pixel_clock)
    {
        is_err = setPixelClockRate(config,user_config.pixel_clock);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user pixel_clock,error code is %d\n",__func__,is_err);
        }
    }

    if(config->ext_trigger_mode != user_config.ext_trigger_mode)
    {
        config->ext_trigger_mode = user_config.ext_trigger_mode;
    }

    if(config->ext_trigger_delay != user_config.ext_trigger_delay)
    {
        config->ext_trigger_delay = user_config.ext_trigger_delay;
    }

    if(config->flash_delay != user_config.flash_delay ||  
       config->flash_duration != user_config.flash_duration)
    {
        is_err = setFlashParams(config,user_config.flash_delay,user_config.flash_duration);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user flash params,error code is %d\n",__func__,is_err);
        }
    }

    if(config->flip_upd != user_config.flip_upd)
    {
        is_err = setMirrorUpsideDown(config,user_config.flip_upd);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user image up down,error code is %d\n",__func__,is_err);
        }
    }

    if(config->flip_lr != user_config.flip_lr)
    {
        is_err = setMirrorLeftRight(config,user_config.flip_lr);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set user image left right,error code is %d\n",__func__,is_err);
        }
    }

    return IS_SUCCESS;
}

static int setFreeRunMode(struct Ui3240Config *config)
{
    int is_err = IS_SUCCESS;
    int flash_delay = 0;
    unsigned int flash_duration = 1000;
    unsigned int nMode = IO_FLASH_MODE_FREERUN_HI_ACTIVE;

    if(!isConnected(*config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

//    if(!freeRunModeActive(config))
//    {
        setStandbyMode(*config); // No need to check for success

        // Set the flash to a high active pulse for each image in the trigger mode
        setFlashParams(config,flash_delay, flash_duration);

        is_err = is_IO(config->camera_handle, IS_IO_CMD_FLASH_SET_MODE,(void*)&nMode, sizeof(nMode));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set free-run active-low flash output,error code is %d\n",__func__,is_err);
            return is_err;
        }
/*
        is_err = is_Event(config->camera_handle, IS_EVENT_CMD_ENABLE, IS_SET_EVENT_FRAME, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not enable frame event,error code is %d\n",__func__,is_err);
            return is_err;
        }
*/
        is_err = is_CaptureVideo(config->camera_handle, IS_WAIT);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not start free-run live video mode,error code is %d\n",__func__,is_err);
            return is_err;
        }

        fprintf(stdout, "%s: Started live video mode\n",__func__);
//    }

    return is_err;
}

static int setExtTriggerMode(struct Ui3240Config config,double frame_rate, int trigger_delay, bool master)
{
    int is_err = IS_SUCCESS;
    int min_delay;
    int max_delay;
    int current_delay;
    unsigned int nMode = IO_FLASH_MODE_TRIGGER_HI_ACTIVE;

    if(!isConnected(config))
    {
        return IS_INVALID_CAMERA_HANDLE;
    }

    if(!extTriggerModeActive(config))
    {
        setStandbyMode(config); // No need to check for success
/*
        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_ENABLE, IS_SET_EVENT_FRAME, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not enable frame event,error code is %d\n",__func__,is_err);
            return is_err;
        }
*/
        /* If "master" set the GPIO2 to generate PWM */
        if(master)
        {
            fprintf(stderr, "%s: GPIO2 configured as output (PWM) at %lfHz\n",__func__,frame_rate);

            is_err = gpioPwmConfig(config.camera_handle, frame_rate, true);
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Could not set GPIO 2 as output (PWM),error code is %d\n",__func__,is_err);
                return is_err;
            }
        }

        /* set global shutter mode */
        /*
        int nShutterMode = IS_DEVICE_FEATURE_CAP_SHUTTER_MODE_GLOBAL;
        is_err = is_DeviceFeature(config.camera_handle, IS_DEVICE_FEATURE_CMD_SET_SHUTTER_MODE,
                                 (void *)&nShutterMode, sizeof(nShutterMode));
        if(is_err == IS_SUCCESS)
        {
            fprintf(stderr, "%s: Global shutter ok,error code is %d\n",__func__,is_err);
        }
        */

        /* Set GPIO1 as input for trigger */
        is_err = gpioInputConfig(config.camera_handle);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set GPIO1 as input,error code is %d\n",__func__,is_err);
            return is_err;
        }

        fprintf(stdout, "%s: GPIO1 configured as input for triggering\n",__func__);
        fprintf(stdout, "%s: GPIO2 configured as output for flash\n",__func__);

        /* Set to trigger on falling edge */
        is_err = is_SetExternalTrigger(config.camera_handle, IS_SET_TRIGGER_HI_LO);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not enable falling-edge external trigger mode,error code is %d\n",__func__,is_err);
            return is_err;
        }

        /* Set trigger delay */
        min_delay = is_SetTriggerDelay(config.camera_handle, IS_GET_MIN_TRIGGER_DELAY);
        max_delay = is_SetTriggerDelay(config.camera_handle, IS_GET_MAX_TRIGGER_DELAY);

        is_err = is_SetTriggerDelay(config.camera_handle, (int)trigger_delay);
        if(is_err != IS_SUCCESS && (trigger_delay >= min_delay && trigger_delay <= max_delay))
        {
            fprintf(stderr, "%s: Min delay: %dus; Max delay: %dus\n",__func__,min_delay,max_delay);
            fprintf(stderr, "%s: Could not set trigger-delay,error code is %d\n",__func__,is_err);
            return is_err;
        }

        current_delay = is_SetTriggerDelay(config.camera_handle, IS_GET_TRIGGER_DELAY);
        fprintf(stdout, "%s: external trigger delay: %dus\n",__func__,current_delay);

        // high level in trigger mode
        is_err = is_IO(config.camera_handle, IS_IO_CMD_FLASH_SET_MODE, (void *)&nMode, sizeof(nMode));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: set trigger flash mode failed,error code is %d\n",__func__,is_err);
        } 

        // // get the flash Mode to confirm
        is_err = is_IO(config.camera_handle, IS_IO_CMD_FLASH_GET_MODE, (void *)&nMode, sizeof(nMode));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: set flash mode failed,error code is %d\n",__func__,is_err);
        }

        // start video capture
        is_err = is_CaptureVideo(config.camera_handle, IS_DONT_WAIT);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not start external trigger live video mode,error code is %d\n",__func__,is_err);
            return is_err;
        }

        fprintf(stdout, "%s: Started falling-edge external trigger live video mode\n",__func__);
    }

    return is_err;
}

static int setTriggerMode(struct Ui3240Config *config)
{
    int is_err = IS_SUCCESS;
    int is_err1 = IS_SUCCESS;

    if(config->ext_trigger_mode) 
    {
        fprintf(stderr, "%s: Setup external trigger mode...\n",__func__);

        is_err = setExtTriggerMode(*config,config->frame_rate, config->ext_trigger_delay, config->master);
        if(is_err != IS_SUCCESS) 
        {
            fprintf(stderr, "%s: Setup external trigger mode failed,error code is %d\n",__func__,is_err);
            return is_err;
        }

        fprintf(stdout, "%s: Setup external trigger mode success\n",__func__);
    } 
    else 
    {
        fprintf(stdout, "%s: Setup freerun mode...\n",__func__);
        // NOTE: need to copy flash parameters to local copies since
        //       config.flash_duration is type int, and also sizeof(int)
        //       may not equal to sizeof(INT) / sizeof(UINT)
        
        is_err  = setFreeRunMode(config);
        is_err1 = setFlashParams(config,config->flash_delay, config->flash_duration);
        if(is_err != IS_SUCCESS || is_err1 != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Setup freerun mode failed,error code is %d,%d\n",__func__,is_err,is_err1);
            return is_err;
        }
        
        fprintf(stdout, "%s: Setup freerun mode success\n",__func__);
    }

    return is_err;
}

static int queryCameraConfig(struct Ui3240Config *config)
{
    int is_err = IS_SUCCESS;
    int is_err1 = IS_SUCCESS;
    IS_RECT camera_aio;
    SENSORSCALERINFO sensor_scaler_info;
    int curr_subsampling_rate;
    int curr_binning_rate;
    int query;
    double pval1;
    double pval2;
    IO_FLASH_PARAMS currFlashParams;
    unsigned int currPixelClock;
    int currROP;

    // Synchronize resolution, color mode, bits per pixel settings
    is_err = is_AOI(config->camera_handle, IS_AOI_IMAGE_GET_AOI,(void*)&camera_aio, sizeof(camera_aio));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not retrieve Area Of Interest (AOI) information,error code is %d\n",__func__,is_err);
        return is_err;
    }
    config->image_width = camera_aio.s32Width;
    config->image_height = camera_aio.s32Height;
    config->image_left = camera_aio.s32X;
    config->image_top = camera_aio.s32Y;

    config->color_mode = is_SetColorMode(config->camera_handle, IS_GET_COLOR_MODE);
    if(config->color_mode == IS_CM_BGR8_PACKED || config->color_mode == IS_CM_RGB8_PACKED)
    {
        config->bits_per_pixel = 24;
    }
    else if(config->color_mode == IS_CM_MONO8 || config->color_mode == IS_CM_SENSOR_RAW8)
    {
        config->bits_per_pixel = 8;
    }
    else
    {
        fprintf(stderr, "%s: Current color mode is not supported by this wrapper{mono8 | bayer_rggb8 | rgb8 | bgr8}\n",__func__);

        is_err = setColorMode(*config,UI3240_DEFAULT_CLOLOR_MODE_STRING); 
        if(is_err != IS_SUCCESS)
        {
            return is_err;
        }
   
        config->color_mode = is_SetColorMode(config->camera_handle, IS_GET_COLOR_MODE);
        config->bits_per_pixel = 8;
    }

    // Synchronize sensor scaling rate setting
    is_err = is_GetSensorScalerInfo(config->camera_handle, &sensor_scaler_info, sizeof(sensor_scaler_info));
    if(is_err == IS_NOT_SUPPORTED)
    {
        config->sensor_scaling = 1.0;
    }
    else if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not obtain supported internal image scaling information,error code is %d\n",__func__,is_err);
        return is_err;
    }
    else
    {
        config->sensor_scaling = sensor_scaler_info.dblCurrFactor;
    }

    // Synchronize subsampling rate setting
    curr_subsampling_rate = is_SetSubSampling(config->camera_handle, IS_GET_SUBSAMPLING);
    if(curr_subsampling_rate == IS_SUBSAMPLING_DISABLE)
    {
        config->subsampling = 1;
    }
    else if(curr_subsampling_rate == IS_SUBSAMPLING_2X_VERTICAL)
    {
        config->subsampling = 2;
    }
    else if(curr_subsampling_rate == IS_SUBSAMPLING_4X_VERTICAL)
    {
        config->subsampling = 4;
    }
    else if(curr_subsampling_rate == IS_SUBSAMPLING_8X_VERTICAL)
    {
        config->subsampling = 8;
    }
    else if(curr_subsampling_rate == IS_SUBSAMPLING_16X_VERTICAL)
    {
        config->subsampling = 16;
    }
    else
    {
        fprintf(stderr, "%s: Current sampling rate is not supported by this wrapper; resetting to 1X\n",__func__);

        is_err = is_SetSubSampling(config->camera_handle, IS_SUBSAMPLING_DISABLE);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set subsampling rate to 1X,error code is %d\n",__func__,is_err);
            return is_err;
        }

        config->subsampling = 1;
    }

    // Synchronize binning rate setting
    curr_binning_rate = is_SetBinning(config->camera_handle, IS_GET_BINNING);
    if(curr_binning_rate == IS_BINNING_DISABLE)
    {
        config->binning = 1;
    }
    else if(curr_binning_rate == IS_BINNING_2X_VERTICAL)
    {
        config->binning = 2;
    }
    else if(curr_binning_rate == IS_BINNING_4X_VERTICAL)
    {
        config->binning = 4;
    }
    else if(curr_binning_rate == IS_BINNING_8X_VERTICAL)
    {
        config->binning = 8;
    }
    else if(curr_binning_rate == IS_BINNING_16X_VERTICAL)
    {
        config->binning = 16;
    }
    else
    {
        fprintf(stderr, "%s: Current binning rate is not supported by this wrapper; resetting to 1X\n",__func__);

        is_err = is_SetBinning(config->camera_handle, IS_BINNING_DISABLE);

        if (is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: Could not set binning rate to 1X,error code is %d\n",__func__,is_err);
            
        }

        config->binning = 1;
    }   

    is_err  = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_SENSOR_GAIN, &pval1, &pval2);
    is_err1 = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_GAIN, &pval1, &pval2);
    if(is_err != IS_SUCCESS && is_err1 != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query auto gain mode,error code is %d\n",__func__,is_err);
        return is_err;
    }
    
    config->auto_gain = (pval1 != 0);

    config->master_gain    = is_SetHardwareGain(config->camera_handle, IS_GET_MASTER_GAIN,IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
    config->red_gain       = is_SetHardwareGain(config->camera_handle, IS_GET_RED_GAIN,IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
    config->green_gain     = is_SetHardwareGain(config->camera_handle, IS_GET_GREEN_GAIN,IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);
    config->blue_gain      = is_SetHardwareGain(config->camera_handle, IS_GET_BLUE_GAIN,IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER, IS_IGNORE_PARAMETER);

    query = is_SetGainBoost(config->camera_handle, IS_GET_SUPPORTED_GAINBOOST);
    if(query == IS_SET_GAINBOOST_ON)
    {
        query = is_SetGainBoost(config->camera_handle, IS_GET_GAINBOOST);
        if(query == IS_SET_GAINBOOST_ON)
        {
            config->gain_boost = true;
        }
        else if(query == IS_SET_GAINBOOST_OFF)
        {
            config->gain_boost = false;
        }
        else
        {
            fprintf(stderr, "%s: Failed to query gain boost,error code is %d\n",__func__,is_err);
            return query;
        }
    }
    else
    {
        config->gain_boost = false;
    }

    is_err  = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_SENSOR_SHUTTER, &pval1, &pval2);
    is_err1 = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_SHUTTER, &pval1, &pval2);
    if(is_err != IS_SUCCESS && is_err1 != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query auto shutter mode,error code is %d\n",__func__,is_err);
        return is_err;
    }

    config->auto_exposure = (pval1 != 0);

    is_err = is_Exposure(config->camera_handle, IS_EXPOSURE_CMD_GET_EXPOSURE,&config->exposure,sizeof(config->exposure));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query exposure timing,error code is %d\n",__func__,is_err);
        return is_err;
    }

    is_err = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_SENSOR_WHITEBALANCE, &pval1, &pval2);
    is_err = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_WHITEBALANCE, &pval1, &pval2);
    if(is_err != IS_SUCCESS && is_err1 != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query auto white balance mode,error code is %d,%d\n",__func__,is_err,is_err1);
        return is_err;
    }

    config->auto_white_balance = (pval1 != 0);

    is_err = is_SetAutoParameter(config->camera_handle,IS_GET_AUTO_WB_OFFSET, &pval1, &pval2);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query auto white balance red/blue channel offsets,error code is %d\n",__func__,is_err);
        return is_err;
    }

    config->white_balance_red_offset = pval1;
    config->white_balance_blue_offset = pval2;

    is_err = is_IO(config->camera_handle, IS_IO_CMD_FLASH_GET_PARAMS,(void*)&currFlashParams, sizeof(IO_FLASH_PARAMS));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Could not retrieve current flash parameter info,error code is %d\n",__func__,is_err);
        return is_err;
    }

    config->flash_delay = currFlashParams.s32Delay;
    config->flash_duration = currFlashParams.u32Duration;

    is_err  = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_SENSOR_FRAMERATE, &pval1, &pval2);
    is_err1 = is_SetAutoParameter(config->camera_handle,IS_GET_ENABLE_AUTO_FRAMERATE, &pval1, &pval2);
    if(is_err != IS_SUCCESS && is_err1 != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query auto frame rate mode,error code is %d\n",__func__,is_err);
        return is_err;
    }

    config->auto_frame_rate = (pval1 != 0);

    is_err = is_SetFrameRate(config->camera_handle, IS_GET_FRAMERATE, &config->frame_rate);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query frame rate,error code is %d\n",__func__,is_err);
        return is_err;
    }

    is_err = is_PixelClock(config->camera_handle, IS_PIXELCLOCK_CMD_GET,(void*)&currPixelClock, sizeof(currPixelClock));
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to query pixel clock rate,error code is %d\n",__func__,is_err);
        return is_err;
    }

    config->pixel_clock = currPixelClock;

    currROP = is_SetRopEffect(config->camera_handle, IS_GET_ROP_EFFECT, 0, 0);
    config->flip_upd = ((currROP & IS_SET_ROP_MIRROR_UPDOWN) == IS_SET_ROP_MIRROR_UPDOWN);
    config->flip_lr = ((currROP & IS_SET_ROP_MIRROR_LEFTRIGHT) == IS_SET_ROP_MIRROR_LEFTRIGHT);

    // NOTE: do not need to (re-)populate ROS image message, since assume that
    //       syncCamConfig() was previously called

    fprintf(stdout, "%s: Successfully queries parameters from UEye camera\n",__func__);

    return is_err;
}

/*
static int getTimestamp(struct Ui3240Config config,UEYETIME *timestamp,int cam_buffer_id)
{
    int is_err = IS_SUCCESS;
    UEYEIMAGEINFO ImageInfo;

    is_err = is_GetImageInfo (config.camera_handle,cam_buffer_id, &ImageInfo, sizeof(ImageInfo));
    if(is_err == IS_SUCCESS)
    {
        *timestamp = ImageInfo.TimestampSystem;

        return is_err;
    }

    return IS_NO_SUCCESS;
}
*/

static int captureImage(struct Ui3240Config config,struct ImageBuffer *image_buf,unsigned char capture_mode, unsigned short timeout_ms)
{
    int is_err = IS_SUCCESS;
    static unsigned char mode = 255;
    char *memory = NULL;
    int id;
//    UEYETIME time_stamp;
    IMAGEQUEUEWAITBUFFER waitBuffer;
    waitBuffer.timeout = timeout_ms;
    waitBuffer.pnMemId= &id;
    waitBuffer.ppcMem = &memory;
    unsigned int event = IS_SET_EVENT_FRAME;
    IS_INIT_EVENT init_event = {IS_SET_EVENT_FRAME, TRUE, FALSE};
    IS_WAIT_EVENT wait_event = {IS_SET_EVENT_FRAME, 1000, 0, 0};

    if(!isCapturing(config))
    {
        fprintf(stderr, "%s: !isCapturing(config)\n",__func__);
        return IS_NO_SUCCESS;
    }

    if(!freeRunModeActive(config) && !extTriggerModeActive(config))
    {
        fprintf(stderr, "%s: !freeRunModeActive(config) && !extTriggerModeActive(config)\n",__func__);
        return IS_NO_SUCCESS;
    }

    if(mode != capture_mode)
    {
        mode = capture_mode;

        if(capture_mode == 0)
        {
            is_err = is_Event(config.camera_handle, IS_EVENT_CMD_INIT, &init_event, sizeof(IS_INIT_EVENT));
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Could not init frame event 3,error code is %d\n",__func__,is_err);
            }

            is_err = is_Event(config.camera_handle, IS_EVENT_CMD_DISABLE, &event, sizeof(unsigned int));
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Could not disable frame event 3,error code is %d\n",__func__,is_err);
            }

            is_err = is_Event(config.camera_handle, IS_EVENT_CMD_EXIT, &event, sizeof(unsigned int));
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Could not exit frame event,error code is %d\n",__func__,is_err);
            }
        }
        else
        {
            wait_event.nTimeoutMilliseconds = timeout_ms;

            is_err = is_Event(config.camera_handle, IS_EVENT_CMD_INIT, &init_event, sizeof(IS_INIT_EVENT));
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Could not init frame event 2,error code is %d\n",__func__,is_err);
            }

            is_err = is_Event(config.camera_handle, IS_EVENT_CMD_ENABLE, &event, sizeof(unsigned int));
            if(is_err != IS_SUCCESS)
            {
                fprintf(stderr, "%s: Could not enable frame event,error code is %d\n",__func__,is_err);
            }
        }
    }

    if(capture_mode == 0)
    {
        is_err = is_ImageQueue(config.camera_handle, IS_IMAGE_QUEUE_CMD_WAIT, (void*)&waitBuffer, sizeof(waitBuffer));
        if(is_err != IS_SUCCESS)
        {
            if(is_err == IS_TIMED_OUT)
            {
                fprintf(stderr, "%s: capture image from queue timeout\n",__func__);
            }
            else
            {
                fprintf(stderr, "%s: Failed to capture from queue image,error code is %d\n",__func__,is_err);
            }

            return IS_NO_SUCCESS;
        }

        pthread_mutex_lock(&mutexImageHeap[config.camera_index]); 
        is_err = is_CopyImageMem(config.camera_handle,memory,id,image_buf->image);
        if(is_err != IS_SUCCESS)
        {   
            pthread_mutex_unlock(&mutexImageHeap[config.camera_index]);
            fprintf(stderr, "%s: copy image buffer failed,error code is %d\n",__func__,is_err);
            return IS_NO_SUCCESS;
        }

        image_buf->size = config.frame_buf_size;
        pthread_mutex_unlock(&mutexImageHeap[config.camera_index]);
/*
        is_err = getTimestamp(config,&time_stamp,id);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: query image time stamp failed,error code is %d\n",__func__,is_err);
            return IS_NO_SUCCESS;
        }

        printf("============================= time stamp start==============================\n");
        printf("| year        : %d\n",time_stamp.wYear);
        printf("| month       : %d\n",time_stamp.wMonth);
        printf("| day         : %d\n",time_stamp.wDay);
        printf("| hour        : %d\n",time_stamp.wHour);
        printf("| minute      : %d\n",time_stamp.wMinute);
        printf("| second      : %d\n",time_stamp.wSecond);
        printf("| willi second: %d\n",time_stamp.wMilliseconds);
        printf("============================= time stamp end ===============================\n");
*/
    }
    else
    {
        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_WAIT, &wait_event, sizeof(wait_event));
        if(is_err != IS_SUCCESS)
        {
            if(is_err == IS_TIMED_OUT)
            {
                fprintf(stderr, "%s: capture image from event timeout\n",__func__);
            }
            else
            {
                fprintf(stderr, "%s: Failed to capture from event image,error code is %d\n",__func__,is_err);
            }

            return IS_NO_SUCCESS;
        }

        is_err = is_Event(config.camera_handle, IS_EVENT_CMD_RESET, &event, sizeof(unsigned int));
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: reset event image failed,error code is %d\n",__func__,is_err);
            return IS_NO_SUCCESS;
        }

        is_err = is_GetImageMem(config.camera_handle, (void **)&memory);
        if(is_err != IS_SUCCESS)
        {
            fprintf(stderr, "%s: get image menory failed,error code is %d\n",__func__,is_err);
            return IS_NO_SUCCESS;
        }

        pthread_mutex_lock(&mutexImageHeap[config.camera_index]); 
        memcpy(image_buf->image, memory, config.frame_buf_size);

        image_buf->size = config.frame_buf_size;
        pthread_mutex_unlock(&mutexImageHeap[config.camera_index]);
    }

    // Unlock the buffer which has been automatically locked by is_WaitForNextImage()
    is_err = is_UnlockSeqBuf(config.camera_handle, IS_IGNORE_PARAMETER, memory);
    if(is_err != IS_SUCCESS)
    {
        fprintf(stderr, "%s: Failed to unlock image buffer,error code is %d\n",__func__,is_err);
    }

    return is_err;
}

static void initCameraConfig(struct Ui3240Config *config,struct CmdArgs *args)
{
    if(config == NULL || args == NULL)
    {
        fprintf(stderr, "%s: input paras is NULL\n",__func__);
        return;
    }

    memset(config,0,sizeof(struct Ui3240Config));

    config->camera_index                = args->camera_index;
    config->capture_mode                = args->capture_mode;
    config->capture_timeout             = args->capture_timeout;
    config->image_width                 = 1280;
    config->image_height                = 512;
    config->image_left                  = 0;
    config->image_top                   = 256;
    config->frame_num                   = args->frame_num;
    config->image_heap_depth            = args->image_heap_depth;
    config->color_mode                  = IS_CM_MONO8;
    config->bits_per_pixel              = 8;
    config->subsampling                 = 1;
    config->binning                     = 1;
    config->sensor_scaling              = 1;
    config->auto_gain                   = false;
    config->master_gain                 = 0;
    config->red_gain                    = 0;
    config->green_gain                  = 0;
    config->blue_gain                   = 0;
    config->gain_boost                  = false;
    config->auto_exposure               = false;
    config->exposure                    = 33;
    config->auto_white_balance          = false;
    config->white_balance_red_offset    = 0;
    config->white_balance_blue_offset   = 0;
    config->auto_frame_rate             = false;
    config->frame_rate                  = 30;
    config->pixel_clock                 = 42;
    config->ext_trigger_mode            = false;
    config->ext_trigger_delay           = 0;
    config->flash_delay                 = 1000;
    config->flash_duration              = 1;
    config->flip_upd                    = false;
    config->flip_lr                     = false;
}

static void printCameraConfig(struct Ui3240Config config)
{
    printf("|==================== Ui3240Config begin ====================\n");
    printf("| camera_index              = %d\n",config.camera_index);
    printf("| camera_handle             = %d\n",config.camera_handle);
    printf("| camera_id                 = %d\n",config.camera_id);
    printf("| master                    = %d\n",config.master);
    printf("| stereo                    = %d\n",config.stereo);
    printf("| capture_mode              = %d\n",config.capture_mode);
    printf("| capture_timeout           = %d\n",config.capture_timeout);
    printf("| image_width               = %d\n",config.image_width);
    printf("| image_height              = %d\n",config.image_height);
    printf("| image_left                = %d\n",config.image_left);
    printf("| image_top                 = %d\n",config.image_top);
    printf("| frame_num                 = %d\n",config.frame_num);
    printf("| image_heap_depth          = %d\n",config.image_heap_depth);
    printf("| frame_buf_size            = %d\n",config.frame_buf_size);
    printf("| color_mode                = %d\n",config.color_mode);
    printf("| bits_per_pixel            = %d\n",config.bits_per_pixel);
    printf("| subsampling               = %d\n",config.subsampling);
    printf("| binning                   = %d\n",config.binning);
    printf("| sensor_scaling            = %lf\n",config.sensor_scaling);
    printf("| auto_gain                 = %d\n",config.auto_gain);
    printf("| master_gain               = %d\n",config.master_gain);
    printf("| red_gain                  = %d\n",config.red_gain);
    printf("| green_gain                = %d\n",config.green_gain);
    printf("| blue_gain                 = %d\n",config.blue_gain);
    printf("| gain_boost                = %d\n",config.gain_boost);
    printf("| auto_exposure             = %d\n",config.auto_exposure);
    printf("| exposure                  = %lf\n",config.exposure);
    printf("| auto_white_balance        = %d\n",config.auto_white_balance);
    printf("| white_balance_red_offset  = %d\n",config.white_balance_red_offset);
    printf("| white_balance_blue_offset = %d\n",config.white_balance_blue_offset);
    printf("| auto_frame_rate           = %d\n",config.auto_frame_rate);
    printf("| frame_rate                = %lf\n",config.frame_rate);
    printf("| pixel_clock               = %d\n",config.pixel_clock);
    printf("| ext_trigger_mode          = %d\n",config.ext_trigger_mode);
    printf("| ext_trigger_delay         = %d\n",config.ext_trigger_delay);
    printf("| flash_delay               = %d\n",config.flash_delay);
    printf("| flash_duration            = %d\n",config.flash_duration);
    printf("| flip_upd                  = %d\n",config.flip_upd);
    printf("| flip_lr                   = %d\n",config.flip_lr);
    printf("|===================== Ui3240Config end =====================\n");
}

static void sendFrameRateMsgToThreadSync(struct Ui3240Config config)
{
    int ret = 0;
    double *frame_rate = NULL;

    if(config.auto_frame_rate == true)
    {
        return;
    }

    frame_rate = (double *)malloc(sizeof(double));
    if(frame_rate != NULL)
    {
        *frame_rate = config.frame_rate;

        ret = xQueueSend((key_t)KEY_FRAME_RATE_MSG,frame_rate,MAX_QUEUE_MSG_NUM);
        if(ret == -1)
        {
            fprintf(stderr, "%s: send ui3240 frame rate queue msg failed\n",__func__);
        }
    }
}

static int recvResetMsg(unsigned char index)
{
    int ret = 0;
    unsigned char *reset = NULL;

    ret = xQueueReceive((key_t)KEY_CAMERA_RESET_MSG + index,(void **)&reset,0);
    if(ret == -1)
    {
        return -1;
    }

    free(reset);
    reset = NULL;

    return ret;
}

void *thread_ui3240(void *arg)
{
    int ret = IS_SUCCESS;
    static unsigned char capture_failed_cnt = 0;
    struct CmdArgs *args = (struct CmdArgs *)arg;
    enum CameraState camera_state = INIT_CONFIG;
    struct Ui3240Config ui3240_config;
    struct Ui3240Config user_ui3240_config;

    while(1)
    {
        switch((unsigned char)camera_state)
        {
            case (unsigned char)INIT_CONFIG:            //初始化配置
                initCameraConfig(&ui3240_config,args);
                camera_state = CONNECT_CAMERA;
            break;

            case (unsigned char)CONNECT_CAMERA:         //链接相机
                ret = connectCamrea(ui3240_config.camera_id,&ui3240_config);
                if(ret != IS_SUCCESS)
                {
                    fprintf(stderr,"%s: connect camera failed\n",__func__);
                    camera_state = DISCONNECT_CAMERA;
                }
                else
                {
                    camera_state = LOAD_DEFAULT_CONFIG;
                }
            break;

            case (unsigned char)LOAD_DEFAULT_CONFIG:    //加载默认配置
                ret = loadCameraDefaultConfig(ui3240_config,L"./config/ids_default_config.ini", false);
                if(ret != IS_SUCCESS)
                {
                    fprintf(stderr,"%s: load camera default config failed\n",__func__);
                    camera_state = DISCONNECT_CAMERA;
                }
                else
                {
                    camera_state = QUERY_CAMERA_CONFIG;
                }
            break;

            case (unsigned char)QUERY_CAMERA_CONFIG:    //获取相机配置
                ret = queryCameraConfig(&ui3240_config);
                if(ret != IS_SUCCESS)
                {
                    fprintf(stderr,"%s: query camera config failed\n",__func__);
                }
                camera_state = LOAD_USER_CONFIG;
            break;

            case (unsigned char)LOAD_USER_CONFIG:       //加载用户配置
                ret = loadCameraUserConfig(ui3240_config,&user_ui3240_config,args->usb_cam_user_conf_file);
                if(ret != IS_SUCCESS)
                {
                    fprintf(stderr,"%s: load camera user config failed\n",__func__);
                    camera_state = ALLOC_FRAME_BUFFER;
                }
                else
                {
                    camera_state = SET_USER_CONFIG;
                }
            break;

            case (unsigned char)SET_USER_CONFIG:        //设置用户配置
                setCameraUserConfig(&ui3240_config,user_ui3240_config);
                printCameraConfig(ui3240_config);
                camera_state = ALLOC_FRAME_BUFFER;
            break;

            case (unsigned char)ALLOC_FRAME_BUFFER:     //申请帧缓存
                ret = reallocateCameraBuffer(&ui3240_config);
                if(ret != IS_SUCCESS)
                {
                    fprintf(stderr,"%s: alloc camera buffer failed\n",__func__);
                    camera_state = DISCONNECT_CAMERA;
                }
                else
                {
                    sendFrameRateMsgToThreadSync(ui3240_config);

                    camera_state = SET_TRIGGER_MODE;
                }
            break;

            case (unsigned char)SET_TRIGGER_MODE:       //设置触发模式 外部触发 自由模式
                ret = setTriggerMode(&ui3240_config);
                if(ret != IS_SUCCESS)
                {
                    fprintf(stderr,"%s: set trigger mode failed\n",__func__);
                    camera_state = DISCONNECT_CAMERA;
                }
                else
                {
                    camera_state = CAPTURE_IMAGE;
                }   
            break;

            case (unsigned char)CAPTURE_IMAGE:          //采集图像
                ret = captureImage(ui3240_config,
                                   imageHeap[ui3240_config.camera_index].heap[imageHeap[ui3240_config.camera_index].put_ptr]->image,
                                   ui3240_config.capture_mode,
                                   ui3240_config.capture_timeout);
                if(ret == IS_SUCCESS)
                {
/* 
                     fprintf(stdout,"%s: capture iamge success,image_counter = %d, put_ptr = %d\n",
                            __func__,
                            imageHeap[ui3240_config.camera_index].heap[imageHeap[ui3240_config.camera_index].put_ptr]->image->counter,
                            imageHeap[ui3240_config.camera_index].put_ptr);
*/
                    capture_failed_cnt = 0;
                    pthread_cond_signal(&condImageHeap[ui3240_config.camera_index]);
                }
                else
                {
                    capture_failed_cnt ++;

                    if(capture_failed_cnt >= UI3240_MAX_CAPTURE_FAILED_CNT)
                    {
                        camera_state = DISCONNECT_CAMERA;
                    }
                }
            break;

            case (unsigned char)DISCONNECT_CAMERA:      //断开相机链接
                disconnectCamera(&ui3240_config);
                usleep(1000 * 1000);
                camera_state = INIT_CONFIG;
            break;

            default:
                camera_state = DISCONNECT_CAMERA;
            break;
        }

        ret = recvResetMsg(ui3240_config.camera_index);
        if(ret != -1)
        {
            camera_state = DISCONNECT_CAMERA;
        }
    }
}
