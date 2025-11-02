/*
*********************************************************************************************************
*                                            EXAMPLE CODE
*
*               This file is provided as an example on how to use Micrium products.
*
*               Please feel free to use any application code labeled as 'EXAMPLE CODE' in
*               your application products.  Example code may be used as is, in whole or in
*               part, or may be used as a reference only. This file can be modified as
*               required to meet the end-product requirements.
*
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*
*               You can find our product's user manual, API reference, release notes and
*               more information at https://doc.micrium.com.
*               You can contact us at www.micrium.com.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                              uC/OS-II
*                                            EXAMPLE CODE
*
* Filename : main.c
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                            INCLUDE FILES
*********************************************************************************************************
*/

#include  <cpu.h>
#include  <lib_mem.h>
#include  <os.h>

#include  "app_cfg.h"
#define TASK_STACKSIZE 2048


/*
*********************************************************************************************************
*                                            LOCAL DEFINES
*********************************************************************************************************
*/


/*
*********************************************************************************************************
*                                       LOCAL GLOBAL VARIABLES
*********************************************************************************************************
*/

static  OS_STK  StartupTaskStk[APP_CFG_STARTUP_TASK_STK_SIZE];


/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  StartupTask (void  *p_arg);


/*
*********************************************************************************************************
*                                                main()
*
* Description : This is the standard entry point for C code.  It is assumed that your code will call
*               main() once you have performed all necessary initialization.
*
* Arguments   : none
*
* Returns     : none
*
* Notes       : none
*********************************************************************************************************
*/

static void task(void* p_arg);

void task(void* p_arg) {
    task_para_set* task_data;
    INT32U        next_release;
    INT32U        time_tag;

    task_data      = p_arg;
    next_release   = task_data->TaskArriveTime;
    task_data->TaskRemainTime = task_data->TaskExecutionTIme;
    time_tag       = OSTimeGet();

    while (1) {
        INT32U now = OSTimeGet();

        if (now < next_release) {
            INT32U wait_ticks = next_release - now;
            INT16U sleep      = (wait_ticks > (INT32U)0xFFFFu) ? (INT16U)0xFFFFu : (INT16U)wait_ticks;

            if (sleep > 0u) {
                OSTimeDly(sleep);
            }
            continue;
        }

        if (task_data->TaskRemainTime == 0u) {
            task_data->TaskRemainTime = task_data->TaskExecutionTIme;
        }

        INT32U run_tick = OSTimeGet();
        time_tag                = run_tick;
        task_data->TaskStartTime = (INT16U)run_tick;

        printf("%2d task(%2d) is running\n", run_tick, task_data->TaskID);
        if ((Output_err = fopen_s(&Output_fp, "./Output.txt", "a")) == 0)
        {
            fprintf(Output_fp, "%2d task(%2d) is running\n", run_tick, task_data->TaskID);
            fclose(Output_fp);
        }

        while (task_data->TaskRemainTime > 0u) {
            INT32U now_tick = OSTimeGet();
  
            if (now_tick == time_tag) {
                OSTimeDly(1u);
                continue;
            }

            INT32U elapsed = now_tick - time_tag;

            if (elapsed > task_data->TaskRemainTime) {
                elapsed = task_data->TaskRemainTime;
            }

            task_data->TaskRemainTime -= elapsed;
            time_tag                   = now_tick;
            task_data->TaskStartTime   = (INT16U)now_tick;
        }

        task_data->TaskCount += 1u;

        if (task_data->TaskPeriodic != 0u) {
            next_release = task_data->TaskArriveTime + (task_data->TaskCount * task_data->TaskPeriodic);
        } else {
            next_release = OSTimeGet() + 1u;
        }

        OSTimeDly(1u);
    }
}

int  main (void)
{
#if OS_TASK_NAME_EN > 0u
    CPU_INT08U  os_err;
#endif


    CPU_IntInit();

    Mem_Init();                                                 /* Initialize Memory Managment Module                   */
    CPU_IntDis();                                               /* Disable all Interrupts                               */
    CPU_Init();                                                 /* Initialize the uC/CPU services                       */

    OSInit();                                                   /* Initialize uC/OS-II                                  */


    OutFileInit();
    InputFile();

    Task_STK = malloc(TASK_NUMBER * sizeof(int*));

    for (int n = 0; n < TASK_NUMBER; n++) {
        Task_STK[n] = malloc(TASK_STACKSIZE * sizeof(int));
        OSTaskCreateExt(task,
            &TaskParameter[n],
            &Task_STK[n][TASK_STACKSIZE - 1],
            TaskParameter[n].TaskID, //modefied
            TaskParameter[n].TaskID,
            &Task_STK[n][0],
            TASK_STACKSIZE,
            &TaskParameter[n],
            (OS_TASK_OPT_STK_CHK | OS_TASK_OPT_STK_CLR));
        TaskParameter[n].TaskRemainTime = TaskParameter[n].TaskExecutionTIme;
    }

    printf("==========TCB linked list==========\n");
    printf("Task \t Prev_TCB_addr   TCB_addr   Next_TCB_addr\n");
    for (int i = OS_MAX_TASKS + OS_N_SYS_TASKS;i >= 0;i--) if (OSTCBTbl[i].OSTCBPrio != 0) printf("%2d \t %6x \t %6x \t %6x\n", OSTCBTbl[i].OSTCBPrio, OSTCBTbl[i].OSTCBPrev, &OSTCBTbl[i], OSTCBTbl[i].OSTCBNext);
    printf("\n");

#if OS_TASK_NAME_EN > 0u
    OSTaskNameSet(         APP_CFG_STARTUP_TASK_PRIO,
                  (INT8U *)"Startup Task",
                           &os_err);
#endif
    OSStart();                                                  /* Start multitasking (i.e. give control to uC/OS-II)   */

    while (DEF_ON) {                                            /* Should Never Get Here.                               */
        ;
    }
}


/*
*********************************************************************************************************
*                                            STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'StartupTask()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  StartupTask (void *p_arg)
{
   (void)p_arg;

    OS_TRACE_INIT();                                            /* Initialize the uC/OS-II Trace recorder               */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                               /* Compute CPU capacity with no task running            */
#endif

#ifdef CPU_CFG_INT_DIS_MEAS_EN
    CPU_IntDisMeasMaxCurReset();
#endif
    
    APP_TRACE_DBG(("uCOS-III is Running...\n\r"));

    while (DEF_TRUE) {                                          /* Task body, always written as an infinite loop.       */
        OSTimeDlyHMSM(0u, 0u, 1u, 0u);
		APP_TRACE_DBG(("Time: %d\n\r", OSTimeGet()));
    }
}

