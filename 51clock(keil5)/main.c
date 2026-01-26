#include <STC89C5xRC.H>
#include <intrins.h>
#define uchar unsigned char
#define uint unsigned int
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE 

sbit onewire_dq = P3^7;//温度传感器引脚
float T;//温度初始数据

//数码管段选
uchar table[] = {0xC0,0xF9,0xA4,0xB0,0x99,0x92,0x82,0xF8,0x80,0x90,0xBF};//0~9,-
uchar table_dot[] = {0x40,0x79,0x24,0x30,0x19,0x12,0x02,0x78,0x00,0x10,0xBF};//带小数点0~9,-

sbit BEEP=P1^5;//蜂鸣器

uchar whereNow;	//0:时钟界面；1:调时界面；2:闹钟界面；3:日历界面；4:温度界面；5:秒表界面

sbit K1=P1^0;//五个独立按键
sbit K2=P1^1;
sbit K3=P1^2;
sbit K4=P1^3;
sbit K5=P1^4;
 
#define shanCount 128//选中位置闪烁计数标志
uchar blankCount;//空白显示 计数标志
uchar normalCount;//正常显示 计数标志

uchar shi=0,fen=0,miao=0;	//时钟时、分、秒 初始数据

uchar clockSetLocation;//记录调时选中的位置 0,不闪烁,且无法使用加减按钮

uchar alarmSetLocation;//记录闹钟设置时的所选位置
uchar shi1=0,fen1=0,miao1=5;//闹钟时、分、秒 初始数据 
bit alarmOnOff;//表明闹钟是否开启
bit alarmSetOrNot;//表明是否正在设置闹钟
uchar cldSetLocation;//记录闹钟设置时的所选位置

uint year=2025;	//日历 年 初始数据
uchar month=12,day=28;//日历 月、日 初始数据
#define minYear 2020//设定日历可显示的年份上下限
#define maxYear 2050
bit isLeapYear;	//表明是否是闰年 1，是；0，否。
bit cldSetOrNot;//表明是否正在设置日历
uchar commonYearTable[]={0,31,28,31,30,31,30,31,31,30,31,30,31};//记录平年每个月有多少天
uchar leapYearTable[]={0,31,29,31,30,31,30,31,31,30,31,30,31};	//记录闰年每个月有多少天

uint stopwatch_time_fen = 0,stopwatch_time_miao = 0,stopwatch_time_miao_2 = 0;//秒表累计时间（毫秒）
uchar stopwatch_state = 0;  //0-停止，1-运行，2-暂停
uint watch_count = 0;

void delay1ms(unsigned int xms)	//@12.000MHz
{
	unsigned char data i, j;
	while(xms--)
	{
		i = 2;
		j = 239;
		do
		{
			while (--j);
		} while (--i);
	}
}

unsigned char onewire_Init()
{
	unsigned char i;
	unsigned char AckBit;
	onewire_dq = 1;
	onewire_dq = 0;
	i = 247;
	while (--i);//delay 500us
	onewire_dq = 1;
	i = 32;
	while (--i);//delay 70us
	AckBit = onewire_dq;
	i = 247;
	while (--i);//delay 500us
	return AckBit;
}

void onewire_sendbit(uchar Bit)
{
	unsigned char i;
	onewire_dq = 0;
	i = 4;
	while (--i);//delay 10us
	onewire_dq = Bit;
	i = 24;
	while (--i);//delay 50us
	onewire_dq = 1;
}

unsigned char onewire_receivebit()
{
	unsigned char i;
	unsigned char Bit;
	onewire_dq = 0;
	i = 2;
	while (--i);//delay 5us
	onewire_dq = 1;
	i = 2;
	while (--i);//delay 5us
	Bit = onewire_dq;
	i = 24;
	while (--i);//delay 50us
	return Bit;
}

void onewire_sendbyte(uchar Byte)
{
	unsigned char i;
	for(i = 0;i < 8;i++)
	{
		onewire_sendbit(Byte & (0x01 << i));
	}
}

