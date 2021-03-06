
#include <system.h>
#include <drv.h>
#include <alg.h>
#include <mcvip.h>

int SYSTEM_init(int debug)
{
    int status;
    int devAddr[MCVIP_VIDEO_INPUT_PORT_MAX *MCVIP_TVP5158_MAX_CASCADE];

    #if DEBUG==1
    printf("SYSTEM_init debug=%d, MCVIP_VIDEO_INPUT_PORT_MAX=%d, MCVIP_TVP5158_MAX_CASCADE=%d\n",debug,MCVIP_VIDEO_INPUT_PORT_MAX ,MCVIP_TVP5158_MAX_CASCADE);
    #endif

    status = DRV_init();
    if (status < 0)
    {
        OSA_ERROR("DRV_init()\n");
        return status;
    }

    memset(devAddr, 0, sizeof(devAddr));

    if (!debug)
    {
        // control TVP5158 I2C via software
        devAddr[0] = TVP5158_A_I2C_ADDR;
        //add by sxh
        //devAddr[1] = TVP5158_B_I2C_ADDR;
        //devAddr[2] = TVP5158_A_I2C_ADDR;
        //devAddr[3] = TVP5158_B_I2C_ADDR;

    }
    else
    {
        // debug mode, let WinVCC tool control TVP5158 I2C
    }


    status = MCVIP_init(devAddr);
    if (status < 0)
    {
        OSA_ERROR("MCVIP_init()\n");
        DRV_exit();
        return status;
    }

    status = ALG_sysInit();
    if (status != OSA_SOK)
    {
        OSA_ERROR("ALG_sysInit()\n");
        MCVIP_exit();
        DRV_exit();
        return status;
    }

    return status;
}

int SYSTEM_exit()
{
    MCVIP_exit();
    ALG_sysExit();
    DRV_exit();

    return OSA_SOK;
}

char SYSTEM_getInput()
{
    return getchar();
}
