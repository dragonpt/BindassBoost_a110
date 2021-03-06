/* Copyright Statement:
 *
 * This software/firmware and related documentation ("MediaTek Software") are
 * protected under relevant copyright laws. The information contained herein
 * is confidential and proprietary to MediaTek Inc. and/or its licensors.
 * Without the prior written permission of MediaTek inc. and/or its licensors,
 * any reproduction, modification, use or disclosure of MediaTek Software,
 * and information contained herein, in whole or in part, shall be strictly prohibited.
 */
/* MediaTek Inc. (C) 2010. All rights reserved.
 *
 * BY OPENING THIS FILE, RECEIVER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
 * THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
 * RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO RECEIVER ON
 * AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
 * NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
 * SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
 * SUPPLIED WITH THE MEDIATEK SOFTWARE, AND RECEIVER AGREES TO LOOK ONLY TO SUCH
 * THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. RECEIVER EXPRESSLY ACKNOWLEDGES
 * THAT IT IS RECEIVER'S SOLE RESPONSIBILITY TO OBTAIN FROM ANY THIRD PARTY ALL PROPER LICENSES
 * CONTAINED IN MEDIATEK SOFTWARE. MEDIATEK SHALL ALSO NOT BE RESPONSIBLE FOR ANY MEDIATEK
 * SOFTWARE RELEASES MADE TO RECEIVER'S SPECIFICATION OR TO CONFORM TO A PARTICULAR
 * STANDARD OR OPEN FORUM. RECEIVER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND
 * CUMULATIVE LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
 * AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
 * OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY RECEIVER TO
 * MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE.
 */

/*****************************************************************************
*  Copyright Statement:
*  --------------------
*  This software is protected by Copyright and the information contained
*  herein is confidential. The software may not be copied and the information
*  contained herein may not be used or disclosed except with the written
*  permission of MediaTek Inc. (C) 2005
*
*  BY OPENING THIS FILE, BUYER HEREBY UNEQUIVOCALLY ACKNOWLEDGES AND AGREES
*  THAT THE SOFTWARE/FIRMWARE AND ITS DOCUMENTATIONS ("MEDIATEK SOFTWARE")
*  RECEIVED FROM MEDIATEK AND/OR ITS REPRESENTATIVES ARE PROVIDED TO BUYER ON
*  AN "AS-IS" BASIS ONLY. MEDIATEK EXPRESSLY DISCLAIMS ANY AND ALL WARRANTIES,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE OR NONINFRINGEMENT.
*  NEITHER DOES MEDIATEK PROVIDE ANY WARRANTY WHATSOEVER WITH RESPECT TO THE
*  SOFTWARE OF ANY THIRD PARTY WHICH MAY BE USED BY, INCORPORATED IN, OR
*  SUPPLIED WITH THE MEDIATEK SOFTWARE, AND BUYER AGREES TO LOOK ONLY TO SUCH
*  THIRD PARTY FOR ANY WARRANTY CLAIM RELATING THERETO. MEDIATEK SHALL ALSO
*  NOT BE RESPONSIBLE FOR ANY MEDIATEK SOFTWARE RELEASES MADE TO BUYER'S
*  SPECIFICATION OR TO CONFORM TO A PARTICULAR STANDARD OR OPEN FORUM.
*
*  BUYER'S SOLE AND EXCLUSIVE REMEDY AND MEDIATEK'S ENTIRE AND CUMULATIVE
*  LIABILITY WITH RESPECT TO THE MEDIATEK SOFTWARE RELEASED HEREUNDER WILL BE,
*  AT MEDIATEK'S OPTION, TO REVISE OR REPLACE THE MEDIATEK SOFTWARE AT ISSUE,
*  OR REFUND ANY SOFTWARE LICENSE FEES OR SERVICE CHARGE PAID BY BUYER TO
*  MEDIATEK FOR SUCH MEDIATEK SOFTWARE AT ISSUE. 
*
*  THE TRANSACTION CONTEMPLATED HEREUNDER SHALL BE CONSTRUED IN ACCORDANCE
*  WITH THE LAWS OF THE STATE OF CALIFORNIA, USA, EXCLUDING ITS CONFLICT OF
*  LAWS PRINCIPLES.  ANY DISPUTES, CONTROVERSIES OR CLAIMS ARISING THEREOF AND
*  RELATED THERETO SHALL BE SETTLED BY ARBITRATION IN SAN FRANCISCO, CA, UNDER
*  THE RULES OF THE INTERNATIONAL CHAMBER OF COMMERCE (ICC).
*
*****************************************************************************/
/*****************************************************************************
 *
 * Filename:
 * ---------
 *   ov5649mipi_Sensor.c
 *
 * Project:
 * --------
 *   ALPS
 *
 * Description:
 * ------------
 *   Source code of Sensor driver
 *
 *
 *------------------------------------------------------------------------------
 * Upper this line, this part is controlled by CC/CQ. DO NOT MODIFY!!
 *============================================================================
 ****************************************************************************/

#include <linux/videodev2.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <asm/atomic.h>
#include <asm/system.h>

#include "kd_camera_hw.h"
#include "kd_imgsensor.h"
#include "kd_imgsensor_define.h"
#include "kd_imgsensor_errcode.h"

#include "ov5649mipi_Sensor.h"
#include "ov5649mipi_Camera_Sensor_para.h"
#include "ov5649mipi_CameraCustomized.h"

#define OV5649MIPI_DRIVER_TRACE
#define OV5649MIPI_DEBUG
#ifdef OV5649MIPI_DEBUG
#define SENSORDB(fmt, arg...) printk("%s: " fmt "\n", __FUNCTION__ ,##arg)
#else
#define SENSORDB(x,...)
#endif

typedef enum
{
    OV5649MIPI_SENSOR_MODE_INIT,
    OV5649MIPI_SENSOR_MODE_PREVIEW,  
    OV5649MIPI_SENSOR_MODE_CAPTURE
} OV5649MIPI_SENSOR_MODE;

/* SENSOR PRIVATE STRUCT */
typedef struct OV5649MIPI_sensor_STRUCT
{
    MSDK_SENSOR_CONFIG_STRUCT cfg_data;
    sensor_data_struct eng; /* engineer mode */
    MSDK_SENSOR_ENG_INFO_STRUCT eng_info;
    kal_uint8 mirror;

    OV5649MIPI_SENSOR_MODE ov5649mipi_sensor_mode;
    
    kal_bool video_mode;
    kal_bool NightMode;
    kal_uint16 normal_fps; /* video normal mode max fps */
    kal_uint16 night_fps; /* video night mode max fps */
    kal_uint16 FixedFps;
    kal_uint16 shutter;
    kal_uint16 gain;
    kal_uint32 pclk;
    kal_uint16 frame_length;
    kal_uint16 line_length;

    kal_uint16 dummy_pixel;
    kal_uint16 dummy_line;
} OV5649MIPI_sensor_struct;


MSDK_SCENARIO_ID_ENUM OV5649_mCurrentScenarioId = MSDK_SCENARIO_ID_CAMERA_PREVIEW;
static kal_bool OV5649MIPIAutoFlickerMode = KAL_FALSE;

extern int iReadRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u8 * a_pRecvData, u16 a_sizeRecvData, u16 i2cId);
extern int iWriteRegI2C(u8 *a_pSendData , u16 a_sizeSendData, u16 i2cId);

//extern void kdCISModuleChangePinConfig(int CameraIdx, int Pin, int On);
extern void set_maincam_powerdown(BOOL is_high);
extern signed char OV5649_otp_read_wb_group(signed char index);
extern bool OV5649_otp_update_wb(unsigned short golden_rg, unsigned short golden_bg);

UINT32 OV5649MIPISetMaxFrameRate(UINT16 u2FrameRate);

static DEFINE_SPINLOCK(ov5649mipi_drv_lock);


static OV5649MIPI_sensor_struct OV5649MIPI_sensor =
{
    .eng =
    {
        .reg = CAMERA_SENSOR_REG_DEFAULT_VALUE,
        .cct = CAMERA_SENSOR_CCT_DEFAULT_VALUE,
    },
    .eng_info =
    {
        .SensorId = 128,
        .SensorType = CMOS_SENSOR,
        .SensorOutputDataFormat = OV5649MIPI_COLOR_FORMAT,
    },
    .ov5649mipi_sensor_mode = OV5649MIPI_SENSOR_MODE_INIT,
    .shutter = 0x3D0,  
    .gain = 0x100,
    .pclk = OV5649MIPI_PREVIEW_CLK,
    .frame_length = OV5649MIPI_PV_PERIOD_LINE_NUMS,
    .line_length = OV5649MIPI_PV_PERIOD_PIXEL_NUMS,
    .dummy_pixel = 0,
    .dummy_line = 0,
};


kal_uint16 OV5649MIPI_read_cmos_sensor(kal_uint32 addr)
{
    kal_uint16 get_byte=0;

    char puSendCmd[2] = {(char)(addr >> 8), (char)(addr & 0xFF) };
    iReadRegI2C(puSendCmd, 2, (u8*)&get_byte, 1, OV5649MIPI_WRITE_ID);

    return get_byte;
}

kal_uint16 OV5649MIPI_write_cmos_sensor(kal_uint32 addr, kal_uint32 para)
{
    char puSendCmd[3] = {(char)(addr >> 8), (char)(addr & 0xFF), (char)(para & 0xFF)};
    iWriteRegI2C(puSendCmd, 3, OV5649MIPI_WRITE_ID);
}

static UINT32 OV5649_OTP_combination_read(void)
{
    uint32_t address, index,i;
    kal_int8 mid = 0x00;

    OV5649MIPI_write_cmos_sensor(0x0100, 0x01);
    mdelay(50);
    for(index = 0; index < 3; index++)
    {
        if (index<2)
        {
            // read otp --Bank 0
            OV5649MIPI_write_cmos_sensor(0x3d84, 0xc0);
            OV5649MIPI_write_cmos_sensor(0x3d85, 0x00);
            OV5649MIPI_write_cmos_sensor(0x3d86, 0x0f);
            OV5649MIPI_write_cmos_sensor(0x3d81, 0x01);
            mdelay(10);
            address = 0x3d05 + index*9;
        }
        else{
            // read otp --Bank 1
            OV5649MIPI_write_cmos_sensor(0x3d84, 0xc0);
            OV5649MIPI_write_cmos_sensor(0x3d85, 0x10);
            OV5649MIPI_write_cmos_sensor(0x3d86, 0x1f);
            OV5649MIPI_write_cmos_sensor(0x3d81, 0x01);
            mdelay(10);
            address = 0x3d05 + index*9-16;
        }

        mid = OV5649MIPI_read_cmos_sensor(address);
        //mid &= 0x7f;
        printk("*********************************************MID = %d\n",mid);
	if (0x02 == mid){
	    printk("This is truly module.\n");
            break;
	}
    }

    OV5649MIPI_write_cmos_sensor(0x3d81, 0x00);
    // clear otp buffer
    address = 0x3d00;
    for(i=0; i<16;i++) {
        OV5649MIPI_write_cmos_sensor(address + i, 0x00);
    }

    printk("OV5649_OTP_combination_read()!!!!! index==%d\n", index);
    if(index > 2)
        return FALSE;
    else
        return TRUE;
}