unsigned char onewire_receivebyte()
{
	unsigned char i;
	unsigned char Byte = 0x00;
	for(i = 0;i < 8;i++)
	{
		if(onewire_receivebit())
		{
			Byte |= (0x01 << i);
		}
	}
	return Byte;
}

void DS18B20_ConvertT()
{
	onewire_Init();
	onewire_sendbyte(DS18B20_SKIP_ROM);
	onewire_sendbyte(DS18B20_CONVERT_T);
}

float DS18B20_ReadT()
{
	uchar TLSB,TMSB;
	int Temp;
	float T;
	onewire_Init();
	onewire_sendbyte(DS18B20_SKIP_ROM);
	onewire_sendbyte(DS18B20_READ_SCRATCHPAD);
	TLSB = onewire_receivebyte();
	TMSB = onewire_receivebyte();
	Temp = (TMSB << 8) | TLSB;
	T = Temp *0.0625;
	return T;
}

void Timer0_Init(void)		//1毫秒@12.000MHz
{
	AUXR &= 0x7F;			//定时器时钟12T模式
	TMOD &= 0xF0;			//设置定时器模式
	TMOD |= 0x01;			//设置定时器模式
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	TF0 = 0;				//清除TF0标志
	TR0 = 1;				//定时器0开始计时
	ET0 = 1;				//使能定时器0中断
	EA = 1;
}

void Timer0_Isr(void) interrupt 1
{
	static uint count=0;
	TL0 = 0x18;				//设置定时初始值
	TH0 = 0xFC;				//设置定时初始值
	count++;//定时器计数标志+1
	
	if(whereNow == 5 && stopwatch_state == 1) // 在秒表界面且正在运行
	{
		watch_count++;
		
		if(10 == watch_count)
		{
			stopwatch_time_miao_2++;// 增加10毫秒（0.01秒）
			watch_count=0;
		}
		if(100 ==stopwatch_time_miao_2)
		{
			stopwatch_time_miao++;
			stopwatch_time_miao_2=0;
		}
		if(60 == stopwatch_time_miao)
		{
			stopwatch_time_fen++;
			stopwatch_time_miao=0;
		}
		if(60 == stopwatch_time_fen)
		{
			stopwatch_time_fen=0;
		}
	}
	
	if(1000==count)//定时器次数达到1000次，秒+1
	{
		miao++;
		T = DS18B20_ReadT();
		count=0;//定时器次数清零
	}
	if(60==miao)//秒计数达到60次，分+1
	{
		fen++;
		miao=0;//秒计数清零
	}
	if(60==fen)//分计数达到60次，时+1
	{
		shi++;
		fen=0;//分计数清零
	}
	if(24==shi)//时计数达到24次，日+1
	{	 	
		day++;
		shi=0;//时计数清零
	}	
	if(0==isLeapYear)//如果是平年	
	{	
		if(day>commonYearTable[month])
		{
			 day=1;//如果日计数超过本月天数，则变为1
			 month++;
		}
	}
	else			//如果是闰年
	{
		if(day>leapYearTable[month])
		{
			 day=1;//如果日计数超过本月天数，则变为1
			 month++;
		}
	}		
	if(month>12)
	{
		month=1;//如果月计数超过12，则变为1
		year++;
	}
	if(year>maxYear)
	{
		year=minYear;//如果年计数超过一定限度，则变为最低限度年份
	}
}

void is_leap_year()//--------------------------判断是否是闰年
{
	if((year%4==0 && year%100!=0) || (year%400==0))
	isLeapYear=1;
	else
	isLeapYear=0;
}

void smg(uchar Location,Number)//数码管单个位置亮一下
{
	switch(Location)
	{
		case 1:P0 = 0xFE;break;
		case 2:P0 = 0xFD;break;
		case 3:P0 = 0xFB;break;
		case 4:P0 = 0xF7;break;
		case 5:P0 = 0xEF;break;
		case 6:P0 = 0xDF;break;
		case 7:P0 = 0xBF;break;
		case 8:P0 = 0x7F;break;
	}
	P2 = Number; 
	delay1ms(1);
	P2 = 0xFF;
}

