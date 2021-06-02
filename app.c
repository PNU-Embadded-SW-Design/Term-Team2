/*
*********************************************************************************************************
*                                              EXAMPLE CODE
*
*                             (c) Copyright 2009; Micrium, Inc.; Weston, FL
*
*               All rights reserved.  Protected by international copyright laws.
*               Knowledge of the source code may NOT be used to develop a similar product.
*               Please help us continue to provide the Embedded community with the finest
*               software available.  Your honesty is greatly appreciated.
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*
*                                            EXAMPLE CODE
*
*                                     ST Microelectronics STM32
*                                              on the
*
*                                     Micrium uC-Eval-STM32F107
*                                        Evaluation Board
*
* Filename      : app.c
* Version       : V1.00
* Programmer(s) : JJL
                  EHS
*********************************************************************************************************
*/

/*
*********************************************************************************************************
*                                             INCLUDE FILES
*********************************************************************************************************
*/

#include <includes.h>
#include <lcd.h>
#include <touch.h>

/*
*********************************************************************************************************
*                                             LOCAL DEFINES
*********************************************************************************************************
*/
#define X0 120
#define Y0 155
#define X_START 10
#define Y_START 10
#define X_END 230
#define Y_END 300

#define DEV_COORDINATE

/*
*********************************************************************************************************
*                                            LOCAL VARIABLES
*********************************************************************************************************
*/

static  OS_SEM   AppSem; 

static  OS_TCB   AppTaskStartTCB; 
static  OS_TCB   ADCTaskTCB; 


static  CPU_STK  AppTaskStartStk[APP_TASK_START_STK_SIZE];
static  CPU_STK  ADCTaskStartStk[APP_TASK_START_STK_SIZE];


uint32_t ADC_value = 0;

#ifdef DEV_COORDINATE

#define PI 3.14159265 
static  OS_TCB   RadTaskTCB; 
static  CPU_STK  RadTaskStartStk[APP_TASK_START_STK_SIZE];
static uint32_t degree;
static void Rad_task(void* p_arg);
#endif

/*
*********************************************************************************************************
*                                         FUNCTION PROTOTYPES
*********************************************************************************************************
*/

static  void  AppTaskStart  (void *p_arg);
static  void  ADC_Task      (void* p_arg);


void LED_ON(u16 GPIO_Pin);
void LED_OFF(u16 GPIO_Pin);
void GPIO_Configure();
void LCD_Display();
void two_dimensional_coord();

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
*********************************************************************************************************
*/

int  main (void)
{
    OS_ERR  err;


    BSP_IntDisAll();                                            /* Disable all interrupts.                              */

    OSInit(&err);                                               /* Init uC/OS-III.                                      */

    OSTaskCreate((OS_TCB     *)&AppTaskStartTCB,                /* Create the start task                                */
                 (CPU_CHAR   *)"App Task Start",
                 (OS_TASK_PTR )AppTaskStart, 
                 (void       *)0,
                 (OS_PRIO     )APP_TASK_START_PRIO,
                 (CPU_STK    *)&AppTaskStartStk[0],
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE / 10,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);

    OSStart(&err);                                              /* Start multitasking (i.e. give control to uC/OS-III). */
}


/*
*********************************************************************************************************
*                                          STARTUP TASK
*
* Description : This is an example of a startup task.  As mentioned in the book's text, you MUST
*               initialize the ticker only once multitasking has started.
*
* Arguments   : p_arg   is the argument passed to 'AppTaskStart()' by 'OSTaskCreate()'.
*
* Returns     : none
*
* Notes       : 1) The first line of code is used to prevent a compiler warning because 'p_arg' is not
*                  used.  The compiler should not generate any code for this statement.
*********************************************************************************************************
*/