static void OV5649MIPI_Write_Shutter(kal_uint16 iShutter)
{
    kal_uint16 extra_line = 0, frame_length;

    #ifdef OV5649MIPI_DRIVER_TRACE
        SENSORDB("iShutter =  %d", iShutter);
    #endif
    
    /* 0x3500, 0x3501, 0x3502 will increase VBLANK to get exposure larger than frame exposure */
    /* AE doesn't update sensor gain at capture mode, thus extra exposure lines must be updated here. */
    if (!iShutter) iShutter = 1;

    if(OV5649MIPIAutoFlickerMode){
        if(OV5649MIPI_sensor.video_mode == KAL_FALSE){
            if(OV5649_mCurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_ZSD)
            {
                //Change frame 14.7fps ~ 14.9fps to do auto flick
                OV5649MIPISetMaxFrameRate(148);
            }
            else
            {
                //Change frame 29.5fps ~ 29.8fps to do auto flick
                OV5649MIPISetMaxFrameRate(296);
            }
        }
    }
    
#if 0

	if(iShutter > (OV5649MIPI_sensor.frame_length - 4))
		extra_line = iShutter - (OV5649MIPI_sensor.frame_length - 4);
	else
	    extra_line = 0;

	// Update Extra shutter
	OV5649MIPI_write_cmos_sensor(0x350c, (extra_line >> 8) & 0xFF);	
	OV5649MIPI_write_cmos_sensor(0x350d, (extra_line) & 0xFF); 
	
#endif

#if 1  

    if(iShutter > OV5649MIPI_sensor.frame_length - 4)
        frame_length = iShutter + 4;
    else
        frame_length = OV5649MIPI_sensor.frame_length;

    // Extend frame length
    OV5649MIPI_write_cmos_sensor(0x380f, frame_length & 0xFF);
    OV5649MIPI_write_cmos_sensor(0x380e, frame_length >> 8);
    
#endif

    // Update Shutter
    OV5649MIPI_write_cmos_sensor(0x3502, (iShutter << 4) & 0xFF);
    OV5649MIPI_write_cmos_sensor(0x3501, (iShutter >> 4) & 0xFF);     
    OV5649MIPI_write_cmos_sensor(0x3500, (iShutter >> 12) & 0x0F);
}   /*  OV5649MIPI_Write_Shutter  */

static void OV5649MIPI_Set_Dummy(const kal_uint16 iDummyPixels, const kal_uint16 iDummyLines)
{
    kal_uint16 hactive, vactive, line_length, frame_length;

    #ifdef OV5649MIPI_DRIVER_TRACE
        SENSORDB("iDummyPixels = %d, iDummyLines = %d ", iDummyPixels, iDummyLines);
    #endif

    if (OV5649MIPI_SENSOR_MODE_PREVIEW == OV5649MIPI_sensor.ov5649mipi_sensor_mode){
        line_length = OV5649MIPI_PV_PERIOD_PIXEL_NUMS + iDummyPixels;
        frame_length = OV5649MIPI_PV_PERIOD_LINE_NUMS + iDummyLines;
    }else{
        line_length = OV5649MIPI_FULL_PERIOD_PIXEL_NUMS + iDummyPixels;
        frame_length = OV5649MIPI_FULL_PERIOD_LINE_NUMS + iDummyLines;
    }
    
    if ((line_length >= 0x1FFF)||(frame_length >= 0xFFF))
        return ;

    OV5649MIPI_sensor.dummy_pixel = iDummyPixels;
    OV5649MIPI_sensor.dummy_line = iDummyLines;
    OV5649MIPI_sensor.line_length = line_length;
    OV5649MIPI_sensor.frame_length = frame_length;
    
    OV5649MIPI_write_cmos_sensor(0x380c, line_length >> 8);
    OV5649MIPI_write_cmos_sensor(0x380d, line_length & 0xFF);
    OV5649MIPI_write_cmos_sensor(0x380e, frame_length >> 8);
    OV5649MIPI_write_cmos_sensor(0x380f, frame_length & 0xFF);
    
}   /*  OV5649MIPI_Set_Dummy  */


UINT32 OV5649MIPISetMaxFrameRate(UINT16 u2FrameRate)
{
    kal_int16 dummy_line;
    kal_uint16 frame_length = OV5649MIPI_sensor.frame_length;
    unsigned long flags;

    #ifdef OV5649MIPI_DRIVER_TRACE
        SENSORDB("u2FrameRate = %d ", u2FrameRate);
    #endif

    frame_length= (10 * OV5649MIPI_sensor.pclk) / u2FrameRate / OV5649MIPI_sensor.line_length;

    spin_lock_irqsave(&ov5649mipi_drv_lock, flags);
    OV5649MIPI_sensor.frame_length = frame_length;
    spin_unlock_irqrestore(&ov5649mipi_drv_lock, flags);

    if(OV5649_mCurrentScenarioId == MSDK_SCENARIO_ID_CAMERA_ZSD)
        dummy_line = frame_length - OV5649MIPI_FULL_PERIOD_LINE_NUMS;
    else
        dummy_line = frame_length - OV5649MIPI_PV_PERIOD_LINE_NUMS;

    OV5649MIPI_Set_Dummy(OV5649MIPI_sensor.dummy_pixel, dummy_line); /* modify dummy_pixel must gen AE table again */
}   /*  OV5649MIPISetMaxFrameRate  */

/*************************************************************************
* FUNCTION
*   OV5649MIPI_SetShutter
*
* DESCRIPTION
*   This function set e-shutter of OV5649MIPI to change exposure time.
*
* PARAMETERS
*   iShutter : exposured lines
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void set_OV5649MIPI_shutter(kal_uint16 iShutter)
{
    unsigned long flags;
    
    spin_lock_irqsave(&ov5649mipi_drv_lock, flags);
    OV5649MIPI_sensor.shutter = iShutter;
    spin_unlock_irqrestore(&ov5649mipi_drv_lock, flags);
    
    OV5649MIPI_Write_Shutter(iShutter);
}   /*  Set_OV5649MIPI_Shutter */

static kal_uint16 OV5649MIPI_Reg2Gain(const kal_uint8 iReg)
{
    kal_uint16 iGain ;
    /* Range: 1x to 32x */
    iGain = (iReg >> 4) * BASEGAIN + (iReg & 0xF) * BASEGAIN / 16; 
    return iGain ;
}

 kal_uint8 OV5649MIPI_Gain2Reg(const kal_uint16 iGain)
{
    kal_uint16 iReg = 0x0000;
    
    iReg = ((iGain / BASEGAIN) << 4) + ((iGain % BASEGAIN) * 16 / BASEGAIN);
    iReg = iReg & 0xFF;
    return (kal_uint8)iReg;
}

/*************************************************************************
* FUNCTION
*   OV5649MIPI_SetGain
*
* DESCRIPTION
*   This function is to set global gain to sensor.
*
* PARAMETERS
*   iGain : sensor global gain(base: 0x40)
*
* RETURNS
*   the actually gain set to sensor.
*
* GLOBALS AFFECTED
*
*************************************************************************/
kal_uint16 OV5649MIPI_SetGain(kal_uint16 iGain)
{
    kal_uint8 iRegGain;

    OV5649MIPI_sensor.gain = iGain;

    /* 0x350A[0:1], 0x350B[0:7] AGC real gain */
    /* [0:3] = N meams N /16 X  */
    /* [4:9] = M meams M X       */
    /* Total gain = M + N /16 X   */

    //
    if(iGain >= BASEGAIN && iGain <= 32 * BASEGAIN){
    
        iRegGain = OV5649MIPI_Gain2Reg(iGain);

        #ifdef OV5649MIPI_DRIVER_TRACE
            SENSORDB("iGain = %d , iRegGain = 0x%x ", iGain, iRegGain);
        #endif

        if (iRegGain < 0x10) iRegGain = 0x10;
        OV5649MIPI_write_cmos_sensor(0x350b, iRegGain);
    } else {
        SENSORDB("Error gain setting");
    }
    
    return iGain;
}   /*  OV5649MIPI_SetGain  */


void OV5649MIPI_Set_Mirror_Flip(kal_uint8 image_mirror)
{
    SENSORDB("image_mirror = %d", image_mirror);

    /********************************************************
       *
       *   0x3820[2] Sensor Vertical flip
       *   0x3821[2] Sensor Horizontal mirror
       *
       ********************************************************/
    
	switch (image_mirror)
	{
		case IMAGE_NORMAL:
		    OV5649MIPI_write_cmos_sensor(0x3820,((OV5649MIPI_read_cmos_sensor(0x3820) & !0x02) | 0x00));
		    OV5649MIPI_write_cmos_sensor(0x3821,((OV5649MIPI_read_cmos_sensor(0x3821) & !0x02) | 0x00));
		    break;
		case IMAGE_H_MIRROR:
		    OV5649MIPI_write_cmos_sensor(0x3820,((OV5649MIPI_read_cmos_sensor(0x3820) & !0x02) | 0x00));
		    OV5649MIPI_write_cmos_sensor(0x3821,((OV5649MIPI_read_cmos_sensor(0x3821) & !0x02) | 0x02));
		    break;
		case IMAGE_V_MIRROR:
		    OV5649MIPI_write_cmos_sensor(0x3820,((OV5649MIPI_read_cmos_sensor(0x3820) & !0x02) | 0x02));
		    OV5649MIPI_write_cmos_sensor(0x3821,((OV5649MIPI_read_cmos_sensor(0x3821) & !0x02) | 0x00));		
		    break;
		case IMAGE_HV_MIRROR:
		    OV5649MIPI_write_cmos_sensor(0x3820,((OV5649MIPI_read_cmos_sensor(0x3820) & !0x02) | 0x02));
		    OV5649MIPI_write_cmos_sensor(0x3821,((OV5649MIPI_read_cmos_sensor(0x3821) & !0x02) | 0x02));
		    break;
		default:
		    SENSORDB("Error image_mirror setting");
	}
}