void clock_display()//-------------------------数码管动态显示时钟界面
{
	//时的十位 
		//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
		if(1 == clockSetLocation && blankCount>0)
		{
			smg(1,0xFF);
			blankCount--;
		}
		//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
		else if(1 == clockSetLocation && normalCount>0)
		{
			smg(1,table[shi/10]);
			normalCount--;
		}
		else if(1 == clockSetLocation)
		{
			blankCount = shanCount;//闪烁标志初始化
			normalCount = shanCount;	
		}
		else
		smg(1,table[shi/10]);

	//时的个位 
		//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
		if(1 == clockSetLocation && blankCount>0)
		{
			smg(2,0xFF);
			blankCount--;
		}
		//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
		else if(1 == clockSetLocation && normalCount>0)
		{
			smg(2,table[shi%10]);
			normalCount--;
		}
		else if(1 == clockSetLocation)
		{
			blankCount = shanCount;//闪烁标志初始化
			normalCount = shanCount;	
		}
		else
		smg(2,table[shi%10]);

	//-	
		smg(3,table[10]);

	//分的十位 
		//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
		if(2==clockSetLocation && blankCount>0)
		{
			smg(4,0xFF);
			blankCount--;
		}
		//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
		else if(2 == clockSetLocation && normalCount > 0)
		{
			smg(4,table[fen/10]);
			normalCount--;
		}
		else if(2 == clockSetLocation)
		{
			blankCount = shanCount;//闪烁标志初始化
			normalCount = shanCount;	
		}
		else
		smg(4,table[fen/10]);

	//分的个位 
		//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
		if(2 == clockSetLocation && blankCount>0)
		{
			smg(5,0xFF);
			blankCount--;
		}
		//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
		else if(2 == clockSetLocation && normalCount>0)
		{
			smg(5,table[fen%10]);
			normalCount--;
		}
		else if(2 == clockSetLocation)
		{
			blankCount = shanCount;//闪烁标志初始化
			normalCount = shanCount;	
		}
		else
		smg(5,table[fen%10]);

	//-	
		smg(6,table[10]);

	//秒的十位 
		//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
		if(3 == clockSetLocation && blankCount>0)
		{
			smg(7,0xFF);
			blankCount--;
		}
		//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
		else if(3 == clockSetLocation && normalCount>0)
		{
			smg(7,table[miao/10]);
			normalCount--;
		}
		else if(3 == clockSetLocation)
		{
			blankCount = shanCount;//闪烁标志初始化
			normalCount = shanCount;	
		}
		else
		smg(7,table[miao/10]);

	//秒的个位 
		//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
		if(3 == clockSetLocation && blankCount>0)
		{
			smg(8,0xFF);
			blankCount--;
		}
		//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
		else if(3 == clockSetLocation && normalCount>0)
		{
			smg(8,table[miao%10]);
			normalCount--;
		}
		else if(3 == clockSetLocation)
		{
			blankCount = shanCount;//闪烁标志初始化
			normalCount = shanCount;	
		}
		else
		smg(8,table[miao%10]);
}

void clock_set()//--------------------------调时(与时钟同界面)
{
	if(0==K1 && 0==whereNow)	//如果此时在时钟界面并按下了调时按键K1
	{
		delay1ms(20);//消抖
		if(0==K1)//再次检测按键状态
		{	
			while(0==K1);//长按等待
			delay1ms(20);//消抖
			whereNow=1;
			clockSetLocation=1;
			TR0=0;
		}
	}
	if(1==whereNow)	//进入调时界面，4个独立按键化身为调时相关按钮
	{
		if(0==K1)//如果按下K1:调整位置
		{
			delay1ms(20);//消抖
			if(0==K1)//再次检测按键状态
			{	 
				while(0==K1);//长按停留在此
				delay1ms(20);//消抖
				clockSetLocation++;//位置标志+1
				if(clockSetLocation>3)
					clockSetLocation=1;
			}
		}

			if(1==clockSetLocation)//进行“时”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(0==shi)
							shi=24;
						shi--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						shi++;
						if(24==shi)
							shi=0;
					}	
				}
			}

			if(2==clockSetLocation)//进行“分”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(0==fen)
							fen=60;
						fen--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						fen++;
						if(60==fen)
							fen=0;
					}	
				}
			}

			if(3==clockSetLocation)//进行“秒”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(0==miao)
							miao=60;
						miao--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						miao++;
						if(60==miao)
							miao=0;
					}	
				}
			}	
	 
		if(0==K4)//如果按下K4：确认并退出调时模式，然后返回时钟界面
		{
			delay1ms(20);//消抖
			if(0==K4)//再次检测按键状态
			{	 
				while(0==K4);//长按停留在此
				delay1ms(20);//消抖
				clockSetLocation=0;
				whereNow=0;
				TR0=1;
			}
		} 
	}
}

