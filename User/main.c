#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "OLED.h"
#include "Motor.h"
#include "Key.h"
#include "Timer.h"

int32_t speed=0;
float kp=3,ki=0.1,kd=2;
float error0,error1,errorint,out=0;
int8_t keynum=0;
uint8_t l,ml,mr,r,state=0;
int8_t basespeed=45,leftspeed=0,rightspeed=0;
int8_t mode=0,hang=1;

uint8_t anjian(void)
{
    if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_15) == 0) return 15;
	if (GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_1) == 0) return 1;
	if (GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13) == 0) return 13;
    return 0;
}

void xianshi(void)
{
	if(mode==0 && hang==1)
	{
		OLED_Clear();
		OLED_ShowString(1,1,">speed control");
		OLED_ShowString(2,1,"motor control");
	}else if(mode==0 && hang==2)
	{
		OLED_Clear();
		OLED_ShowString(1,1,"speed control");
		OLED_ShowString(2,1,">motor control");
	}else if(mode==1 && hang==1)
	{
		OLED_Clear();
		OLED_ShowNum(1,1,basespeed,3);
	}
	else if(mode==1 && hang==2)
	{
		OLED_Clear();
		OLED_ShowString(1,1,"motor control E");
	}
	keynum=0;
}

void anjian1(void)
{
    static uint8_t count = 0;
    static uint8_t last_key = 3;
    
    count++;
    if(count >= 2)  // 改为20ms检测一次
    {
        count = 0;
        
        uint8_t current_key = anjian();
        
        // 直接检测当前按键状态，但去抖动
        if(current_key != 0 && last_key ==0 ) {
            keynum = current_key;
        }
        
        last_key = current_key;
    }
}

uint8_t keyget(void)
{
	uint8_t t;
	t=keynum;
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
	if(keyget()==15 && mode==0)
	{
		mode=1;
		xianshi();
	}else if(keyget()==1 && mode==1 )
	{
		mode=0;
		xianshi();
	}else if(keyget()==13 && mode==0 && hang==2)
	{
		hang=1;
		xianshi();
	}else if(keyget()==13 && mode==0 && hang==1)
	{
		hang=2;
		xianshi();
	}else if(keyget()==13 && mode==1 && hang==1)
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
	// 明确初始化显示
	OLED_Clear();
	mode = 0;
	hang = 1;
	xianshi();  // 显示正确的初始菜单
	
	while (1)
	{
		botton();
		if(mode==1 && hang==2)
		{
			l = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_9);
			ml = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_10);
			mr = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_11);
			r = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_12);

			// 保存上一次的有效速度
			static int8_t last_leftspeed = 0;
			static int8_t last_rightspeed = 0;

			// 修正：电机2是右轮，电机1是左轮
			if (ml == 1 && mr == 1) {
				// 直线
				last_leftspeed = basespeed;
				last_rightspeed = basespeed;
				Motor_SetSpeed1(basespeed);  // 左轮
				Motor_SetSpeed2(basespeed);  // 右轮
			} 
			else if (ml == 0 && mr == 1) {
				// 偏右，需要向左修正
				// 左轮加速，右轮减速
				last_leftspeed = basespeed + 40;
				last_rightspeed = basespeed - 30;
				Motor_SetSpeed1(last_leftspeed);   // 左轮加速
				Motor_SetSpeed2(last_rightspeed);  // 右轮减速
			} 
			else if (ml == 1 && mr == 0) {
				// 偏左，需要向右修正
				// 右轮加速，左轮减速
				last_leftspeed = basespeed - 30;
				last_rightspeed = basespeed + 40;
				Motor_SetSpeed1(last_leftspeed);   // 左轮减速
				Motor_SetSpeed2(last_rightspeed);  // 右轮加速
			} 
			else if (l == 1) {
				// 严重偏右，需要急向左转
				// 左轮加速/正转，右轮减速/反转
				last_leftspeed = 90;    // 左轮全速
				last_rightspeed = -80;  // 右轮反转
				Motor_SetSpeed1(last_leftspeed);   // 左轮
				Motor_SetSpeed2(last_rightspeed);  // 右轮
			} 
			else if (r == 1) {
				// 严重偏左，需要急向右转
				// 右轮加速/正转，左轮减速/反转
				last_leftspeed = -80;   // 左轮反转
				last_rightspeed = 90;   // 右轮全速
				Motor_SetSpeed1(last_leftspeed);   // 左轮
				Motor_SetSpeed2(last_rightspeed);  // 右轮
			}
			else if ((l == 1 && ml == 1) || (l == 1 && mr == 1)) {
				// 极右偏
				last_leftspeed = 100;   // 左轮全速
				last_rightspeed = -100;  // 右轮反转
				Motor_SetSpeed1(last_leftspeed);
				Motor_SetSpeed2(last_rightspeed);
			} 
			else if ((r == 1 && ml == 1) || (r == 1 && mr == 1)) {
				// 极左偏
				last_leftspeed = -100;   // 左轮反转
				last_rightspeed = 100;  // 右轮全速
				Motor_SetSpeed1(last_leftspeed);
				Motor_SetSpeed2(last_rightspeed);
			} else if (l == 1 && ml == 1 && mr == 1 && r == 1) 
			{
				// 十字路口 - 直行通过
				last_leftspeed = basespeed;
				last_rightspeed = basespeed;
				Motor_SetSpeed1(last_leftspeed);   // 左轮
				Motor_SetSpeed2(last_rightspeed);  // 右轮
			} else {
				// 所有传感器离线，维持上次速度
				Motor_SetSpeed1(last_leftspeed);   // 左轮
				Motor_SetSpeed2(last_rightspeed);  // 右轮
			}
			
			// 修正的循迹逻辑
