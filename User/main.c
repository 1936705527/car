#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "Key.h"
#include "Timer.h"

int32_t speed=0;
float kp=0,ki=0.1,kd=0;
float error0,error1,errorint,out=0;
int8_t keynum=0;
uint8_t l,ml,mr,r,state=0;
int8_t basespeed=50,leftspeed=0,rightspeed=0;
int8_t mode=0,hang=1;

uint8_t anjian(void)
{
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15) == 0) return 0;
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == 0) return 1;
	if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 0) return 2;
    return 3;
}

void xianshi(void)
{
	if(mode==0 && hang==1)
	{
		OLED_ShowString(1,1,">speed control");
		OLED_ShowString(2,1,"motor control");
	}else if(mode==0 && hang==2)
	{
		OLED_ShowString(1,1,"speed control");
		OLED_ShowString(2,1,">motor control");
	}else if(mode==1 && hang==1)OLED_ShowNum(1,1,basespeed,3);
	else if(mode==1 && hang==2)OLED_ShowString(1,1,"motor control E");
}

void anjian1(void)
{
    static uint8_t count = 0;
    static uint8_t last_state = 3;
    static uint8_t current_state = 3;
    
    count++;
    if(count >= 2) 
    {
        count = 0;
        
        last_state = current_state;
        current_state = anjian();  
        
        // 检测按键按下（从无按键到有按键）
        if(last_state == 3 && current_state != 3) {
            keynum = current_state;
        }
    }
}

uint8_t keyget(void)
{
	uint8_t t;
	t=keynum;
	keynum=3;
	return t;
}


void TIM4_IRQHandler(void)
{
	if (TIM_GetITStatus(TIM4, TIM_IT_Update) == SET)
	{
		anjian1();
		TIM_ClearITPendingBit(TIM4, TIM_IT_Update);
	}
}

void botton(void)
{
	if(keyget()==0 && mode==0 && hang==1)
	{
		mode=1;
		xianshi();
	}else if(keyget()==0 && mode==0 && hang==2)
	{
		mode=1;
		xianshi();
	}else if(keyget()==1 && mode==1 && hang==2)
	{
		mode=0;
		xianshi();
	}else if(keyget()==1 && mode==1 && hang==1)
	{
		mode=0;
		xianshi();
	}else if(keyget()==2 && mode==0 && hang==2)
	{
		hang=1;
		xianshi();
	}else if(keyget()==2 && mode==0 && hang==1)
	{
		hang=2;
		xianshi();
	}else if(keyget()==2 && mode==1 && hang==1)
	{
		basespeed+=5;
		xianshi();
	}
}


int main(void)
{
	OLED_Init();		
	Motor_Init();		
	Key_Init();			
	Timer_Init();
	
	l = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9);
	ml = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
	mr = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11);
	r = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12);
	error0=0;
	error1=0;
	errorint=0;
	
	while (1)
	{
		botton();
		if(mode==1 && hang==2)
		{
			l = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9);
			ml = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
			mr = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11);
			r = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12);
			error1=error0;
			// 四个传感器的循迹判断逻辑
            if (ml == 1 && mr == 1) {
                error0 = 0;        // 直线
            } else if (ml == 1 && mr == 0) {
                error0 = -1;       // 轻微左偏
            } else if (ml == 0 && mr == 1) {
                error0 = 1;        // 轻微右偏
            } else if (l == 1 && mr == 0) {
                error0 = -2;       // 严重左偏
            } else if (r == 1 && ml == 0) {
                error0 = 2;        // 严重右偏
            } else if (l == 1 && ml == 1) {
                error0 = -3;       // 极左偏
            } else if (mr == 1 && r == 1) {
                error0 = 3;        // 极右偏
            } else {
                error0 = (error1 < 0) ? -4 : 4;  // 丢失线路，保持方向
            }
            
			errorint+=error0;
			if(errorint>100)errorint=100;
			if(errorint<-100)errorint=-100;
			out=kp*error0+ki*errorint+kd*(error0-error1);
			leftspeed=basespeed+(int16_t)out;
			rightspeed=basespeed-(int16_t)out;
			if(leftspeed>100)leftspeed=100;
			if(leftspeed<-100)leftspeed=-100;
			if(rightspeed<-100)rightspeed=-100;
			if(rightspeed>100)rightspeed=100;
			Motor_SetSpeed1(leftspeed);
			Motor_SetSpeed2(rightspeed);
			Delay_ms(5);
		}
	}
}