void alarm_display()//----------------------数码管动态显示闹钟界面
{
	if(0==alarmOnOff)//如果闹钟关闭
		smg(1,0x8C);//'P.'表示STOP,闹钟关闭
	if(1==alarmOnOff)//如果闹钟开启
		smg(1,0x86);//'E.'表示OPEN,闹钟开
	
//时的十位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(1 == alarmSetLocation && blankCount>0)
	{
		smg(3,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(1 == alarmSetLocation && normalCount>0)
	{
		smg(3,table[shi1/10]);
		normalCount--;
	}
	else if(1 == alarmSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(3,table[shi1/10]);
	
//时的个位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(1 == alarmSetLocation && blankCount>0)
	{
		smg(4,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(1 == alarmSetLocation && normalCount>0)
	{
		smg(4,table_dot[shi1%10]);
		normalCount--;
	}
	else if(1 == alarmSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(4,table_dot[shi1%10]);
//分的十位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(2 == alarmSetLocation && blankCount>0)
	{
		smg(5,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(2 == alarmSetLocation && normalCount>0)
	{
		smg(5,table[fen1/10]);
		normalCount--;
	}
	else if(2==alarmSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(5,table[fen1/10]);
	
//分的个位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(2 == alarmSetLocation && blankCount>0)
	{
		smg(6,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(2 == alarmSetLocation && normalCount>0)
	{
		smg(6,table_dot[fen1%10]);
		normalCount--;
	}
	else if(2 == alarmSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(6,table_dot[fen1%10]);
	
//秒的十位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(3 == alarmSetLocation && blankCount>0)
	{
		smg(7,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(3 == alarmSetLocation && normalCount>0)
	{
		smg(7,table[miao1/10]);
		normalCount--;
	}
	else if(3 == alarmSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(7,table[miao1/10]);
	
//秒的个位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(3 == alarmSetLocation && blankCount>0)
	{
		smg(8,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(3 == alarmSetLocation && normalCount>0)
	{
		smg(8,table_dot[miao1%10]);
		normalCount--;
	}
	else if(3==alarmSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(8,table_dot[miao1%10]);
}

void alarm_set()//----------------------闹钟设置
{
	if(0==whereNow && 0==K2)//如果此时在时钟界面并按下了闹钟按键K2
	{
		delay1ms(20);//消抖
		if(0==K2)//再次检测按键状态
		{	
			while(0==K2);//长按等待
			delay1ms(20);//消抖
			whereNow=2;
		}
	}
	
	if(2==whereNow)//如果此时在闹钟界面，那么四个独立按键化身为闹钟相关按键
	{
		if(0==K1)//如果按下K1:调整位置
		{
			delay1ms(20);//消抖
			if(0==K1)//再次检测按键状态
			{	 
				while(0==K1);//长按停留在此
				delay1ms(20);//消抖
				alarmSetLocation++;//位置标志+1
				if(4==alarmSetLocation)
					alarmOnOff=!alarmOnOff;
				if(alarmSetLocation>3)
					alarmSetLocation=0; 	
			}
		}		

			if(1==alarmSetLocation)//进行“时”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(0==shi1)shi1=24;
						shi1--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						shi1++;
						if(24==shi1)shi1=0;
					}	
				}
			}

			if(2==alarmSetLocation)//进行“分”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(0==fen1)fen1=60;
						fen1--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						fen1++;
						if(60==fen1)fen1=0;
					}	
				}
			}

			if(3==alarmSetLocation)//进行“秒”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(0==miao1)miao1=60;
						miao1--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						miao1++;
						if(60==miao1)miao1=0;
					}	
				}	
			}

			if(0==K4)//如果按下K4：确认并退出闹钟界面，然后返回时钟界面
		   	{
				delay1ms(20);//消抖
				if(0==K4)//再次检测按键状态
				{	 
					while(0==K4);//长按停留在此
					delay1ms(20);//消抖
					alarmSetLocation=0;
					whereNow=0;
				}
			} 
	}	
}

void alarm_exe()//---------------------------------------------闹钟执行
//只有当以下条件【都满足】的时候，闹钟才会响起：
//1、时钟的时分秒=闹钟的时分秒
//2、闹钟处于开启状态
//3、当前在时钟运行界面
//
//注：按下独立按键中的任意一个并松开，可停止闹钟
{	
	if(shi==shi1 && fen==fen1 && miao==miao1 && 1==alarmOnOff && 0==whereNow)
	{
		while(1==K1 && 1==K2 && 1==K3 && 1==K4)
		{
		BEEP=~BEEP;
		smg(1,0x88);//'A'
			BEEP=~BEEP;
		smg(2,0x47);//'L.'
			BEEP=~BEEP;
		smg(3,table[shi1/10]);
			BEEP=~BEEP;
		smg(4,table_dot[shi1%10]);
			BEEP=~BEEP;
		smg(5,table[fen1/10]);
			BEEP=~BEEP;
		smg(6,table_dot[fen1%10]);
			BEEP=~BEEP;
		smg(7,table[miao1/10]);
			BEEP=~BEEP;
		smg(8,table[miao1%10]);
			BEEP=~BEEP;
		}
		while(0==K1 || 0==K2 || 0==K3 || 0==K4);//等待，直至松开独立按键
		delay1ms(20);
		BEEP=1;
	}
}

void cld_display()//------------------------数码管动态显示日历界面
{
//年的千位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(1 == cldSetLocation && blankCount>0)
	{
		smg(1,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(1 == cldSetLocation && normalCount>0)
	{
		smg(1,table[year/1000]);
		normalCount--;
	}
	else if(1 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(1,table[year/1000]);
	
//年的百位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(1 == cldSetLocation && blankCount>0)
	{
		smg(2,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(1 == cldSetLocation && normalCount>0)
	{
		smg(2,table[year/100%10]);
		normalCount--;
	}
	else if(1 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(2,table[year/100%10]);
	
//年的十位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(1 == cldSetLocation && blankCount>0)
	{
		smg(3,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(1 == cldSetLocation && normalCount>0)
	{
		smg(3,table[year%100/10]);
		normalCount--;
	}
	else if(1 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(3,table[year%100/10]);
	
//年的个位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(1 == cldSetLocation && blankCount>0)
	{
		smg(4,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(1 == cldSetLocation && normalCount>0)
	{
		smg(4,table_dot[year%10]);
		normalCount--;
	}
	else if(1 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(4,table_dot[year%10]);
	
//月的十位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(2 == cldSetLocation && blankCount>0)
	{
		smg(5,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(2 == cldSetLocation && normalCount>0)
	{
		smg(5,table[month/10]);
		normalCount--;
	}else if(2 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(5,table[month/10]);
	
//月的个位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(2 == cldSetLocation && blankCount>0)
	{
		smg(6,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(2 == cldSetLocation && normalCount>0)
	{
		smg(6,table_dot[month%10]);
		normalCount--;
	}
	else if(2==cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(6,table_dot[month%10]);
	
//日的十位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(3 == cldSetLocation && blankCount>0)
	{
		smg(7,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(3 == cldSetLocation && normalCount>0)
	{
		smg(7,table[day/10]);
		normalCount--;
	}
	else if(3 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(7,table[day/10]);
	
//日的个位
	//如果正在调整此位置且闪烁空白计数不为0，则此位置继续空白显示
	if(3 == cldSetLocation && blankCount>0)
	{
		smg(8,0xFF);
		blankCount--;
	}
	//如果正在调整此位置且闪烁正常计数不为0，则此位置继续正常显示
	else if(3 == cldSetLocation && normalCount>0)
	{
		smg(8,table_dot[day%10]);
		normalCount--;
	}
	else if(3 == cldSetLocation)
	{
		blankCount=shanCount;//闪烁标志初始化
		normalCount=shanCount;	
	}
	else
	smg(8,table_dot[day%10]);	
}

void cld_set()//---------------------------日历设置
{
	if(0==whereNow && 0==K3)//如果此时在时钟界面并按下了日历按键K3
	{
		delay1ms(20);//消抖
		if(0==K3)//再次检测按键状态
		{	
			while(0==K3);//长按等待
			delay1ms(20);//消抖
			whereNow=3;
		}
	}
	
	if(3==whereNow)//如果此时在日历界面，那么四个独立按键化身为日历相关按键
	{
		if(0==K1)//如果按下K1:调整位置
		{
			delay1ms(20);//消抖
			if(0==K1)//再次检测按键状态
			{	 
				while(0==K1);//长按停留在此
				delay1ms(20);//消抖
				cldSetLocation++;//位置标志+1
				if(cldSetLocation>3)
					cldSetLocation=1; 	
			}
		}		

			if(1==cldSetLocation)//进行“年”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(minYear==year)
							year=minYear+1;//设定日历年份减到最低限度则无法再减
						year--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						year++;
						if(maxYear+1==year)
							year=maxYear;//设定日历年份加到最大限度则无法再加
					}	
				}
			}

			if(2==cldSetLocation)//进行“月”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(1==month)
							month=13;
						month--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						month++;
						if(13==month)
							month=1;
					}	
				}
			}

			if(3==cldSetLocation)//进行“日”加减
			{
				if(0==K2)//按下K2：数字-1
				{
					delay1ms(20);//消抖
					if(0==K2)//再次检测按键状态
					{	 
						while(0==K2);//长按停留在此
						delay1ms(20);//消抖
						if(day<=1)
						{					
							if(0==isLeapYear)//如果是平年
								day=commonYearTable[month]+1;
							if(1==isLeapYear)//如果是闰年
								day=leapYearTable[month]+1;	
						}	
						day--;
					}	
				}
				if(0==K3)//按下K3：数字+1
				{
					delay1ms(20);//消抖
					if(0==K3)//再次检测按键状态
					{	 
						while(0==K3);//长按停留在此
						delay1ms(20);//消抖
						day++;					
						if(0==isLeapYear)//如果是平年
						{
							if(day>commonYearTable[month])
								day=1;
						}
						if(1==isLeapYear)//如果是闰年
						{
							if(day>leapYearTable[month])
								day=1;
						}
					}	
				}	
			}

			if(0==K4)//如果按下K4：确认并退出日历界面，然后返回时钟界面
		   	{
				delay1ms(20);//消抖
				if(0==K4)//再次检测按键状态
				{	 
					while(0==K4);//长按停留在此
					delay1ms(20);//消抖
					cldSetLocation=0;
					whereNow=0;
				}
			} 
	}		
}

void stopwatch_display()
{
		smg(1,table[stopwatch_time_fen/10]);
		smg(2,table[stopwatch_time_fen%10]);
		smg(3,table[10]);
		smg(4,table[stopwatch_time_miao/10]);
		smg(5,table_dot[stopwatch_time_miao%10]);
		smg(6,table[10]);
		smg(7,table[stopwatch_time_miao_2/10]);
		smg(8,table[stopwatch_time_miao_2%10]);
}

void stopwatch_set()
{
	if(0==whereNow && 0==K5)//如果此时在时钟界面并按下了秒表按键K5
	{
		delay1ms(20);//消抖
		if(0==K5)//再次检测按键状态
		{	
			while(0==K5);//长按等待
			delay1ms(20);//消抖
			whereNow=5;
		}
	}
	if(5==whereNow)
	{
		if(K1== 0)  // 如果按下K1
		{ 
			delay1ms(20);  // 消抖
      if(K1 == 0) // 再次检测按键状态
			{  
				while(K1 == 0);  // 等待按键释放
        delay1ms(20);  // 消抖
        if(stopwatch_state == 0 || stopwatch_state == 2)// 如果当前是停止或暂停状态
				{  
					stopwatch_state = 1;  // 开始运行
				} 
				else if(stopwatch_state == 1)// 如果当前是运行状态
				{  
					stopwatch_state = 2;  // 暂停
				}
      }
		}
		
		if(0 == K2)  // 如果按下K2
		{ 
			delay1ms(20);  // 消抖
      if(0 == K2) // 再次检测按键状态
			{  
				while(0 == K2);  // 等待按键释放
        delay1ms(20);  // 消抖
        stopwatch_time_miao_2 = 0;  // 时间清零
				stopwatch_time_miao = 0;
				stopwatch_time_fen = 0;
				stopwatch_state = 0;    // 状态置为停止
				watch_count = 0;
      }
		}
		
		if(0==K4)//如果按下K4：确认并退出秒表界面，然后返回时钟界面
		{
			delay1ms(20);//消抖
			if(0==K4)//再次检测按键状态
			{	 
				while(0==K4);//长按停留在此
				delay1ms(20);//消抖
				whereNow=0;
			}
		} 
	}
}

void T_display()
{
	uchar integer_part;    // 整数部分
  uchar decimal_part1;    // 小数部分1
	uchar decimal_part2;    // 小数部分2
	
	DS18B20_ConvertT();
	
	integer_part = (uchar)T;  // 整数部分
  decimal_part1 = (uchar)((T - integer_part) * 100);  // 小数部分1
	decimal_part2 = (uchar)((((T - integer_part)*100)-decimal_part1) * 100);  // 小数部分2
	
	smg(2,table[integer_part/10]);
	smg(3,table_dot[integer_part%10]);
	smg(4,table[decimal_part1/10]);
	smg(5,table[decimal_part1%10]);
	smg(6,table[decimal_part2/10]);
	smg(7,table[decimal_part2%10]);
	smg(8,0xA7);
}

void T_set()
{
	if(0==whereNow && 0==K4)//如果此时在时钟界面并按下了秒表按键K4
	{
		delay1ms(20);//消抖
		if(0==K4)//再次检测按键状态
		{	
			while(0==K4);//长按等待
			delay1ms(20);//消抖
			whereNow=4;
		}
	}

	if(4==whereNow && 0==K4)//如果此时在时钟界面并按下了秒表按键K4
	{
		delay1ms(20);//消抖
		if(0==K4)//再次检测按键状态
		{	
			while(0==K4);//长按等待
			delay1ms(20);//消抖
			whereNow=0;
		}
	}
}

void main()
{
	clockSetLocation=0;	//
	alarmSetOrNot=0;	//
	alarmSetLocation=0;	//
	cldSetOrNot=0;		//
	cldSetLocation=0;  	//各界面位置标志与是否调整标志初始化为0
	alarmOnOff=0;		//闹钟默认关闭
	whereNow=0;			//默认最开始在时钟界面
	blankCount=shanCount; //
	normalCount=shanCount;//闪烁计数标志初始化
	Timer0_Init();	//定时器T0初始化
	
	while(1)
	{
		is_leap_year();	//判断是否是闰年	
		if(0==whereNow || 1==whereNow)
			clock_display();//数码管动态显示时钟界面
		clock_set();	//调时(与时钟同界面)	
		if(2==whereNow)
			alarm_display();	//数码管动态显示闹钟界面
		alarm_set();	//闹钟设置
		alarm_exe();	//闹钟执行
		if(3==whereNow)
			cld_display();//数码管动态显示日历界面
		cld_set();		//日历设置
		if(4==whereNow)
			stopwatch_display();//数码管动态显示秒表界面
		stopwatch_set();//秒表设置
		if(5==whereNow)
			T_display();//数码管动态显示温度界面
		T_set();//温度设置
	}
}