static  void  AppTaskStart (void *p_arg)
{
    CPU_INT32U  cpu_clk_freq;
    CPU_INT32U  cnts;
    OS_ERR      err;
    CPU_TS  ts;
    

   (void)p_arg;

    OSSemCreate(&AppSem, "Test Sem", 0, &err);

    BSP_Init();                                                   /* Initialize BSP functions                         */
    CPU_Init();                                                   /* Initialize the uC/CPU services                   */
    LCD_Init();
    LCD_Clear(WHITE);
    two_dimensional_coord();
    GPIO_Configure();
    BSP_Infrared_Init();
    
    cpu_clk_freq = BSP_CPU_ClkFreq();                             /* Determine SysTick reference freq.                */                                                                        
    cnts         = cpu_clk_freq / (CPU_INT32U)OSCfg_TickRate_Hz;  /* Determine nbr SysTick increments                 */
    OS_CPU_SysTickInit(cnts);                                     /* Init uC/OS periodic time src (SysTick).          */

#if OS_CFG_STAT_TASK_EN > 0u
    OSStatTaskCPUUsageInit(&err);                                 /* Compute CPU capacity with no task running        */
#endif
    

    CPU_IntDisMeasMaxCurReset();
    
    BSP_LED_Off(0);
    
        OSTaskCreate((OS_TCB     *)&ADCTaskTCB,                /* Create the start task                                */
                 (CPU_CHAR   *)"ADC Task",
                 (OS_TASK_PTR )ADC_Task, 
                 (void       *)0,
                 (OS_PRIO     )1,
                 (CPU_STK    *)&ADCTaskStartStk[0],
                 (CPU_STK_SIZE)0,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);
        
#ifdef DEV_COORDINATE
        
        OSTaskCreate((OS_TCB     *)&RadTaskTCB,                /* Create the start task                                */
                 (CPU_CHAR   *)"Rad Task",
                 (OS_TASK_PTR )Rad_task, 
                 (void       *)0,
                 (OS_PRIO     )1,
                 (CPU_STK    *)&RadTaskStartStk[0],
                 (CPU_STK_SIZE)0,
                 (CPU_STK_SIZE)APP_TASK_START_STK_SIZE,
                 (OS_MSG_QTY  )0,
                 (OS_TICK     )0,
                 (void       *)0,
                 (OS_OPT      )(OS_OPT_TASK_STK_CHK | OS_OPT_TASK_STK_CLR),
                 (OS_ERR     *)&err);   
#endif  
  
    while (DEF_TRUE) {                                            /* Task body, always written as an infinite loop.   */
        OSTimeDlyHMSM(0, 0, 0, 100, 
                      OS_OPT_TIME_HMSM_STRICT, 
                      &err);
/*
        OSSemPend(&AppSem,
                  100,
                  OS_OPT_PEND_BLOCKING,
                  &ts,
                  &err);
*/
    }

}

/*
* LED 점등 방식
* 
*/



void GPIO_Configure() {
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    
    GPIO_InitTypeDef GPIOD_Init;
    
    GPIOD_Init.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIOD_Init.GPIO_Pin = (GPIO_Pin_2 | GPIO_Pin_3 | GPIO_Pin_4 | GPIO_Pin_7);
    GPIOD_Init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOD, &GPIOD_Init);
    
}

/*-------------------------------------------------------------*/


static void ADC_Task(void* p_arg){
    OS_ERR err;
  
        while(DEF_TRUE){

            ADC_SoftwareStartConvCmd(ADC1, ENABLE);

            while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET);     
            
            ADC_value = ADC_GetConversionValue(ADC1);
            LCD_Display();
           
            //OSMboxPost(App_UserIFMbox,(void*)ADC_value);
            OSTimeDlyHMSM(0, 0, 0, 100, 
                          OS_OPT_TIME_HMSM_STRICT, 
                          &err);
                          
        }
}


void LCD_Display()
{
    //LCD_Clear(WHITE);
    LCD_ShowNum(15, 80, ADC_value, 5, BLUE, WHITE); // print ADC value
    
}

void two_dimensional_coord()
{   
    LCD_DrawRectangle(X_START,Y_START, X_END, Y_END);
    //(120,155) ==> (0,0)
    LCD_DrawLine(X_START ,Y0, X_END, Y0); // x
    LCD_DrawLine(X0 ,Y_START, X0, Y_END); // y
}



#ifdef DEV_COORDINATE

typedef struct {
    double x;
    double y;
} Coordinate;

/*
static void Draw_Task(void* p_arg)
{
    OS_ERR err;
    Coordinate coordinate;
    
    while (DEF_TRUE) {
        
        OSTimeDlyHMSM(0, 0, 0, 100, 
                      OS_OPT_TIME_HMSM_STRICT, 
                      &err);        
    }
    
}
*/
void Calc_Coordinate(uint32_t degree, uint32_t adc, Coordinate* coordinate)
{
    double rad = degree * PI / 180.0;
    int r = (1750 - adc/2) / 10;
    coordinate->x = r * cos(rad);
    coordinate->y = r * sin(rad);
    
    LCD_ShowNum(15, 100, r, 4, BLUE, WHITE);
    LCD_ShowNum(15, 120, (int)coordinate->x, 5, BLUE, WHITE);
    LCD_ShowNum(15, 140, (int)coordinate->y, 5, BLUE, WHITE);
   
}

static void Rad_task(void* p_arg)
{
    OS_ERR err;
    Coordinate coordinate;
    degree = 0;
    int cnt=0;
    while (DEF_TRUE) {
        
        degree++;
        if(degree>=360) {
            degree = 0;
            cnt++;
        }
        if (cnt > 10) {
            LCD_Clear(WHITE);
            two_dimensional_coord();
            cnt=0;
        }
        if(ADC_value > 1500) {
            int x = X0 + (int)coordinate.x;
            int y = Y0 + (int)coordinate.y;
            Calc_Coordinate(degree, ADC_value, &coordinate);
            if ( x >= X_START && x <= X_END && y >= Y_START && y < Y_END) 
                LCD_DrawCircle(x, y, 1);
        }
        OSTimeDlyHMSM(0, 0, 0, 1, 
                      OS_OPT_TIME_HMSM_STRICT, 
                      &err);        
    }
    
}

#endif