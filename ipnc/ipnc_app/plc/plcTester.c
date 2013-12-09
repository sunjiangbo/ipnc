
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <Msg_Def.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <share_mem.h>
#include <pthread.h>
#include <file_msg_drv.h>
#include <alarm_msg_drv.h>
#include <signal.h>

#include <sys_env_type.h>
#include <system_default.h>
#include <sys_info_default.h>

static int mid;
static int mFileId;
static void *pFileShareMem;

/**
 * @brief Initialize message queue.

 * Initialize message queue.
 * @note This API must be used before use any other message driver API.
 * @param msgKey [I ] Key number for message queue and share memory.
 * @return message ID.
 */

int Msg_Init(int msgKey)
{
    int qid;
    key_t key = msgKey;

    qid = msgget(key, 0);

    if (qid < 0)
    {
        qid = msgget(key, IPC_CREAT | 0666);

        #if DEBUG==0
        printf("Creat queue id:%d\n", qid);
        #endif
    }

    #if DEBUG==0
    printf("queue id:%d\n", qid);
    #endif

    return qid;
}

/**
 * @brief Initialize pshared memory driver.

 * Initialize pshared momory driver.
 * @note The memory ID isn't saved to global variable.
 * @return Memory ID
 * @retval -1 Fail to initialize shared memory.
 */
int pShareMemInit(int key)
{
    mid = shmget(key, 0, 0);

    if (mid < 0)
    {
        mid = shmget(key, PROC_MEM_SIZE *MAX_SHARE_PROC, IPC_CREAT | 0660);

        #if DEBUG==0
        printf("Create shared memory id:%d\n", mid);
        #endif
    }

    #if DEBUG==0
    printf("shared memory size %d\n", PROC_MEM_SIZE *MAX_SHARE_PROC);
    #endif

    #if DEBUG==0
    printf("shared memory id:%d\n", mid);
    #endif

    return mid;
}

/**
 * @brief    check magic number
 * @param    fp [I ]file pointer
 * @return    error code : SUCCESS(0) or FAIL(-1)
 */
int check_magic_num(FILE *fp)
{
    int ret;
    unsigned long MagicNum;
    if (fread(&MagicNum, 1, sizeof(MagicNum), fp) != sizeof(MagicNum))
    {
        ret = FAIL;
    }
    else
    {
        if (MagicNum == MAGIC_NUM)
        {
            ret = SUCCESS;
        }
        else
        {
            ret = FAIL;
        }
    }
    return ret;
}

/**
 * @brief    read SysInfo from system file
 * @param    "void *Buffer" : [OUT]buffer to store SysInfo
 * @return    error code : SUCCESS(0) or FAIL(-1)
 */
int ReadGlobal(void *Buffer)
{
    FILE *fp;
    int ret;

    if ((fp = fopen(SYS_FILE, "rb")) == NULL)
    {
        //printf("ReadGlobal ret == FAIL)\n");
        /* System file not exist */
        ret = FAIL;
    }
    else
    {
        if (check_magic_num(fp) == SUCCESS)
        {
            if (fread(Buffer, 1, SYS_ENV_SIZE, fp) != SYS_ENV_SIZE)
            {
                ret = FAIL;
            }
            else
            {
                ret = SUCCESS;
            }
        }
        else
        {
            ret = FAIL;
        }

        fclose(fp);
    }
    return ret;
}

/**
 * @brief    create system file
 * @param    name [I ]File name to create in nand.
 * @param    Global [I ]Pointer to System information
 * @return    error code : SUCCESS(0) or FAIL(-1)
 */
int create_sys_file(char *name, void *Global)
{
    FILE *fp;
    int ret;
    unsigned long MagicNum = MAGIC_NUM;

    if ((fp = fopen(name, "wb")) == NULL)
    {
        perror("Can't create system file\n");
        ret = FAIL;
    }
    else
    {
        if (fwrite(&MagicNum, 1, sizeof(MagicNum), fp) != sizeof(MagicNum))
        {
            perror("Writing Magic Number fail\n");
            ret = FAIL;
        }
        else
        {
            if (fwrite(Global, 1, SYS_ENV_SIZE, fp) != SYS_ENV_SIZE)
            {
                perror("Writing global fail\n");
                ret = FAIL;
            }
            else
            {
                ret = SUCCESS;
            }
        }
        fclose(fp);
    }
    return ret;
}

/**
 * @brief    file manager initialization
 * @param    ShareMem [O ]Pointer to share memory where system information will
be stored.
 * @return    error code : SUCCESS(0) or FAIL(-1)
 */
int FileMngInit(void *ShareMem)
{
    int ret;

    #if DEBUG==0
    printf("FileMngInit\n");
    #endif

    #if DEBUG==0
    printf("Global value size:%d\n", SYS_ENV_SIZE);
    #endif

    ret = ReadGlobal(ShareMem);
    //printf("FileMngInit ret == %d)\n",FAIL);
    if (ret == FAIL)
    {
        printf("FileMngInit ret == FAIL)\n");
        ret = create_sys_file(SYS_FILE, &SysInfoDefault);
        printf("create_sys_file Done!!!\n");
        if (ret == SUCCESS)
        {
            printf("create_sys_file ret == SUCCESS!!!\n");
            memcpy(ShareMem, &SysInfoDefault, SYS_ENV_SIZE);
        }
    }

    return ret;
}

static char *HelpMsg =
{
    "\n""?: Print Help Message.\n"
    "bye: Exit plcTester\n"
    "start: start Plc Progress\n"
    "quit: Quit Plc Progress\n"
    "restart: Restart Plc Progress\n""\n"
};

int main(int argc, char **argv)
{
    plcInit();

    mFileId = pShareMemInit(FILE_MSG_KEY);
    if (mFileId < 0)
    {
        return  - 1;
    }

    #if DEBUG==0
    printf("%s:mFileId=%d\n", __func__, mFileId);
    #endif

    pFileShareMem = shmat(mFileId, 0, 0);

    #if DEBUG==0
    printf("%s:Share Memory Addr=%p\n", __func__, pFileShareMem);
    #endif

    if (FileMngInit(pFileShareMem) != 0)
    {
        return  - 1;
    }

    #if DEBUG==0
    {
        SysInfo *sysInfoP;

        sysInfoP = pFileShareMem;

        printf("admin user:=%s, passwd=%s,authority=%d\n", sysInfoP->acounts[0].user, sysInfoP->acounts[0].password, sysInfoP->acounts[0].authority);
    }
    #endif

    while (1)
    {
        char buff[512];

        printf("plcTester>");

        fgets(buff, sizeof(buff), stdin);

        #if DEBUG==0
        {
            int i;

            printf("buff: ");
            for (i = 0; i < strlen(buff); i++)
            {
                printf("%02x ", buff[i]);
            }
            printf("\n");
        }
        #endif

        if(strcmp("bye\n",buff)==0)
        {
            break;
        }
        else if (strcmp("start\n", buff) == 0)
        {
            SendPlcStartCmd();
        }
        else if (strcmp("quit\n", buff) == 0)
        {
            SendPlcQuitCmd();
        }
        else if (strcmp("restart\n", buff) == 0)
        {
            SendPlcRestartCmd();
        }
        else if (strcmp("?", buff) == 0 || strlen(buff) > 0)
        {
            printf(HelpMsg);
        }
    }

    create_sys_file(SYS_FILE, &SysInfoDefault);

    return 0;
}