/*************************************************************************
* FUNCTION
*   OV5649MIPI_NightMode
*
* DESCRIPTION
*   This function night mode of OV5649MIPI.
*
* PARAMETERS
*   bEnable: KAL_TRUE -> enable night mode, otherwise, disable night mode
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
void OV5649MIPI_night_mode(kal_bool enable)
{
/*No Need to implement this function*/ 
}   /*  OV5649MIPI_night_mode  */


/* write camera_para to sensor register */
static void OV5649MIPI_camera_para_to_sensor(void)
{
    kal_uint32 i;

    SENSORDB("OV5649MIPI_camera_para_to_sensor\n");

    for (i = 0; 0xFFFFFFFF != OV5649MIPI_sensor.eng.reg[i].Addr; i++)
    {
        OV5649MIPI_write_cmos_sensor(OV5649MIPI_sensor.eng.reg[i].Addr, OV5649MIPI_sensor.eng.reg[i].Para);
    }
    for (i = OV5649MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != OV5649MIPI_sensor.eng.reg[i].Addr; i++)
    {
        OV5649MIPI_write_cmos_sensor(OV5649MIPI_sensor.eng.reg[i].Addr, OV5649MIPI_sensor.eng.reg[i].Para);
    }
    OV5649MIPI_SetGain(OV5649MIPI_sensor.gain); /* update gain */
}

/* update camera_para from sensor register */
static void OV5649MIPI_sensor_to_camera_para(void)
{
    kal_uint32 i,temp_data;

    SENSORDB("OV5649MIPI_sensor_to_camera_para\n");

    for (i = 0; 0xFFFFFFFF != OV5649MIPI_sensor.eng.reg[i].Addr; i++)
    {
        temp_data =OV5649MIPI_read_cmos_sensor(OV5649MIPI_sensor.eng.reg[i].Addr);
     
        spin_lock(&ov5649mipi_drv_lock);
        OV5649MIPI_sensor.eng.reg[i].Para = temp_data;
        spin_unlock(&ov5649mipi_drv_lock);
    }
    for (i = OV5649MIPI_FACTORY_START_ADDR; 0xFFFFFFFF != OV5649MIPI_sensor.eng.reg[i].Addr; i++)
    {
        temp_data =OV5649MIPI_read_cmos_sensor(OV5649MIPI_sensor.eng.reg[i].Addr);
    
        spin_lock(&ov5649mipi_drv_lock);
        OV5649MIPI_sensor.eng.reg[i].Para = temp_data;
        spin_unlock(&ov5649mipi_drv_lock);
    }
}

/* ------------------------ Engineer mode ------------------------ */
inline static void OV5649MIPI_get_sensor_group_count(kal_int32 *sensor_count_ptr)
{

    SENSORDB("OV5649MIPI_get_sensor_group_count\n");

    *sensor_count_ptr = OV5649MIPI_GROUP_TOTAL_NUMS;
}

inline static void OV5649MIPI_get_sensor_group_info(MSDK_SENSOR_GROUP_INFO_STRUCT *para)
{

    SENSORDB("OV5649MIPI_get_sensor_group_info\n");

    switch (para->GroupIdx)
    {
    case OV5649MIPI_PRE_GAIN:
        sprintf(para->GroupNamePtr, "CCT");
        para->ItemCount = 5;
        break;
    case OV5649MIPI_CMMCLK_CURRENT:
        sprintf(para->GroupNamePtr, "CMMCLK Current");
        para->ItemCount = 1;
        break;
    case OV5649MIPI_FRAME_RATE_LIMITATION:
        sprintf(para->GroupNamePtr, "Frame Rate Limitation");
        para->ItemCount = 2;
        break;
    case OV5649MIPI_REGISTER_EDITOR:
        sprintf(para->GroupNamePtr, "Register Editor");
        para->ItemCount = 2;
        break;
    default:
        ASSERT(0);
  }
}

inline static void OV5649MIPI_get_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{

    const static kal_char *cct_item_name[] = {"SENSOR_BASEGAIN", "Pregain-R", "Pregain-Gr", "Pregain-Gb", "Pregain-B"};
    const static kal_char *editer_item_name[] = {"REG addr", "REG value"};
  
    SENSORDB("OV5649MIPI_get_sensor_item_info");

    switch (para->GroupIdx)
    {
    case OV5649MIPI_PRE_GAIN:
        switch (para->ItemIdx)
        {
        case OV5649MIPI_SENSOR_BASEGAIN:
        case OV5649MIPI_PRE_GAIN_R_INDEX:
        case OV5649MIPI_PRE_GAIN_Gr_INDEX:
        case OV5649MIPI_PRE_GAIN_Gb_INDEX:
        case OV5649MIPI_PRE_GAIN_B_INDEX:
            break;
        default:
            ASSERT(0);
        }
        sprintf(para->ItemNamePtr, cct_item_name[para->ItemIdx - OV5649MIPI_SENSOR_BASEGAIN]);
        para->ItemValue = OV5649MIPI_sensor.eng.cct[para->ItemIdx].Para * 1000 / BASEGAIN;
        para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
        para->Min = OV5649MIPI_MIN_ANALOG_GAIN * 1000;
        para->Max = OV5649MIPI_MAX_ANALOG_GAIN * 1000;
        break;
    case OV5649MIPI_CMMCLK_CURRENT:
        switch (para->ItemIdx)
        {
        case 0:
            sprintf(para->ItemNamePtr, "Drv Cur[2,4,6,8]mA");
            switch (OV5649MIPI_sensor.eng.reg[OV5649MIPI_CMMCLK_CURRENT_INDEX].Para)
            {
            case ISP_DRIVING_2MA:
                para->ItemValue = 2;
                break;
            case ISP_DRIVING_4MA:
                para->ItemValue = 4;
                break;
            case ISP_DRIVING_6MA:
                para->ItemValue = 6;
                break;
            case ISP_DRIVING_8MA:
                para->ItemValue = 8;
                break;
            default:
                ASSERT(0);
            }
            para->IsTrueFalse = para->IsReadOnly = KAL_FALSE;
            para->IsNeedRestart = KAL_TRUE;
            para->Min = 2;
            para->Max = 8;
            break;
        default:
            ASSERT(0);
        }
        break;
    case OV5649MIPI_FRAME_RATE_LIMITATION:
        switch (para->ItemIdx)
        {
        case 0:
            sprintf(para->ItemNamePtr, "Max Exposure Lines");
            para->ItemValue = 5998;
            break;
        case 1:
            sprintf(para->ItemNamePtr, "Min Frame Rate");
            para->ItemValue = 5;
            break;
        default:
            ASSERT(0);
        }
        para->IsTrueFalse = para->IsNeedRestart = KAL_FALSE;
        para->IsReadOnly = KAL_TRUE;
        para->Min = para->Max = 0;
        break;
    case OV5649MIPI_REGISTER_EDITOR:
        switch (para->ItemIdx)
        {
        case 0:
        case 1:
            sprintf(para->ItemNamePtr, editer_item_name[para->ItemIdx]);
            para->ItemValue = 0;
            para->IsTrueFalse = para->IsReadOnly = para->IsNeedRestart = KAL_FALSE;
            para->Min = 0;
            para->Max = (para->ItemIdx == 0 ? 0xFFFF : 0xFF);
            break;
        default:
            ASSERT(0);
        }
        break;
    default:
        ASSERT(0);
  }
}

inline static kal_bool OV5649MIPI_set_sensor_item_info(MSDK_SENSOR_ITEM_INFO_STRUCT *para)
{
    kal_uint16 temp_para;

    SENSORDB("OV5649MIPI_set_sensor_item_info\n");

    switch (para->GroupIdx)
    {
    case OV5649MIPI_PRE_GAIN:
        switch (para->ItemIdx)
        {
        case OV5649MIPI_SENSOR_BASEGAIN:
        case OV5649MIPI_PRE_GAIN_R_INDEX:
        case OV5649MIPI_PRE_GAIN_Gr_INDEX:
        case OV5649MIPI_PRE_GAIN_Gb_INDEX:
        case OV5649MIPI_PRE_GAIN_B_INDEX:
            spin_lock(&ov5649mipi_drv_lock);
            OV5649MIPI_sensor.eng.cct[para->ItemIdx].Para = para->ItemValue * BASEGAIN / 1000;
            spin_unlock(&ov5649mipi_drv_lock);
            OV5649MIPI_SetGain(OV5649MIPI_sensor.gain); /* update gain */
            break;
        default:
            ASSERT(0);
        }
        break;
    case OV5649MIPI_CMMCLK_CURRENT:
        switch (para->ItemIdx)
        {
        case 0:
            switch (para->ItemValue)
            {
            case 2:
                temp_para = ISP_DRIVING_2MA;
                break;
            case 3:
            case 4:
                temp_para = ISP_DRIVING_4MA;
                break;
            case 5:
            case 6:
                temp_para = ISP_DRIVING_6MA;
                break;
            default:
                temp_para = ISP_DRIVING_8MA;
                break;
            }
            spin_lock(&ov5649mipi_drv_lock);
            //OV5649MIPI_set_isp_driving_current(temp_para);
            OV5649MIPI_sensor.eng.reg[OV5649MIPI_CMMCLK_CURRENT_INDEX].Para = temp_para;
            spin_unlock(&ov5649mipi_drv_lock);
            break;
        default:
            ASSERT(0);
        }
        break;
    case OV5649MIPI_FRAME_RATE_LIMITATION:
        ASSERT(0);
        break;
    case OV5649MIPI_REGISTER_EDITOR:
        switch (para->ItemIdx)
        {
        static kal_uint32 fac_sensor_reg;
        case 0:
            if (para->ItemValue < 0 || para->ItemValue > 0xFFFF) return KAL_FALSE;
            fac_sensor_reg = para->ItemValue;
            break;
        case 1:
            if (para->ItemValue < 0 || para->ItemValue > 0xFF) return KAL_FALSE;
            OV5649MIPI_write_cmos_sensor(fac_sensor_reg, para->ItemValue);
            break;
        default:
            ASSERT(0);
        }
        break;
    default:
        ASSERT(0);
    }

    return KAL_TRUE;
}