//			if (ml == 1 && mr == 1) {
//				// 两个中间传感器都在线上 - 直线行驶
//				Motor_SetSpeed2(basespeed);
//				Motor_SetSpeed1(basespeed);
//			} 
//			else if (ml == 0 && mr == 1) {
//				// 只有右边中间传感器在线 - 车体偏左，需要向右修正
//				// 左轮加速，右轮减速
//				Motor_SetSpeed2(basespeed + 15);
//				Motor_SetSpeed1(basespeed - 15);
//			} 
//			else if (ml == 1 && mr == 0) {
//				// 只有左边中间传感器在线 - 车体偏右，需要向左修正
//				// 右轮加速，左轮减速
//				Motor_SetSpeed2(basespeed - 15);
//				Motor_SetSpeed1(basespeed + 15);
//			} 
//			else if (l == 1) {
//				// 左边传感器检测到线 - 严重偏右，需要急向左转
//				Motor_SetSpeed2(basespeed - 30);  // 左轮减速或反转
//				Motor_SetSpeed1(basespeed + 30);  // 右轮加速
//			} 
//			else if (r == 1) {
//				// 右边传感器检测到线 - 严重偏左，需要急向右转
//				Motor_SetSpeed2(basespeed + 30);  // 左轮加速
//				Motor_SetSpeed1(basespeed - 30);  // 右轮减速或反转
//			} 
//			else if (l == 1 && ml == 1) {
//				// 左边和左中传感器都在线 - 极右偏
//				Motor_SetSpeed2(-40);  // 左轮反转
//				Motor_SetSpeed1(60);   // 右轮加速
//			}
//			else if (r == 1 && mr == 1) {
//				// 右边和右中传感器都在线 - 极左偏
//				Motor_SetSpeed2(60);   // 左轮加速
//				Motor_SetSpeed1(-40);  // 右轮反转
//			}
//			else {
//				// 所有传感器都离线 - 停车或保持上次状态
//				Motor_SetSpeed2(0);
//				Motor_SetSpeed1(0);
//			}
			
//			errorint+=error0;
//			if(errorint>100)errorint=100;
//			if(errorint<-100)errorint=-100;
//			out=kp*error0+ki*errorint+kd*(error0-error1);
//			leftspeed=basespeed-(int16_t)out;
//			rightspeed=basespeed+(int16_t)out;
//			if(leftspeed>100)leftspeed=100;
//			if(leftspeed<-100)leftspeed=-100;
//			if(rightspeed<-100)rightspeed=-100;
//			if(rightspeed>100)rightspeed=100;
//			Motor_SetSpeed2(leftspeed);
//			Motor_SetSpeed1(rightspeed);
			Delay_ms(5);
		}
	}
}