static void OV5649MIPI_Sensor_Init(void)
{
    SENSORDB("Enter!");

   /*****************************************************************************
    0x3037  SC_CMMN_PLL_CTR13 
        SC_CMMN_PLL_CTR13[4] pll_root_div 0: bypass 1: /2
        SC_CMMN_PLL_CTR13[3:0] pll_prediv 1, 2, 3, 4, 6, 8
        
    0x3036  SC_CMMN_PLL_MULTIPLIER 
        SC_CMMN_PLL_MULTIPLIER[7:0] PLL_multiplier(4~252) This can be any integer during 4~127 and only even integer during 128 ~ 252
    
    0x3035  SC_CMMN_PLL_CTR1
        SC_CMMN_PLL_CTR1[7:4] system_pll_div
        SC_CMMN_PLL_CTR1[3:0] scale_divider_mipi
    
    0x3034  SC_CMMN_PLL_CTR0
        SC_CMMN_PLL_CTR0[6:4] pll_charge_pump
        SC_CMMN_PLL_CTR0[3:0] mipi_bit_mode
    
    0x3106  SRB CTRL
        SRB CTRL[3:2] PLL_clock_divider
        SRB CTRL[1] rst_arb
        SRB CTRL[0] sclk_arb
    
    pll_prediv_map[] = {2, 2, 4, 6, 8, 3, 12, 5, 16, 2, 2, 2, 2, 2, 2, 2};
    system_pll_div_map[] = {16, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    pll_root_div_map[] = {1, 2};
    mipi_bit_mode_map[] = {2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 5, 2, 2, 2, 2, 2};
    PLL_clock_divider_map[] = {1, 2, 4, 1};
    
    VCO = XVCLK * 2 / pll_prediv_map[pll_prediv] * PLL_multiplier;
    sysclk = VCO * 2 / system_pll_div_map[system_pll_div] / pll_root_div_map[pll_root_div] / mipi_bit_mode_map[mipi_bit_mode] / PLL_clock_divider_map[PLL_clock_divider]
    
    Change Register
    
    VCO = XVCLK * 2 / pll_prediv_map[0x3037[3:0]] * 0x3036[7:0];
    sysclk = VCO * 2 / system_pll_div_map[0x3035[7:4]] / pll_root_div_map[0x3037[4]] / mipi_bit_mode_map[0x3034[3:0]] / PLL_clock_divider_map[0x3106[3:2]]

    XVCLK = 24 MHZ
    0x3106 0x05
    0x3037 0x03
    0x3036 0x69
    0x3035 0x21
    0x3034 0x1A

    VCO = 24 * 2 / 6 * 105
    sysclk = VCO * 2 / 2 / 1 / 5 / 2
    sysclk = 84 MHZ
    */

    //@@ global setting                                                                 
    OV5649MIPI_write_cmos_sensor(0x0100, 0x00); // Software Standy
    OV5649MIPI_write_cmos_sensor(0x0103, 0x01); // Software Reset

    mDELAY(5);

    OV5649MIPI_write_cmos_sensor(0x3001, 0x00); // D[7:0] set to input
    OV5649MIPI_write_cmos_sensor(0x3002, 0x00); // D[11:8] set to input
    OV5649MIPI_write_cmos_sensor(0x3011, 0x02); // Drive strength 2x
    OV5649MIPI_write_cmos_sensor(0x3018, 0x4c); // MIPI 2 lane
    OV5649MIPI_write_cmos_sensor(0x3022, 0x00);
    
    OV5649MIPI_write_cmos_sensor(0x3034, 0x1a); // 10-bit mode
    OV5649MIPI_write_cmos_sensor(0x3035, 0x21); // PLL
    OV5649MIPI_write_cmos_sensor(0x3036, 0x69); // PLL
    OV5649MIPI_write_cmos_sensor(0x3037, 0x03); // PLL
    
    OV5649MIPI_write_cmos_sensor(0x3038, 0x00); // PLL
    OV5649MIPI_write_cmos_sensor(0x3039, 0x00); // PLL
    OV5649MIPI_write_cmos_sensor(0x303a, 0x00); // PLLS
    OV5649MIPI_write_cmos_sensor(0x303b, 0x19); // PLLS
    OV5649MIPI_write_cmos_sensor(0x303c, 0x11); // PLLS
    OV5649MIPI_write_cmos_sensor(0x303d, 0x30); // PLLS
    OV5649MIPI_write_cmos_sensor(0x3105, 0x11);
    OV5649MIPI_write_cmos_sensor(0x3106, 0x05); // PLL
    OV5649MIPI_write_cmos_sensor(0x3304, 0x28);
    OV5649MIPI_write_cmos_sensor(0x3305, 0x41);
    OV5649MIPI_write_cmos_sensor(0x3306, 0x30);
    OV5649MIPI_write_cmos_sensor(0x3308, 0x00);
    OV5649MIPI_write_cmos_sensor(0x3309, 0xc8);
    OV5649MIPI_write_cmos_sensor(0x330a, 0x01);
    OV5649MIPI_write_cmos_sensor(0x330b, 0x90);
    OV5649MIPI_write_cmos_sensor(0x330c, 0x02);
    OV5649MIPI_write_cmos_sensor(0x330d, 0x58);
    OV5649MIPI_write_cmos_sensor(0x330e, 0x03);
    OV5649MIPI_write_cmos_sensor(0x330f, 0x20);
    OV5649MIPI_write_cmos_sensor(0x3300, 0x00);

    // exposure time
    //OV5649MIPI_write_cmos_sensor(0x3500, 0x00); // exposure [19:16]
    //OV5649MIPI_write_cmos_sensor(0x3501, 0x3d); // exposure [15:8]
    //OV5649MIPI_write_cmos_sensor(0x3502, 0x00); // exposure [7:0], exposure = 0x3d0 = 976

    OV5649MIPI_write_cmos_sensor(0x3503, 0x07); // gain has no delay, manual agc/aec

    // gain
    //OV5649MIPI_write_cmos_sensor(0x350a, 0x00); // gain[9:8]
    //OV5649MIPI_write_cmos_sensor(0x350b, 0x40); // gain[7:0], gain = 4x
    
    OV5649MIPI_write_cmos_sensor(0x3601, 0x33); // analog control
    OV5649MIPI_write_cmos_sensor(0x3602, 0x00); // analog control
    OV5649MIPI_write_cmos_sensor(0x3611, 0x0e); // analog control
    OV5649MIPI_write_cmos_sensor(0x3612, 0x2b); // analog control
    OV5649MIPI_write_cmos_sensor(0x3614, 0x50); // analog control
    OV5649MIPI_write_cmos_sensor(0x3620, 0x33); // analog control
    OV5649MIPI_write_cmos_sensor(0x3622, 0x00); // analog control
    OV5649MIPI_write_cmos_sensor(0x3630, 0xad); // analog control
    OV5649MIPI_write_cmos_sensor(0x3631, 0x00); // analog control
    OV5649MIPI_write_cmos_sensor(0x3632, 0x94); // analog control
    OV5649MIPI_write_cmos_sensor(0x3633, 0x17); // analog control
    OV5649MIPI_write_cmos_sensor(0x3634, 0x14); // analog control
    OV5649MIPI_write_cmos_sensor(0x3705, 0x2a); // analog control
    OV5649MIPI_write_cmos_sensor(0x3708, 0x66); // analog control
    OV5649MIPI_write_cmos_sensor(0x3709, 0x52); // analog control
    OV5649MIPI_write_cmos_sensor(0x370b, 0x23); // analog control
    OV5649MIPI_write_cmos_sensor(0x370c, 0xc3); // analog control
    OV5649MIPI_write_cmos_sensor(0x370d, 0x00); // analog control
    OV5649MIPI_write_cmos_sensor(0x370e, 0x00); // analog control
    OV5649MIPI_write_cmos_sensor(0x371c, 0x07); // analog control
    OV5649MIPI_write_cmos_sensor(0x3739, 0xd2); // analog control
    OV5649MIPI_write_cmos_sensor(0x373c, 0x00);
    
    OV5649MIPI_write_cmos_sensor(0x3800, 0x00); // xstart = 0
    OV5649MIPI_write_cmos_sensor(0x3801, 0x00); // xstart
    OV5649MIPI_write_cmos_sensor(0x3802, 0x00); // ystart = 0
    OV5649MIPI_write_cmos_sensor(0x3803, 0x00); // ystart
    OV5649MIPI_write_cmos_sensor(0x3804, 0x0a); // xend = 2623
    OV5649MIPI_write_cmos_sensor(0x3805, 0x3f); // yend
    OV5649MIPI_write_cmos_sensor(0x3806, 0x07); // yend = 1955
    OV5649MIPI_write_cmos_sensor(0x3807, 0xa3); // yend
    OV5649MIPI_write_cmos_sensor(0x3808, 0x05); // x output size = 1296
    OV5649MIPI_write_cmos_sensor(0x3809, 0x10); // x output size
    OV5649MIPI_write_cmos_sensor(0x380a, 0x03); // y output size = 972
    OV5649MIPI_write_cmos_sensor(0x380b, 0xcc); // y output size
    OV5649MIPI_write_cmos_sensor(0x380c, 0x0b); // hts = 2816
    OV5649MIPI_write_cmos_sensor(0x380d, 0x00); // hts
    OV5649MIPI_write_cmos_sensor(0x380e, 0x03); // vts = 992
    OV5649MIPI_write_cmos_sensor(0x380f, 0xe0); // vts
    
    OV5649MIPI_write_cmos_sensor(0x3810, 0x00); // isp x win = 8
    OV5649MIPI_write_cmos_sensor(0x3811, 0x08); // isp x win
    OV5649MIPI_write_cmos_sensor(0x3812, 0x00); // isp y win = 4
    OV5649MIPI_write_cmos_sensor(0x3813, 0x04); // isp y win
    OV5649MIPI_write_cmos_sensor(0x3814, 0x31); // x inc
    OV5649MIPI_write_cmos_sensor(0x3815, 0x31); // y inc
    OV5649MIPI_write_cmos_sensor(0x3817, 0x00); // hsync start

    // Horizontal binning
    OV5649MIPI_write_cmos_sensor(0x3820, 0x08); // flip off, v bin off
    OV5649MIPI_write_cmos_sensor(0x3821, 0x07); // mirror on, h bin on
    
    OV5649MIPI_write_cmos_sensor(0x3826, 0x03);
    OV5649MIPI_write_cmos_sensor(0x3829, 0x00);
    OV5649MIPI_write_cmos_sensor(0x382b, 0x0b);
    OV5649MIPI_write_cmos_sensor(0x3830, 0x00);
    OV5649MIPI_write_cmos_sensor(0x3836, 0x00);
    OV5649MIPI_write_cmos_sensor(0x3837, 0x00);
    OV5649MIPI_write_cmos_sensor(0x3838, 0x00);
    OV5649MIPI_write_cmos_sensor(0x3839, 0x04);
    OV5649MIPI_write_cmos_sensor(0x383a, 0x00);
    OV5649MIPI_write_cmos_sensor(0x383b, 0x01);

    OV5649MIPI_write_cmos_sensor(0x3b00, 0x00); // strobe off
    OV5649MIPI_write_cmos_sensor(0x3b02, 0x08); // shutter delay
    OV5649MIPI_write_cmos_sensor(0x3b03, 0x00); // shutter delay
    OV5649MIPI_write_cmos_sensor(0x3b04, 0x04); // frex_exp
    OV5649MIPI_write_cmos_sensor(0x3b05, 0x00); // frex_exp
    OV5649MIPI_write_cmos_sensor(0x3b06, 0x04);
    OV5649MIPI_write_cmos_sensor(0x3b07, 0x08); // frex inv
    OV5649MIPI_write_cmos_sensor(0x3b08, 0x00); // frex exp req
    OV5649MIPI_write_cmos_sensor(0x3b09, 0x02); // frex end option
    OV5649MIPI_write_cmos_sensor(0x3b0a, 0x04); // frex rst length
    OV5649MIPI_write_cmos_sensor(0x3b0b, 0x00); // frex strobe width
    OV5649MIPI_write_cmos_sensor(0x3b0c, 0x3d); // frex strobe width
    OV5649MIPI_write_cmos_sensor(0x3f01, 0x0d);
    OV5649MIPI_write_cmos_sensor(0x3f0f, 0xf5);
    
    OV5649MIPI_write_cmos_sensor(0x4000, 0x89); // blc enable
    OV5649MIPI_write_cmos_sensor(0x4001, 0x02); // blc start line
    OV5649MIPI_write_cmos_sensor(0x4002, 0x45); // blc auto, reset frame number = 5
    OV5649MIPI_write_cmos_sensor(0x4004, 0x02); // black line number
    OV5649MIPI_write_cmos_sensor(0x4005, 0x18); // blc normal freeze
    OV5649MIPI_write_cmos_sensor(0x4006, 0x08);
    OV5649MIPI_write_cmos_sensor(0x4007, 0x10);
    OV5649MIPI_write_cmos_sensor(0x4008, 0x00);
    OV5649MIPI_write_cmos_sensor(0x4300, 0xf8);
    OV5649MIPI_write_cmos_sensor(0x4303, 0xff);
    OV5649MIPI_write_cmos_sensor(0x4304, 0x00);
    OV5649MIPI_write_cmos_sensor(0x4307, 0xff);
    OV5649MIPI_write_cmos_sensor(0x4520, 0x00);
    OV5649MIPI_write_cmos_sensor(0x4521, 0x00);
    OV5649MIPI_write_cmos_sensor(0x4511, 0x22);
    OV5649MIPI_write_cmos_sensor(0x481f, 0x3c); // MIPI clk prepare min
    OV5649MIPI_write_cmos_sensor(0x4826, 0x00); // MIPI hs prepare min
    OV5649MIPI_write_cmos_sensor(0x4837, 0x18); // MIPI global timing
    OV5649MIPI_write_cmos_sensor(0x4b00, 0x06);
    OV5649MIPI_write_cmos_sensor(0x4b01, 0x0a);
    OV5649MIPI_write_cmos_sensor(0x5000, 0xff); // bpc on, wpc on
    OV5649MIPI_write_cmos_sensor(0x5001, 0x00); // awb disable
    OV5649MIPI_write_cmos_sensor(0x5002, 0x41); // win enable, awb gain enable
    OV5649MIPI_write_cmos_sensor(0x5003, 0x0a); // buf en, bin auto en
    OV5649MIPI_write_cmos_sensor(0x5004, 0x00); // size man off
    OV5649MIPI_write_cmos_sensor(0x5043, 0x00);
    OV5649MIPI_write_cmos_sensor(0x5013, 0x00);
    OV5649MIPI_write_cmos_sensor(0x501f, 0x03); // ISP output data
    OV5649MIPI_write_cmos_sensor(0x503d, 0x00); // test pattern off
    OV5649MIPI_write_cmos_sensor(0x5180, 0x08); // manual wb gain on
    OV5649MIPI_write_cmos_sensor(0x5a00, 0x08);
    OV5649MIPI_write_cmos_sensor(0x5b00, 0x01);
    OV5649MIPI_write_cmos_sensor(0x5b01, 0x40);
    OV5649MIPI_write_cmos_sensor(0x5b02, 0x00);
    OV5649MIPI_write_cmos_sensor(0x5b03, 0xf0);
    
    OV5649MIPI_write_cmos_sensor(0x0100, 0x01); // wake up from software sleep
    //OV5649MIPI_write_cmos_sensor(0x350b, 0x80); // gain = 8x
    OV5649MIPI_write_cmos_sensor(0x4837, 0x17); // MIPI global timing
/*
    //OTP calibration
    OV5649MIPI_write_cmos_sensor(0x0100, 0x01);
    mdelay(50);
    OV5649_otp_update_wb(0x011E, 0x018C);
    SENSORDB("This is truly module!!!!!!!!!!!!!\n");
*/
}   /*  OV5649MIPI_Sensor_Init  */


static void OV5649MIPI_Preview_Setting(void)
{
    SENSORDB("Enter!");

    /********************************************************
       *
       *   1296x972 30fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/

    //OV5649MIPI_write_cmos_sensor(0x3500, 0x00); // exposure [19:16]
    //OV5649MIPI_write_cmos_sensor(0x3501, 0x3d); // exposure
    //OV5649MIPI_write_cmos_sensor(0x3502, 0x00); // exposure
    OV5649MIPI_write_cmos_sensor(0x3708, 0x66);

    OV5649MIPI_write_cmos_sensor(0x3709, 0x52);
    OV5649MIPI_write_cmos_sensor(0x370c, 0xc3);
    OV5649MIPI_write_cmos_sensor(0x3800, 0x00); // x start = 0
    OV5649MIPI_write_cmos_sensor(0x3801, 0x00); // x start
    OV5649MIPI_write_cmos_sensor(0x3802, 0x00); // y start = 0
    OV5649MIPI_write_cmos_sensor(0x3803, 0x00); // y start
    OV5649MIPI_write_cmos_sensor(0x3804, 0x0a); // xend = 2623
    OV5649MIPI_write_cmos_sensor(0x3805, 0x3f); // xend
    OV5649MIPI_write_cmos_sensor(0x3806, 0x07); // yend = 1955
    OV5649MIPI_write_cmos_sensor(0x3807, 0xa3); // yend
    OV5649MIPI_write_cmos_sensor(0x3808, 0x05); // x output size = 1296
    OV5649MIPI_write_cmos_sensor(0x3809, 0x10); // x output size
    OV5649MIPI_write_cmos_sensor(0x380a, 0x03); // y output size = 972
    OV5649MIPI_write_cmos_sensor(0x380b, 0xcc); // y output size
    OV5649MIPI_write_cmos_sensor(0x380c, 0x0b); // hts = 2816
    OV5649MIPI_write_cmos_sensor(0x380d, 0x00); // hts
    OV5649MIPI_write_cmos_sensor(0x380e, 0x03); // vts = 992
    OV5649MIPI_write_cmos_sensor(0x380f, 0xE0); // vts
    
    OV5649MIPI_write_cmos_sensor(0x3810, 0x00); // isp x win = 8
    OV5649MIPI_write_cmos_sensor(0x3811, 0x08); // isp x win
    OV5649MIPI_write_cmos_sensor(0x3812, 0x00); // isp y win = 4
    OV5649MIPI_write_cmos_sensor(0x3813, 0x04); // isp y win
    OV5649MIPI_write_cmos_sensor(0x3814, 0x31); // x inc
    OV5649MIPI_write_cmos_sensor(0x3815, 0x31); // y inc
    OV5649MIPI_write_cmos_sensor(0x3817, 0x00); // hsync start
    OV5649MIPI_write_cmos_sensor(0x3820, 0x08); // flip off, v bin off
    OV5649MIPI_write_cmos_sensor(0x3821, 0x07); // mirror on, h bin on
    OV5649MIPI_write_cmos_sensor(0x4004, 0x02); // black line number
    //OV5649MIPI_write_cmos_sensor(0x4005, 0x1a); // blc normal freeze
    OV5649MIPI_write_cmos_sensor(0x4005, 0x18); // blc normal freeze
    OV5649MIPI_write_cmos_sensor(0x4050, 0x37); // blc normal freeze
    OV5649MIPI_write_cmos_sensor(0x4051, 0x8f); // blc normal freeze

    //OV5649MIPI_write_cmos_sensor(0x350b, 0x80); // gain = 8x
    OV5649MIPI_write_cmos_sensor(0x4837, 0x17); // MIPI global timing
    
    OV5649MIPI_write_cmos_sensor(0x0100, 0x01); //Stream On
}   /*  OV5649MIPI_Preview_Setting  */


static void OV5649MIPI_Capture_Setting(void)
{
    SENSORDB("Enter!");

    /********************************************************
       *
       *   2592x1944 15fps 2 lane MIPI 420Mbps/lane
       *
       ********************************************************/

    //OV5649MIPI_write_cmos_sensor(0x3500, 0x00); // exposure [19:16]
    //OV5649MIPI_write_cmos_sensor(0x3501, 0x7b); // exposure
    //OV5649MIPI_write_cmos_sensor(0x3502, 0x00); // exposure
    OV5649MIPI_write_cmos_sensor(0x3708, 0x63);
    
    OV5649MIPI_write_cmos_sensor(0x3709, 0x12);
    OV5649MIPI_write_cmos_sensor(0x370c, 0xc0);
    OV5649MIPI_write_cmos_sensor(0x3800, 0x00); // xstart = 0
    OV5649MIPI_write_cmos_sensor(0x3801, 0x00); // xstart
    OV5649MIPI_write_cmos_sensor(0x3802, 0x00); // ystart = 0
    OV5649MIPI_write_cmos_sensor(0x3803, 0x00); // ystart
    OV5649MIPI_write_cmos_sensor(0x3804, 0x0a); // xend = 2623
    OV5649MIPI_write_cmos_sensor(0x3805, 0x3f); // xend
    OV5649MIPI_write_cmos_sensor(0x3806, 0x07); // yend = 1955
    OV5649MIPI_write_cmos_sensor(0x3807, 0xa3); // yend
    OV5649MIPI_write_cmos_sensor(0x3808, 0x0a); // x output size = 2592
    OV5649MIPI_write_cmos_sensor(0x3809, 0x20); // x output size
    OV5649MIPI_write_cmos_sensor(0x380a, 0x07); // y output size = 1944
    OV5649MIPI_write_cmos_sensor(0x380b, 0x98); // y output size
    OV5649MIPI_write_cmos_sensor(0x380c, 0x0b); // hts = 2816
    OV5649MIPI_write_cmos_sensor(0x380d, 0x00); // hts
    OV5649MIPI_write_cmos_sensor(0x380e, 0x07); // vts = 1984
    OV5649MIPI_write_cmos_sensor(0x380f, 0xc0); // vts
    
    OV5649MIPI_write_cmos_sensor(0x3810, 0x00); // isp x win = 16
    OV5649MIPI_write_cmos_sensor(0x3811, 0x10); // isp x win
    OV5649MIPI_write_cmos_sensor(0x3812, 0x00); // isp y win = 6
    OV5649MIPI_write_cmos_sensor(0x3813, 0x06); // isp y win
    OV5649MIPI_write_cmos_sensor(0x3814, 0x11); // x inc
    OV5649MIPI_write_cmos_sensor(0x3815, 0x11); // y inc
    OV5649MIPI_write_cmos_sensor(0x3817, 0x00); // hsync start
    OV5649MIPI_write_cmos_sensor(0x3820, 0x40); // flip off, v bin off
    OV5649MIPI_write_cmos_sensor(0x3821, 0x06); // mirror on, v bin off
    OV5649MIPI_write_cmos_sensor(0x4004, 0x04); // black line number
    OV5649MIPI_write_cmos_sensor(0x4005, 0x1a); // blc always update
    
    //OV5649MIPI_write_cmos_sensor(0x350b, 0x40); // gain = 4x
    OV5649MIPI_write_cmos_sensor(0x4837, 0x17); // MIPI global timing
   
    OV5649MIPI_write_cmos_sensor(0x0100, 0x01); //Stream On  
}   /*  OV5649MIPI_Capture_Setting  */


/*************************************************************************
* FUNCTION
*   OV5649MIPIOpen
*
* DESCRIPTION
*   This function initialize the registers of CMOS sensor
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5649MIPIOpen(void)
{
    kal_uint16 sensor_id = 0; 

    // check if sensor ID correct
    sensor_id=((OV5649MIPI_read_cmos_sensor(0x300A) << 8) | OV5649MIPI_read_cmos_sensor(0x300B));   
    SENSORDB("OV5649MIPIOpen sensor_id = 0x%x ", sensor_id);
    
    if (sensor_id != OV5648MIPI_SENSOR_ID){
        return ERROR_SENSOR_CONNECT_FAIL;
    }
    
    /* initail sequence write in  */
    OV5649MIPI_Sensor_Init();

    spin_lock(&ov5649mipi_drv_lock);
    OV5649MIPIAutoFlickerMode = KAL_FALSE;
    OV5649MIPI_sensor.ov5649mipi_sensor_mode = OV5649MIPI_SENSOR_MODE_INIT;
    OV5649MIPI_sensor.shutter = 0x3D0;
    OV5649MIPI_sensor.gain = 0x100;
    OV5649MIPI_sensor.pclk = OV5649MIPI_PREVIEW_CLK;
    OV5649MIPI_sensor.frame_length = OV5649MIPI_PV_PERIOD_LINE_NUMS;
    OV5649MIPI_sensor.line_length = OV5649MIPI_PV_PERIOD_PIXEL_NUMS;
    OV5649MIPI_sensor.dummy_pixel = 0;
    OV5649MIPI_sensor.dummy_line = 0;
    spin_unlock(&ov5649mipi_drv_lock);

    return ERROR_NONE;
}   /*  OV5649MIPIOpen  */


/*************************************************************************
* FUNCTION
*   OV5649GetSensorID
*
* DESCRIPTION
*   This function get the sensor ID 
*
* PARAMETERS
*   *sensorID : return the sensor ID 
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5649GetSensorID(UINT32 *sensorID) 
{
    signed char index;

    // check if sensor ID correct
    *sensorID=((OV5649MIPI_read_cmos_sensor(0x300A) << 8) | OV5649MIPI_read_cmos_sensor(0x300B));   
    printk("Sensor ID: 0x%x ", *sensorID);

    if (*sensorID == OV5648MIPI_SENSOR_ID) {    
        //check otp mid
        if (OV5649_OTP_combination_read()){
            printk("[OV5649GetSensorID],this is truly module.\n");
            *sensorID = OV5649MIPI_SENSOR_ID;
        }
        else{
            printk("[OV5649GetSensorID],this is sunny module.\n");
            *sensorID = 0xFFFFFFFF;
            return ERROR_SENSOR_CONNECT_FAIL;
        }
    }
        
    if (*sensorID != OV5649MIPI_SENSOR_ID) {
//Try again with invert PWD Pin config
	printk("OV5649GetSensorID Try again with PWD High \n");

	//mt_set_gpio_out(GPIO_CAMERA_CMPDN_PIN,GPIO_OUT_ONE);
	set_maincam_powerdown(GPIO_OUT_ONE);
	mdelay(5);
	*sensorID=((OV5649MIPI_read_cmos_sensor(0x300A) << 8) | OV5649MIPI_read_cmos_sensor(0x300B));	
	printk("OV5649MIPIOpen, sensor_id:%x \n",*sensorID);

        if (*sensorID == OV5648MIPI_SENSOR_ID) {    
            //check otp mid
            if (OV5649_OTP_combination_read()){
                printk("[OV5649GetSensorID],this is truly module.\n");
                *sensorID = OV5649MIPI_SENSOR_ID;
            }
            else{
                printk("[OV5649GetSensorID],this is sunny module.\n");
                *sensorID = 0xFFFFFFFF;
                return ERROR_SENSOR_CONNECT_FAIL;
            }
        }
		
	if (*sensorID == OV5649MIPI_SENSOR_ID)
	{
	    //kdCISModuleChangePinConfig(0,IDX_PS_CMPDN,GPIO_OUT_ONE);
	    return ERROR_NONE;
	}
//end

        // if Sensor ID is not correct, Must set *sensorID to 0xFFFFFFFF 
        *sensorID = 0xFFFFFFFF;
	return ERROR_SENSOR_CONNECT_FAIL;
    }
 
    return ERROR_NONE;
}

/*************************************************************************
* FUNCTION
*   OV5649MIPIClose
*
* DESCRIPTION
*   
*
* PARAMETERS
*   None
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5649MIPIClose(void)
{

    /*No Need to implement this function*/ 
    
    return ERROR_NONE;
}   /*  OV5649MIPIClose  */


/*************************************************************************
* FUNCTION
* OV5649MIPIPreview
*
* DESCRIPTION
*   This function start the sensor preview.
*
* PARAMETERS
*   *image_window : address pointer of pixel numbers in one period of HSYNC
*  *sensor_config_data : address pointer of line numbers in one period of VSYNC
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5649MIPIPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_pixel, dummy_line;

    SENSORDB("Enter!");
    
    OV5649MIPI_Preview_Setting();

    spin_lock(&ov5649mipi_drv_lock);
    OV5649MIPI_sensor.ov5649mipi_sensor_mode = OV5649MIPI_SENSOR_MODE_PREVIEW;
    OV5649MIPI_sensor.pclk = OV5649MIPI_PREVIEW_CLK;
    OV5649MIPI_sensor.video_mode = KAL_FALSE;
    spin_unlock(&ov5649mipi_drv_lock);

    //OV5649MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

    dummy_pixel = 0;
    dummy_line = 0;
    OV5649MIPI_Set_Dummy(dummy_pixel, dummy_line); /* modify dummy_pixel must gen AE table again */

    OV5649MIPI_Write_Shutter(OV5649MIPI_sensor.shutter);
    OV5649MIPI_SetGain(OV5649MIPI_sensor.gain);

    SENSORDB("Exit!");
    
    return ERROR_NONE;
}   /*  OV5649MIPIPreview   */

UINT32 OV5649MIPIZSDPreview(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                      MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_pixel, dummy_line;

    SENSORDB("Enter!");

    OV5649MIPI_Capture_Setting();

    spin_lock(&ov5649mipi_drv_lock);
    OV5649MIPI_sensor.ov5649mipi_sensor_mode = OV5649MIPI_SENSOR_MODE_CAPTURE;
    OV5649MIPI_sensor.pclk = OV5649MIPI_CAPTURE_CLK;
    OV5649MIPI_sensor.video_mode = KAL_FALSE;
    spin_unlock(&ov5649mipi_drv_lock);

    //OV5649MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

    dummy_pixel = 0;
    dummy_line = 0;   
    OV5649MIPI_Set_Dummy(dummy_pixel, dummy_line); /* modify dummy_pixel must gen AE table again */

    OV5649MIPI_Write_Shutter(OV5649MIPI_sensor.shutter);
    OV5649MIPI_SetGain(OV5649MIPI_sensor.gain);

    SENSORDB("Exit!");

    return ERROR_NONE;
   
}   /*  OV5649MIPIZSDPreview   */

/*************************************************************************
* FUNCTION
*   OV5649MIPICapture
*
* DESCRIPTION
*   This function setup the CMOS sensor in capture MY_OUTPUT mode
*
* PARAMETERS
*
* RETURNS
*   None
*
* GLOBALS AFFECTED
*
*************************************************************************/
UINT32 OV5649MIPICapture(MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *image_window,
                          MSDK_SENSOR_CONFIG_STRUCT *sensor_config_data)
{
    kal_uint16 dummy_pixel, dummy_line, cap_shutter, cap_gain;
    kal_uint16 pre_shutter = OV5649MIPI_sensor.shutter;
    kal_uint16 pre_gain = OV5649MIPI_sensor.gain;

    SENSORDB("Enter!");

    if(OV5649MIPI_SENSOR_MODE_PREVIEW == OV5649MIPI_sensor.ov5649mipi_sensor_mode)
    {
        OV5649MIPI_Capture_Setting();
    }
        
    spin_lock(&ov5649mipi_drv_lock);
    OV5649MIPI_sensor.ov5649mipi_sensor_mode = OV5649MIPI_SENSOR_MODE_CAPTURE;
    OV5649MIPI_sensor.pclk = OV5649MIPI_CAPTURE_CLK;
    OV5649MIPI_sensor.video_mode = KAL_FALSE;
    OV5649MIPIAutoFlickerMode = KAL_FALSE;    
    spin_unlock(&ov5649mipi_drv_lock);

    //OV5649MIPI_Set_Mirror_Flip(sensor_config_data->SensorImageMirror);

    dummy_pixel = 0;
    dummy_line = 0;               
    OV5649MIPI_Set_Dummy(dummy_pixel, dummy_line);

    // Keep the brightness of capture picture same as to preview
    cap_shutter = pre_shutter;
    cap_gain = pre_gain;
    OV5649MIPI_Write_Shutter(cap_shutter);
    OV5649MIPI_SetGain(cap_gain);

    SENSORDB("Exit!");
    
    return ERROR_NONE;
}   /* OV5649MIPI_Capture() */

UINT32 OV5649MIPIGetResolution(MSDK_SENSOR_RESOLUTION_INFO_STRUCT *pSensorResolution)
{
    pSensorResolution->SensorFullWidth = OV5649MIPI_IMAGE_SENSOR_FULL_WIDTH;
    pSensorResolution->SensorFullHeight = OV5649MIPI_IMAGE_SENSOR_FULL_HEIGHT;
    
    pSensorResolution->SensorPreviewWidth = OV5649MIPI_IMAGE_SENSOR_PV_WIDTH;
    pSensorResolution->SensorPreviewHeight = OV5649MIPI_IMAGE_SENSOR_PV_HEIGHT;
    
    return ERROR_NONE;
}   /*  OV5649MIPIGetResolution  */

UINT32 OV5649MIPIGetInfo(MSDK_SCENARIO_ID_ENUM ScenarioId,
                      MSDK_SENSOR_INFO_STRUCT *pSensorInfo,
                      MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
#ifdef OV5649MIPI_DRIVER_TRACE
    SENSORDB("ScenarioId = %d", ScenarioId);
#endif

    switch(ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            pSensorInfo->SensorPreviewResolutionX = OV5649MIPI_IMAGE_SENSOR_FULL_WIDTH; /* not use */
            pSensorInfo->SensorPreviewResolutionY = OV5649MIPI_IMAGE_SENSOR_FULL_HEIGHT; /* not use */
            pSensorInfo->SensorCameraPreviewFrameRate = 15; /* not use */
            break;

        default:
            pSensorInfo->SensorPreviewResolutionX = OV5649MIPI_IMAGE_SENSOR_PV_WIDTH; /* not use */
            pSensorInfo->SensorPreviewResolutionY = OV5649MIPI_IMAGE_SENSOR_PV_HEIGHT; /* not use */
            pSensorInfo->SensorCameraPreviewFrameRate = 30; /* not use */
            break;
    }

    pSensorInfo->SensorFullResolutionX = OV5649MIPI_IMAGE_SENSOR_FULL_WIDTH; /* not use */
    pSensorInfo->SensorFullResolutionY = OV5649MIPI_IMAGE_SENSOR_FULL_HEIGHT; /* not use */

    pSensorInfo->SensorVideoFrameRate = 30; /* not use */
	pSensorInfo->SensorStillCaptureFrameRate= 15; /* not use */
	pSensorInfo->SensorWebCamCaptureFrameRate= 15; /* not use */

    pSensorInfo->SensorClockPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorClockFallingPolarity = SENSOR_CLOCK_POLARITY_LOW; /* not use */
    pSensorInfo->SensorHsyncPolarity = SENSOR_CLOCK_POLARITY_LOW; // inverse with datasheet
    pSensorInfo->SensorVsyncPolarity = SENSOR_CLOCK_POLARITY_LOW;
    pSensorInfo->SensorInterruptDelayLines = 4; /* not use */
    pSensorInfo->SensorResetActiveHigh = FALSE; /* not use */
    pSensorInfo->SensorResetDelayCount = 5; /* not use */

    #ifdef MIPI_INTERFACE
        pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_MIPI;
    #else
        pSensorInfo->SensroInterfaceType = SENSOR_INTERFACE_TYPE_PARALLEL;
    #endif
    pSensorInfo->SensorOutputDataFormat = OV5649MIPI_COLOR_FORMAT;

    /* ISO BinningInfo  not use yet */
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_100_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_200_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_400_MODE].BinningEnable=FALSE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_800_MODE].BinningEnable=TRUE;

    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxWidth=CAM_SIZE_VGA_WIDTH;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].MaxHeight=CAM_SIZE_VGA_HEIGHT;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].ISOSupported=TRUE;
    pSensorInfo->SensorISOBinningInfo.ISOBinningInfo[ISO_1600_MODE].BinningEnable=TRUE;

    pSensorInfo->CaptureDelayFrame = 2; 
    pSensorInfo->PreviewDelayFrame = 2; 
    pSensorInfo->VideoDelayFrame = 2; 

    pSensorInfo->SensorMasterClockSwitch = 0; /* not use */
    pSensorInfo->SensorDrivingCurrent = ISP_DRIVING_6MA;
    pSensorInfo->SensorDriver3D = 0; /* not use */
    pSensorInfo->AEShutDelayFrame = 0;          /* The frame of setting shutter default 0 for TG int */
    pSensorInfo->AESensorGainDelayFrame = 0;    /* The frame of setting sensor gain */
    pSensorInfo->AEISPGainDelayFrame = 2;   

    switch (ScenarioId)
    {
	    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
	    case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
	    case MSDK_SCENARIO_ID_CAMERA_ZSD:
			pSensorInfo->SensorClockFreq = 24;
			pSensorInfo->SensorClockDividCount = 3; /* not use */
			pSensorInfo->SensorClockRisingCount = 0;
			pSensorInfo->SensorClockFallingCount = 2; /* not use */
			pSensorInfo->SensorPixelClockCount = 3; /* not use */
			pSensorInfo->SensorDataLatchCount = 2; /* not use */
	        pSensorInfo->SensorGrabStartX = OV5649MIPI_FULL_START_X; 
	        pSensorInfo->SensorGrabStartY = OV5649MIPI_FULL_START_Y;

            #ifdef MIPI_INTERFACE
                pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;         
                pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
                pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
                pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
                pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
                pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
                pSensorInfo->SensorPacketECCOrder = 1;
            #endif

	        break;
	    default:
	        pSensorInfo->SensorClockFreq = 24;
			pSensorInfo->SensorClockDividCount = 3; /* not use */
			pSensorInfo->SensorClockRisingCount = 0;
			pSensorInfo->SensorClockFallingCount = 2; /* not use */
			pSensorInfo->SensorPixelClockCount = 3; /* not use */
			pSensorInfo->SensorDataLatchCount = 2; /* not use */
	        pSensorInfo->SensorGrabStartX = OV5649MIPI_PV_START_X; 
	        pSensorInfo->SensorGrabStartY = OV5649MIPI_PV_START_Y;

            #ifdef MIPI_INTERFACE
                pSensorInfo->SensorMIPILaneNumber = SENSOR_MIPI_2_LANE;         
                pSensorInfo->MIPIDataLowPwr2HighSpeedTermDelayCount = 0; 
                pSensorInfo->MIPIDataLowPwr2HighSpeedSettleDelayCount = 14; 
                pSensorInfo->MIPICLKLowPwr2HighSpeedTermDelayCount = 0;
                pSensorInfo->SensorWidthSampling = 0;  // 0 is default 1x
                pSensorInfo->SensorHightSampling = 0;   // 0 is default 1x 
                pSensorInfo->SensorPacketECCOrder = 1;
            #endif

	        break;
    }
	
	return ERROR_NONE;
}   /*  OV5649MIPIGetInfo  */


UINT32 OV5649MIPIControl(MSDK_SCENARIO_ID_ENUM ScenarioId, MSDK_SENSOR_EXPOSURE_WINDOW_STRUCT *pImageWindow,
                      MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData)
{
    SENSORDB("ScenarioId = %d", ScenarioId);

    OV5649_mCurrentScenarioId =ScenarioId;
    
    switch (ScenarioId)
    {
        case MSDK_SCENARIO_ID_CAMERA_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_PREVIEW:
        case MSDK_SCENARIO_ID_VIDEO_CAPTURE_MPEG4:
            OV5649MIPIPreview(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
        case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
            OV5649MIPICapture(pImageWindow, pSensorConfigData);
            break;
        case MSDK_SCENARIO_ID_CAMERA_ZSD:
            OV5649MIPIZSDPreview(pImageWindow, pSensorConfigData);
            break;      
        default:
            SENSORDB("Error ScenarioId setting");
            return ERROR_INVALID_SCENARIO_ID;
    }

    return ERROR_NONE;
}   /* OV5649MIPIControl() */



UINT32 OV5649MIPISetVideoMode(UINT16 u2FrameRate)
{
    SENSORDB("u2FrameRate = %d ", u2FrameRate);

    // SetVideoMode Function should fix framerate
    if(u2FrameRate < 5){
        // Dynamic frame rate
        return ERROR_NONE;
    }

    spin_lock(&ov5649mipi_drv_lock);
    OV5649MIPI_sensor.video_mode = KAL_TRUE;
    spin_unlock(&ov5649mipi_drv_lock);

    if(u2FrameRate == 30){
        spin_lock(&ov5649mipi_drv_lock);
        OV5649MIPI_sensor.NightMode = KAL_FALSE;
        spin_unlock(&ov5649mipi_drv_lock);
    }else if(u2FrameRate == 15){
        spin_lock(&ov5649mipi_drv_lock);
        OV5649MIPI_sensor.NightMode = KAL_TRUE;
        spin_unlock(&ov5649mipi_drv_lock);
    }else{
        // fixed other frame rate
    }

    spin_lock(&ov5649mipi_drv_lock);
    OV5649MIPI_sensor.FixedFps = u2FrameRate;
    spin_unlock(&ov5649mipi_drv_lock);

    if((u2FrameRate == 30)&&(OV5649MIPIAutoFlickerMode == KAL_TRUE))
        u2FrameRate = 296;
    else if ((u2FrameRate == 15)&&(OV5649MIPIAutoFlickerMode == KAL_TRUE))
        u2FrameRate = 146;
    else
        u2FrameRate = 10 * u2FrameRate;
    
    OV5649MIPISetMaxFrameRate(u2FrameRate);
    OV5649MIPI_Write_Shutter(OV5649MIPI_sensor.shutter);//From Meimei Video issue

    return ERROR_NONE;
}

UINT32 OV5649MIPISetAutoFlickerMode(kal_bool bEnable, UINT16 u2FrameRate)
{
    SENSORDB("bEnable = %d, u2FrameRate = %d ", bEnable, u2FrameRate);

    if(bEnable){
        spin_lock(&ov5649mipi_drv_lock);
        OV5649MIPIAutoFlickerMode = KAL_TRUE;
        spin_unlock(&ov5649mipi_drv_lock);

        /*Change frame rate 29.5fps to 29.8fps to do Auto flick*/
        if((OV5649MIPI_sensor.FixedFps == 30)&&(OV5649MIPI_sensor.video_mode==KAL_TRUE))
            OV5649MIPISetMaxFrameRate(296);
        else if((OV5649MIPI_sensor.FixedFps == 15)&&(OV5649MIPI_sensor.video_mode==KAL_TRUE))
            OV5649MIPISetMaxFrameRate(148);
        
    }else{ //Cancel Auto flick
        spin_lock(&ov5649mipi_drv_lock);
        OV5649MIPIAutoFlickerMode = KAL_FALSE;
        spin_unlock(&ov5649mipi_drv_lock);
        
        if((OV5649MIPI_sensor.FixedFps == 30)&&(OV5649MIPI_sensor.video_mode==KAL_TRUE))
            OV5649MIPISetMaxFrameRate(300);
        else if((OV5649MIPI_sensor.FixedFps == 15)&&(OV5649MIPI_sensor.video_mode==KAL_TRUE))
            OV5649MIPISetMaxFrameRate(150);            
    }

    return ERROR_NONE;
}
UINT32 OV5649MIPIFeatureControl(MSDK_SENSOR_FEATURE_ENUM FeatureId,
                             UINT8 *pFeaturePara,UINT32 *pFeatureParaLen)
{
    UINT16 *pFeatureReturnPara16=(UINT16 *) pFeaturePara;
    UINT16 *pFeatureData16=(UINT16 *) pFeaturePara;
    UINT32 *pFeatureReturnPara32=(UINT32 *) pFeaturePara;
    UINT32 *pFeatureData32=(UINT32 *) pFeaturePara;
    UINT32 OV5649MIPISensorRegNumber;
    UINT32 i;
    PNVRAM_SENSOR_DATA_STRUCT pSensorDefaultData=(PNVRAM_SENSOR_DATA_STRUCT) pFeaturePara;
    MSDK_SENSOR_CONFIG_STRUCT *pSensorConfigData=(MSDK_SENSOR_CONFIG_STRUCT *) pFeaturePara;
    MSDK_SENSOR_REG_INFO_STRUCT *pSensorRegData=(MSDK_SENSOR_REG_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_GROUP_INFO_STRUCT *pSensorGroupInfo=(MSDK_SENSOR_GROUP_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ITEM_INFO_STRUCT *pSensorItemInfo=(MSDK_SENSOR_ITEM_INFO_STRUCT *) pFeaturePara;
    MSDK_SENSOR_ENG_INFO_STRUCT *pSensorEngInfo=(MSDK_SENSOR_ENG_INFO_STRUCT *) pFeaturePara;

    #ifdef OV5649MIPI_DRIVER_TRACE
        //SENSORDB("FeatureId = %d", FeatureId);
    #endif

    switch (FeatureId)
    {
        case SENSOR_FEATURE_GET_RESOLUTION:
            *pFeatureReturnPara16++=OV5649MIPI_IMAGE_SENSOR_FULL_WIDTH;
            *pFeatureReturnPara16=OV5649MIPI_IMAGE_SENSOR_FULL_HEIGHT;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_GET_PERIOD:
            switch(OV5649_mCurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
                case MSDK_SCENARIO_ID_CAMERA_ZSD:
                    *pFeatureReturnPara16++ = OV5649MIPI_FULL_PERIOD_PIXEL_NUMS;
                    *pFeatureReturnPara16 = OV5649MIPI_sensor.frame_length;
                    *pFeatureParaLen=4;
                    break;
                default:
                    *pFeatureReturnPara16++ = OV5649MIPI_PV_PERIOD_PIXEL_NUMS;
                    *pFeatureReturnPara16 = OV5649MIPI_sensor.frame_length;
                    *pFeatureParaLen=4;
                    break;
            }
            break;
        case SENSOR_FEATURE_GET_PIXEL_CLOCK_FREQ:
            switch(OV5649_mCurrentScenarioId)
            {
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_JPEG:
                case MSDK_SCENARIO_ID_CAMERA_CAPTURE_MEM:
                case MSDK_SCENARIO_ID_CAMERA_ZSD: 
                    *pFeatureReturnPara32 = OV5649MIPI_CAPTURE_CLK;
                    *pFeatureParaLen=4;
                    break;
                default:
                    *pFeatureReturnPara32 = OV5649MIPI_PREVIEW_CLK;
                    *pFeatureParaLen=4;
                    break;
            }
            break;
        case SENSOR_FEATURE_SET_ESHUTTER:
            set_OV5649MIPI_shutter(*pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_NIGHTMODE:
            OV5649MIPI_night_mode((BOOL) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_GAIN:       
            OV5649MIPI_SetGain((UINT16) *pFeatureData16);
            break;
        case SENSOR_FEATURE_SET_FLASHLIGHT:
            break;
        case SENSOR_FEATURE_SET_ISP_MASTER_CLOCK_FREQ:
            break;
        case SENSOR_FEATURE_SET_REGISTER:
            OV5649MIPI_write_cmos_sensor(pSensorRegData->RegAddr, pSensorRegData->RegData);
            break;
        case SENSOR_FEATURE_GET_REGISTER:
            pSensorRegData->RegData = OV5649MIPI_read_cmos_sensor(pSensorRegData->RegAddr);
            break;
        case SENSOR_FEATURE_SET_CCT_REGISTER:
            memcpy(&OV5649MIPI_sensor.eng.cct, pFeaturePara, sizeof(OV5649MIPI_sensor.eng.cct));
            break;
            break;
        case SENSOR_FEATURE_GET_CCT_REGISTER:
            if (*pFeatureParaLen >= sizeof(OV5649MIPI_sensor.eng.cct) + sizeof(kal_uint32))
            {
              *((kal_uint32 *)pFeaturePara++) = sizeof(OV5649MIPI_sensor.eng.cct);
              memcpy(pFeaturePara, &OV5649MIPI_sensor.eng.cct, sizeof(OV5649MIPI_sensor.eng.cct));
            }
            break;
        case SENSOR_FEATURE_SET_ENG_REGISTER:
            memcpy(&OV5649MIPI_sensor.eng.reg, pFeaturePara, sizeof(OV5649MIPI_sensor.eng.reg));
            break;
        case SENSOR_FEATURE_GET_ENG_REGISTER:
            if (*pFeatureParaLen >= sizeof(OV5649MIPI_sensor.eng.reg) + sizeof(kal_uint32))
            {
              *((kal_uint32 *)pFeaturePara++) = sizeof(OV5649MIPI_sensor.eng.reg);
              memcpy(pFeaturePara, &OV5649MIPI_sensor.eng.reg, sizeof(OV5649MIPI_sensor.eng.reg));
            }
        case SENSOR_FEATURE_GET_REGISTER_DEFAULT:
            ((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->Version = NVRAM_CAMERA_SENSOR_FILE_VERSION;
            ((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorId = OV5649MIPI_SENSOR_ID;
            memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorEngReg, &OV5649MIPI_sensor.eng.reg, sizeof(OV5649MIPI_sensor.eng.reg));
            memcpy(((PNVRAM_SENSOR_DATA_STRUCT)pFeaturePara)->SensorCCTReg, &OV5649MIPI_sensor.eng.cct, sizeof(OV5649MIPI_sensor.eng.cct));
            *pFeatureParaLen = sizeof(NVRAM_SENSOR_DATA_STRUCT);
            break;
        case SENSOR_FEATURE_GET_CONFIG_PARA:
            memcpy(pFeaturePara, &OV5649MIPI_sensor.cfg_data, sizeof(OV5649MIPI_sensor.cfg_data));
            *pFeatureParaLen = sizeof(OV5649MIPI_sensor.cfg_data);
            break;
        case SENSOR_FEATURE_CAMERA_PARA_TO_SENSOR:
             OV5649MIPI_camera_para_to_sensor();
            break;
        case SENSOR_FEATURE_SENSOR_TO_CAMERA_PARA:
            OV5649MIPI_sensor_to_camera_para();
            break;                          
        case SENSOR_FEATURE_GET_GROUP_COUNT:
            OV5649MIPI_get_sensor_group_count((kal_uint32 *)pFeaturePara);
            *pFeatureParaLen = 4;
            break;    
        case SENSOR_FEATURE_GET_GROUP_INFO:
            OV5649MIPI_get_sensor_group_info((MSDK_SENSOR_GROUP_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_GROUP_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ITEM_INFO:
            OV5649MIPI_get_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_SET_ITEM_INFO:
            OV5649MIPI_set_sensor_item_info((MSDK_SENSOR_ITEM_INFO_STRUCT *)pFeaturePara);
            *pFeatureParaLen = sizeof(MSDK_SENSOR_ITEM_INFO_STRUCT);
            break;
        case SENSOR_FEATURE_GET_ENG_INFO:
            memcpy(pFeaturePara, &OV5649MIPI_sensor.eng_info, sizeof(OV5649MIPI_sensor.eng_info));
            *pFeatureParaLen = sizeof(OV5649MIPI_sensor.eng_info);
            break;
        case SENSOR_FEATURE_GET_LENS_DRIVER_ID:
            // get the lens driver ID from EEPROM or just return LENS_DRIVER_ID_DO_NOT_CARE
            // if EEPROM does not exist in camera module.
            *pFeatureReturnPara32=LENS_DRIVER_ID_DO_NOT_CARE;
            *pFeatureParaLen=4;
            break;
        case SENSOR_FEATURE_SET_VIDEO_MODE:
            OV5649MIPISetVideoMode(*pFeatureData16);
            break; 
        case SENSOR_FEATURE_CHECK_SENSOR_ID:
            OV5649GetSensorID(pFeatureReturnPara32); 
            break; 
        case SENSOR_FEATURE_SET_AUTO_FLICKER_MODE:
            OV5649MIPISetAutoFlickerMode((BOOL)*pFeatureData16,*(pFeatureData16+1));
            break;
        default:
            break;
    }
  
    return ERROR_NONE;
}   /*  OV5649MIPIFeatureControl()  */
SENSOR_FUNCTION_STRUCT  SensorFuncOV5649MIPI=
{
    OV5649MIPIOpen,
    OV5649MIPIGetInfo,
    OV5649MIPIGetResolution,
    OV5649MIPIFeatureControl,
    OV5649MIPIControl,
    OV5649MIPIClose
};

UINT32 OV5649MIPISensorInit(PSENSOR_FUNCTION_STRUCT *pfFunc)
{
    /* To Do : Check Sensor status here */
    if (pfFunc!=NULL)
        *pfFunc=&SensorFuncOV5649MIPI;

    return ERROR_NONE;
}   /*  OV5649MIPISensorInit  */
