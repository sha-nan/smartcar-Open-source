#include "Handle.h"

uint8 colour[2] = {255, 0},black=1,white=0; //0 和 1 分别对应的颜色
//注：山外的摄像头 0 表示 白色，1表示 黑色
uchar Weight[60] = { 5,  6,  8,  9, 11, 12, 14, 15, 17, 18,	// 50
					20, 21, 22, 23, 24, 25, 26, 27, 28, 29,	// 40
				    27, 27, 26, 25, 24, 23, 22, 21, 20, 19,	// 30
				    4,  3,  3,  2,  1,    1,  1,  1,  1,  1,	// 20
					1,  1,  1,  1,  1,    1,  1,  1,  1,  1,
					1,  1,  1,  1,  1,    1,  1,  1,  1,  1};	//加权平均参数

uchar Left_Line[62], Right_Line[62], Mid_Line[62];	// 原始左右边界数据
uchar Left_Add_Line[62], Right_Add_Line[62];		// 左右边界补线数据
uchar Left_Add_Flag[62], Right_Add_Flag[62];		// 左右边界补线标志位
uchar Jump[62];
uchar Width_Real[62];	// 实际赛道宽度
uchar Width_Add[62];	// 补线赛道宽度
uchar Width_Min;		// 最小赛道宽度

uchar Foresight;	// 前瞻，用于速度控制
uchar Out_Side = 0;	// 丢线控制
uchar Line_Count;	// 记录成功识别到的赛道行数

uchar Left_Add_Start, Right_Add_Start;	// 左右补线起始行坐标
uchar Left_Add_Stop, Right_Add_Stop;	// 左右补线结束行坐标
float Left_Ka = 0, Right_Ka = 0;
float Left_Kb = 0, Right_Kb = 0;	// 最小二乘法参数

uchar Left_Hazard_Flag, Right_Hazard_Flag;	// 左右障碍物标志
uchar Left_Max, Right_Min;
int32 Area_Left = 0, Area_Right = 0;	// 左右侧赛面积

uchar Starting_Line_Flag = 0;	// 起跑线标志位

/************** 环路相关变量 *************/
uchar Annulus_Count[10] = {1, 1, 1, 1, 0, 0, 1, 0, 1, 0};	// 1左环路，0右环路
uchar Annulus_Times = 0;	// 环路次数
uchar Annulus_Flag = 0;
uchar Annulus_Left = 0;
uchar Annulus_Right = 0;
int16 Annulus_Delay = 0;
uchar Annulus_Mode = 0;		// 准备进环路状态
/************** 环路相关变量 *************/

void Annulus_Control(void)
{
	Annulus_Left = Annulus_Count[Annulus_Times++];
	Annulus_Right = !Annulus_Left;
	
	Annulus_Mode = 1;	// 开始进入环路
	Annulus_Flag = 1;	// 环路标志位置位
	
	if (Annulus_Times >= 8)
	{
		Annulus_Times = 0;
	}
}

/****************** 新算法 ******************/

/*
*	图像算法参数初始化
*
*	说明：仅影响第一行特殊处理
*/
void Image_Para_Init(void)
{	
	Mid_Line[61] = 80;
	Left_Line[61] = 1;
	Right_Line[61] = 159;
	Left_Add_Line[61] = 1;
	Right_Add_Line[61] = 159;
	Width_Real[61] = 158;
	Width_Add[61] = 158;
	
	Annulus_Left = 1;	// 置位左环路
	Annulus_Right = 0; 
}

/*
*	环路图像处理算法
*
*	说明：环路专用，不含其他元素
*/
void Annulus_Handle(uchar *data)
{
	uchar i;	// 控制行
	uchar res;
	uchar Width_Max = 0;
	uchar Left_Min, Right_Max;
	uchar Damp;
	
	Line_Count = 0;	// 赛道行数复位
	
	Left_Add_Start = 0;		// 复位补线起始行坐标
	Right_Add_Start = 0;
	Left_Add_Stop = 0;
	Right_Add_Stop = 0;
	
	/***************************** 第一行特殊处理 *****************************/
	res = First_Line_Handle(data);
	if (res == 0)
	{
		Out_Side++;	// 丢线
		return;
	}
	if (Left_Add_Start || Right_Add_Start)
	{
		if (Width_Real[59] > 125)
		{
			Width_Min = 110;
			if (Annulus_Left)	// 左环路
			{
				Right_Add_Line[59] = Left_Add_Line[59] + Width_Min;
			}
			else				// 右环路
			{
				Left_Add_Line[59] = Right_Add_Line[59] - Width_Min;
			}
			Mid_Line[59] = (Right_Add_Line[59] + Left_Add_Line[59]) / 2;
		}
	}
	Width_Max = Width_Min;
	Left_Min = Left_Add_Line[59];
	Right_Max = Right_Add_Line[59];
	/*************************** 第一行特殊处理结束 ***************************/
	
	for (i = 59; i >= 15;)
	{
		i -= 2;
		
		if (!data[i*160 + Mid_Line[i+2]])//前2行中点在本行为黑点，赛道结束
		{ 
			i += 2;
			break;
		}
		else
		{
			Traversal_Mid_Line(i, data, Mid_Line[i+2], 1, 159, Left_Line, Right_Line, Left_Add_Line, Right_Add_Line);
			
			if (Width_Max > Width_Real[i])
			{
				Width_Max = Width_Real[i];
			}
				
			/************************** 补线检测开始 **************************/
			if (Width_Real[i] >= Width_Min+3)	// 赛道宽度变宽,进入环路
			{
				if (Annulus_Mode == 1)			// 准备进入环路
				{
					Damp = 2;
				}
				else if (Annulus_Mode == 2)	// 正在进入环路
				{
					Damp = 2;
				}
				else if (Annulus_Mode == 3)	// 成功进入环路
				{
					Damp = 0;
				}
				else if (Annulus_Mode == 4)	// 正在出环路
				{
					Damp = 0;
				}
				
				if (Annulus_Left)	// 左环路
				{
					Left_Add_Line[i] = Left_Line[i];
					Right_Add_Line[i] = Left_Line[i] + (Width_Min-Damp);
					
				}
				else				// 右环路
				{
					Right_Add_Line[i] = Right_Line[i];
					Left_Add_Line[i] = Right_Line[i] - (Width_Min-Damp);
				}
			}
			/************************** 补线检测结束 **************************/
			Mid_Line[i] = (Right_Add_Line[i] + Left_Add_Line[i]) / 2;
			Width_Add[i] = Right_Add_Line[i] - Left_Add_Line[i];	
			if (Width_Add[i] < Width_Min)
			{
				Width_Min = Width_Add[i];
			}
			if (Width_Add[i] <= 20)
			{
				i += 2;
				break;
			}
		}
		if (Left_Add_Line[i] < Left_Min)
		{
			Left_Min = Left_Add_Line[i];
		}
		if (Right_Add_Line[i] > Right_Max)
		{
			Right_Max = Right_Add_Line[i];
		}
	}
	Line_Count = i;
	
//	for (i = 61; i >= Line_Count+2;)
//	{
//		i -= 2;
//		/**//************************* 上位机显示边界 *************************/
//		/**/data[i*160 + Left_Add_Line[i] + 2] = 0;	// 上位机显示补线后的左边界，不用时屏蔽
//		/**/data[i*160 + Right_Add_Line[i] - 2] = 0;	// 上位机显示补线后的右边界，不用时屏蔽
//		/**/data[i*160 + Mid_Line[i]] = 0;			// 上位机显示中线，不用时屏蔽
//		/**/data[i*160 + Left_Line[i] + 2] = 0;		// 上位机显示原始左边界，不用时屏蔽
//		/**/data[i*160 + Right_Line[i] - 2] = 0;		// 上位机显示原始右边界，不用时屏蔽
//		/**//************************* 上位机显示边界 *************************/
//	}
	
	/**************************** 环路出入状态判断 ****************************/
	if (Annulus_Left)	// 左环路
	{
		if (Right_Add_Flag[59] && Right_Add_Flag[57] && Right_Add_Flag[55])		// 最底行需要补线
		{
			if (Annulus_Mode == 1)	// 上次状态为开始进入环路
			{
				Annulus_Mode = 2;	// 当前状态为进入环路中
				Annulus_Delay = 10;
			}
			if (Annulus_Mode == 3)	// 上次状态为已成功进入环路
			{
//				if ((Left_Add_Line[59] > Left_Add_Line[Line_Count]+10) && (Right_Add_Line[59] > Right_Add_Line[Line_Count]))	// 左边界是向左延伸的，消除进环路误判
				if (Left_Add_Line[59] >= Left_Min)	// 左边界是向左延伸的，消除进环路误判
				{
//					if (Right_Add_Flag[59] || Right_Add_Flag[57])
//					{
						Annulus_Mode = 4;	// 当前状态为出环路中
						Annulus_Delay = 250;	// 最少延时1s
						
						led(LED_MAX,LED_OFF);// 关闭LED指示灯
//					}
				}
			}
		}
		else						// 最底行不需要补线
		{
			if (Annulus_Mode == 2)	// 上次状态为进入环路中
			{
				if (!Annulus_Delay)
				{
					Annulus_Mode = 3;	// 当前状态为已成功进入环路，准备出环路
					
					led(LED_MAX,LED_ON);// 打开LED指示灯
				}
			}
			if (Annulus_Mode == 4)	// 上次状态为出环路中
			{
				if (!Annulus_Delay)	// 最少保持1s环路专用算法，防止误判
				{
					if (!Right_Add_Flag[59] && !Right_Add_Flag[57] && !Right_Add_Flag[55])
					Annulus_Mode = 0;	// 本次状态为成功出环路，准备下次进入环路
					Annulus_Flag = 0;	// 清除环路状态标志位
				}
			}
		}
	}
	else	// 右环路
	{
		if (Left_Add_Flag[59] && Left_Add_Flag[57] && Left_Add_Flag[55])		// 最底行需要补线
		{
			if (Annulus_Mode == 1)	// 上次状态为开始进入环路
			{
				Annulus_Mode = 2;	// 当前状态为进入环路中
				Annulus_Delay = 10;
			}
			if (Annulus_Mode == 3)	// 上次状态为已成功进入环路
			{
//				if ((Right_Add_Line[59]+10 < Right_Add_Line[Line_Count]) && (Left_Add_Line[59] < Left_Add_Line[Line_Count]))	// 右边界是向右延伸的，消除进环路误判
				if (Right_Add_Line[59] <= Right_Max)	// 右边界是向右延伸的，消除进环路误判
				{
//					if (Left_Add_Flag[59] || Left_Add_Flag[57])
//					{
						Annulus_Mode = 4;	// 当前状态为出环路中
						Annulus_Delay = 250;	// 最少延时1s
						
						led(LED_MAX,LED_OFF);// 关闭LED指示灯
//					}
				}
			}
		}
		else						// 最底行不需要补线
		{
			if (Annulus_Mode == 2)	// 上次状态为进入环路中
			{
				if (!Annulus_Delay)
				{
					Annulus_Mode = 3;	// 当前状态为已成功进入环路，准备出环路
					
					led(LED_MAX,LED_ON);// 打开LED指示灯
				}
			}
			if (Annulus_Mode == 4)	// 上次状态为出环路中
			{
				if (!Annulus_Delay)	// 本次状态为成功出环路，准备下次进入环路
				{
					if (!Left_Add_Flag[59] && !Left_Add_Flag[57] && !Left_Add_Flag[55])
					{
						Annulus_Mode = 0;	// 本次状态为成功出环路，准备下次进入环路
						Annulus_Flag = 0;	// 清除环路状态标志位
						
	//					led(LED_MAX,LED_OFF);// 关闭LED指示灯
					}
				}
			}
		}
	}
	/************************** 环路出入状态判断结束 **************************/
}

/*
*	图像处理算法
*
*	说明：处理普通图像，包括十字、障碍
*/
void Image_Handle(uchar *data)
{
	uchar i;	// 控制行
	uchar res;	// 用于结果状态判断
	float Result;	// 用于结果状态判断
	uchar Add_Start_Max;
	uchar Width_Check;
	uchar Weight_Left, Weight_Right;
	uchar Mid_Left, Mid_Right;
	uchar Limit_Left, Limit_Right;
	static uchar Annulus_Count = 0;	// 环路检测计数，防止误判
	
	Line_Count = 0;	// 赛道行数复位
	Starting_Line_Flag = 0;	// 起跑线标志位复位
	
	Annulus_Flag = 0;		// 复位环路标志位
	
	Left_Hazard_Flag = 0;	// 复位左右障碍物标志位
	Right_Hazard_Flag = 0;
	
	Left_Add_Start = 0;		// 复位补线起始行坐标
	Right_Add_Start = 0;
	Left_Add_Stop = 0;
	Right_Add_Stop = 0;
	
	/***************************** 第一行特殊处理 *****************************/
	
	res = First_Line_Handle(data);
	if (res == 0)
	{
		Out_Side = 1;	// 丢线
		return;
	}
	Out_Side = 0;
	Line_Count = 59;
	
	/*************************** 第一行特殊处理结束 ***************************/
	
	for (i = 59; i >= 15;)	// 仅处理前40行图像，隔行后仅处理20行数据
	{
		i -= 2;	// 隔行处理，减小单片机负荷
		
		if (Left_Add_Flag[i+2])
		{
			Limit_Left = Left_Line[i+2];
		}
		else
		{
			if (Jump[i] >= 2)
			{
				Limit_Left = Left_Add_Line[i+2];
			}
			else
			{
				Limit_Left = Left_Add_Line[i+2]+1;
			}
			
		}
		
		if (Right_Add_Flag[i+2])
		{
			Limit_Right = Right_Line[i+2];
		}
		else
		{
			if (Jump[i] >= 2)
			{
				Limit_Right = Right_Add_Line[i+2];
			}
			else
			{
				Limit_Right = Right_Add_Line[i+2]-1;
			}
		}
		Jump[i] = Corrode_Filter(i, data, Limit_Left, Limit_Right);	// 使用腐蚀滤波算法先对本行赛道进行预处理，返回跳变点数量
		if (Jump[i] >= 5 && i>= 21)
		{
			Starting_Line_Flag = 1;
		}
			
		if (!data[i*160 + Mid_Line[i+2]])//前2行中点在本行为黑点，可能是赛道结束，也可能是环路
		{
			if (Left_Add_Start && !Left_Add_Stop && Right_Add_Start && !Right_Add_Stop)	//  两边都有补线，但没有补线结束，即遇到了环路
			{ 
				if (Left_Add_Start >= Right_Add_Start)
				{
					Add_Start_Max = Left_Add_Start;
				}
				else
				{
					Add_Start_Max = Right_Add_Start;
				}
				if (Width_Real[i+2] > Width_Real[Add_Start_Max] && res == 1)
				{
					if (Left_Ka <= 0 && Right_Ka >= 0)
					{
						if (Left_Add_Start >= 21 && Left_Add_Start <= 55 && Right_Add_Start >= 21 && Right_Add_Start <= 55)
						{
							if (Left_Add_Start >= i+4 && Right_Add_Start >= i+4)
							{
								if (Left_Ka <= -1.2|| Right_Ka >= 1.2)
								{	
									break;									
								}
								else
								{
									Annulus_Control();	// 启动环路专用算法
									break;
								}
							}
						}
					}
				}
			}
			break;
		}
		else	// 使用前2行中点向两边扫描边界
		{
			Traversal_Mid_Line(i, data, Mid_Line[i+2], 1, 159, Left_Line, Right_Line, Left_Add_Line, Right_Add_Line);
		}
			
		/**************************** 补线检测开始 ****************************/
		if (Starting_Line_Flag)
		{
			Width_Check = 7;
		}
		else
		{
			Width_Check = 2;
		}
		if (Width_Real[i] > Width_Min+Width_Check)	// 赛道宽度变宽，可能是十字或环路
		{
			if (Left_Add_Line[i] <  Left_Add_Line[i+2])
			{
				if (!Left_Add_Flag[i])
				{
					Left_Add_Flag[i] = 1;	// 强制认定为需要补线
				}
			}
			if (Right_Add_Line[i] > Right_Add_Line[i+2])
			{
				if (!Right_Add_Flag[i])
				{
					Right_Add_Flag[i] = 1;	// 强制认定为需要补线
				}
			}
			
			if (Left_Add_Flag[i] || Right_Add_Flag[i])
			{
				if (Left_Add_Stop && Right_Add_Stop)
				{
					break;
				}
			}
		}
		
		/**************************** 补线检测结束 ****************************/
		
		
		/**************************** 障碍检测开始 ****************************/
		
		else
		{
			if ((i != 59) && (Width_Real[i]+12 < Width_Real[i+2]) && (Jump[i] == 2))
			{
				if (Left_Add_Line[i] > Left_Add_Line[i+2] + 6 && !Left_Add_Start)	// 本行与前一行左边界有较大突变且没有补线
				{
					Left_Hazard_Flag = i;	// 障碍物标志位置位
				}
				if (Right_Add_Line[i] < Right_Add_Line[i+2] - 6 && !Right_Add_Start)	// 本行与前一行右边界有较大突变且没有补线
				{
					Right_Hazard_Flag = i;	// 障碍物标志位置位
				}
				
				
			}
		}
		
		/**************************** 障碍检测结束 ****************************/
		
		
		/*************************** 第一轮补线开始 ***************************/
		
		if (Left_Add_Flag[i])	// 左侧需要补线
		{
			if (i >= 53)	// 前三行补线不算
			{
				if (!Left_Add_Start)
				{
					Left_Add_Start = i;	// 记录补线开始行
					Left_Ka = 0;
					Left_Kb = Left_Add_Line[i+2];
				}
				Left_Add_Line[i] = Calculate_Add(i, Left_Ka, Left_Kb);	// 使用前一帧图像左边界斜率补线
			}
			else
			{
				if (!Left_Add_Start)	// 之前没有补线
				{
					Left_Add_Start = i;	// 记录左侧补线开始行
					if (Left_Hazard_Flag)	// 左侧有障碍物
					{
						Left_Ka = 0;
						Left_Kb = Left_Add_Line[i+2];	// 竖直向上补线
					}
					else	// 斜率补线
					{
						Curve_Fitting(&Left_Ka, &Left_Kb, &Left_Add_Start, Left_Add_Line, Left_Add_Flag, 1);	// 使用两点法拟合直线
					}
				}
				Left_Add_Line[i] = Calculate_Add(i, Left_Ka, Left_Kb);	// 补线完成
			}
		}
		else
		{
			if (Left_Add_Start)	// 已经开始补线
			{
				if (!Left_Add_Stop && !Left_Add_Flag[i+2])
				{
					if (Left_Add_Line[i] >= Left_Add_Line[i+2])
					{
						Left_Add_Stop = i;	// 记录左侧补线结束行
					}
				}
			}
		}
		
		if (Right_Add_Flag[i])	// 右侧需要补线
		{
			if (i >= 53)	// 前三行补线不算
			{
				if (!Right_Add_Start)
				{
					Right_Add_Start = i;	// 记录补线开始行
					Right_Ka = 0;
					Right_Kb = Right_Add_Line[i+2];
				}
				Right_Add_Line[i] = Calculate_Add(i, Right_Ka, Right_Kb);	// 使用前一帧图像右边界斜率补线
			}
			else
			{
				if (!Right_Add_Start)	// 之前没有补线
				{
					Right_Add_Start = i;	// 记录右侧补线开始行
					if (Right_Hazard_Flag)	// 右侧有障碍物
					{
						Right_Ka = 0;
						Right_Kb = Right_Add_Line[i+2];	// 竖直向上补线
					}
					else
					{
						Curve_Fitting(&Right_Ka, &Right_Kb, &Right_Add_Start, Right_Add_Line, Right_Add_Flag, 2);	// 使用两点法拟合直线
					}
				}
				Right_Add_Line[i] = Calculate_Add(i, Right_Ka, Right_Kb);	// 补线完成
			}
		}
		else
		{
			if (Right_Add_Start)	// 已经开始补线
			{
				if (!Right_Add_Stop && !Right_Add_Flag[i+2])
				{
					if (Right_Line[i] <= Right_Line[i+2])
					{
						Right_Add_Stop = i;	// 记录右侧补线结束行
					}
				}
			}
		}
		
		/*************************** 第一轮补线结束 ***************************/
		
//		if ((Left_Add_Flag[i] && Right_Add_Flag[i]) || (!Left_Add_Flag[i] && !Right_Add_Flag[i]))
//		{
			Width_Add[i] = Right_Add_Line[i] - Left_Add_Line[i];	// 重新计算赛道宽度
			Mid_Line[i] = (Right_Add_Line[i] + Left_Add_Line[i]) / 2;	// 计算中线
//		}
//		else
//		{
//			Width_Add[i] = Right_Add_Line[i] - Left_Add_Line[i];	// 重新计算赛道宽度
//			Mid_Line[i] = Mid_Line[i+2];
//		}

		if (Width_Add[i] < Width_Min)
		{
			Width_Min = Width_Add[i];	// 更新最小赛道宽度
		}
		if (Width_Add[i] <= 20)	// 赛道宽度太小
		{
			break;
		}
		Line_Count = i;	// 记录成功识别到的赛道行数
	}
	/*************************** 第二轮补线修复开始 ***************************/
	if (!Annulus_Flag)	// 不是环路
	{
		if (Left_Add_Start)		// 左边界需要补线
		{
			Line_Repair(Left_Add_Start, Left_Add_Stop, data, Left_Line, Left_Add_Line, Left_Add_Flag, 1);
		}
		if (Right_Add_Start)	// 右边界需要补线
		{
			Line_Repair(Right_Add_Start, Right_Add_Stop, data, Right_Line, Right_Add_Line, Right_Add_Flag, 2);
		}
	}
	
	/*************************** 第二轮补线修复结束 ***************************/
	
	
	/****************************** 中线修复开始 ******************************/
	
	Mid_Line_Repair(Line_Count, data);
	
	/****************************** 中线修复结束 ******************************/
}


uchar Fres = 0;	// 前瞻
void Fresight_Calculate(void)
{
	if (Speed_Min < 140)
	{
		Fres = 0;
	}
	else if (Speed_Min < 150)
	{
		Fres = 1;
	}
	else if (Speed_Min < 160)
	{
		Fres = 2;
	}
	else if (Speed_Min < 170)
	{
		Fres = 3;
	}
	else if (Speed_Min >= 170)
	{
		Fres = 4;
	}
}


uchar Fresight[5][2] = {{21, 35},	// 130	前瞻控制
						{21, 35},	// 140
						{21, 33},	// 150
						{21, 31},	// 160
						{19, 29}};	// 170
/*
*	中线平均
*
*	说明：不做特殊处理，直接取均值
*/
int32 Point_Average(void)
{
	uchar i, Point;
	int32 Sum = 0;
	static uchar Last_Point = 80;
	
	Fresight_Calculate();	// 计算前瞻
	
	if (Line_Count <= Fresight[Fres][0])
	{
		Line_Count = Fresight[Fres][0];
	}
	
	if (Annulus_Flag)
	{
		if (Line_Count <= Fresight[Fres][1])
		{
			Line_Count = Fresight[Fres][1];
		}
	}
	
	if (Out_Side || Line_Count >= 57)	// 出界或者摄像头图像异常
	{
		if (Last_Point <= 80)
		{
			Point  = 1;			// 使用上次目标点
		}
		else
		{
			Point = 159;
		}
	}
	else
	{
		
		for (i = 61; i >= Line_Count+2;)
		{
			i -= 2;
			Sum += Mid_Line[i];
		}

		Point = Sum / ((61-Line_Count)/2);	// 对中线求平均
		
		Point = Point*0.8 + Last_Point*0.2;	// 低通滤波
		Point = range_protect(Point, 1, 159);		// 限幅，防止补偿溢出
		
		/*** 障碍物特殊情况处理 ***/
		if (Left_Hazard_Flag)			//左侧有障碍物且需要补线，即使误判也不会造成影响
		{
			Point = Mid_Line[Left_Hazard_Flag];	//使用障碍物出现的那一行中点作为目标点
//			if (Left_Hazard_Flag < 40)
//			{
//				Point += 5;
//			}
		}
		else if (Right_Hazard_Flag)	//右测有障碍物且需要补线，即使误判也不会造成影响
		{
			Point = Mid_Line[Right_Hazard_Flag];//使用障碍物出现的那一行中点作为目标点
//			if (Right_Hazard_Flag < 40)
//			{
//				Point -= 5;
//			}
		}
		Last_Point = Point;	// 更新上次目标点
	}
	
	return (Point-77);
}


/*
*	面积算法
*
*	说明：计算左右面积
*/
int32 Area_Calculate(void)
{
	char i;
	uchar Line_Start = 61;
	uint16 Area_Mid = 79;
	int32 Result;
	static int32 Result_Last = 0;
	
	Area_Left = 0;	// 左侧面积清零
	Area_Right = 0;	// 右侧面积清零
	
	Fresight_Calculate();	// 计算前瞻

	if (Line_Count <= Fresight[Fres][0])
	{
		Line_Count = Fresight[Fres][0];
	}
	
	if (Annulus_Flag)
	{
		if (Line_Count <= Fresight[Fres][1])
		{
			Line_Count = Fresight[Fres][1];
		}
	}
	
	for (i = Line_Start; i >= Line_Count+2;)
	{
		i -= 2;
		
		if (Mid_Line[i] == Area_Mid)
		{
			Area_Left += (Area_Mid - Left_Add_Line[i]);
			Area_Right += (Right_Add_Line[i] - Area_Mid);
		}
		else if (Right_Add_Line[i] <= Area_Mid)	// 仅左侧有赛道
		{
			Area_Left += Width_Add[i];
		}
		else if (Left_Add_Line[i] >= Area_Mid)	// 仅右侧有赛道
		{
			Area_Right += Width_Add[i];
		}
		else
		{
			Area_Left += (Area_Mid - Left_Add_Line[i]);
			Area_Right += (Right_Add_Line[i] - Area_Mid);
		}
	}
	
	Result = 50*(Area_Right - Area_Left)/(Area_Right + Area_Left);
	Result = range_protect(Result, -40, 40);
	
	Result = Result * 0.8 + Result_Last * 0.2;
	Result_Last = Result;
	
	return Result;
}

/*
*	最小二乘法计算斜率
*
*	说明：返回斜率作为转向控制量,结果放大100倍
*/
int32 Least_Squares(void)
{
	int32 Ave_x, Ave_y, Sum1, Sum2;
	uchar i;
	float f_Slope;
	int32 i_Slope;
	static int32 i_Last_Slope = 0;
	
	for (i = 61; i >= Line_Count; )
	{
		i -= 2;
	}
	i_Last_Slope = i_Slope;
	
	return i_Slope;
}

/*
*	斜率计算
*
*	说明：使用斜率作为转向控制量，返回斜率的倒数
*/
float Slope_Weight(uchar *Mid)
{
	float Slope;
	
	Slope = 1.0 * (Mid[Line_Count] - 80) / (60-Line_Count);
	
	return Slope;
}

/*
*	赛道角度计算
*
*	说明：返回结果为 右：0：180°，1：135°，2：90°，>= 1可能为直角突变
*					 左：0：180°，-1：135°，-2：90°，<= -1可能为直角突变
*/
float Calculate_Angle(uchar Point_1, uchar Point_2, uchar Point_3)
{
	char K1, K2;
	float Result;
	
	K1 = Point_2 - Point_1;
	K2 = Point_3 - Point_2;
	
	Result = (K2 - K1) * 0.5;
	
	return Result;
}

/*
*	两点法求直线
*
*	说明：拟合直线 y = Ka * x + Kb   Mode == 1代表左边界，Mode == 2代表右边界
*/
void Curve_Fitting(float *Ka, float *Kb, uchar *Start, uchar *Line, uchar *Add_Flag, uchar Mode)
{
	*Start += 4;
	if (Add_Flag[*Start] == 2)
	{
		if (*Start <= 51)
		{
			*Start += 2;
		}
		*Ka = 1.0*(Line[*Start+4] - Line[*Start]) / 4;
		if (Mode == 2)
		{
			if (*Ka < 0)
			{
				*Ka = 0;
			}
		}
		if (Mode == 1)
		{
			if (*Ka > 0)
			{
				*Ka = 0;
			}
		}
	}
	else
	{
		*Ka = 1.0*(Line[*Start+4] - Line[*Start]) / 4;
	}
	*Kb = 1.0*Line[*Start] - (*Ka * (*Start));
}

/*
*	计算补线坐标
*
*	说明：使用两点法计算拟合出的补线坐标
*/
uchar Calculate_Add(uchar i, float Ka, float Kb)	// 计算补线坐标
{
	float res;
	int32 Result;
	
	res = i * Ka + Kb;
	Result = range_protect((int32)res, 1, 159);
	
	return (uchar)Result;
}

uchar Test_Jump;
/*
*	腐蚀滤波
*
*	说明：将干扰滤除，并统计黑白跳变点数量，用于起跑线检测
*/
uchar Corrode_Filter(uchar i, uchar *data, uchar Left_Min, uchar Right_Max)
{
	uchar j;
	uchar White_Flag = 0;
	uchar Jump_Count = 0;	// 跳变点计数
	
	Test_Jump = 0;
	
	Right_Max = range_protect(Right_Max, 1, 159);	// 保留右侧部分区域，防止溢出
	
	for (j = Left_Min; j <= Right_Max; j++)	// 从左向右扫描，方向不影响结果
	{
		if (!White_Flag)	// 先查找白点，只滤黑点，不滤白点
		{
			if (data[i*160 + j])	// 检测到白点
			{
				White_Flag = 1;	// 开始找黑点
			}
		}
		else
		{
			if (!data[i*160 + j])	// 检测到黑点
			{
				Jump_Count++;	// 视为一次跳变
				
				Test_Jump = Jump_Count;
				
				if (!data[i*160 + j+1] && j+1 <= Right_Max)	// 连续两个黑点
				{
					if (!data[i*160 + j+2] && j+2 <= Right_Max)	// 连续三个黑点
					{
						if (!data[i*160 + j+3] && j+3 <= Right_Max)	// 连续四个黑点
						{
							if (!data[i*160 + j+4] && j+4 <= Right_Max)	// 连续五个黑点
							{
								if (!data[i*160 + j+5] && j+5 <= Right_Max)	// 连续六个黑点
								{
									if (!data[i*160 + j+6] && j+6 <= Right_Max)	// 连续七个黑点
									{
										if (!data[i*160 + j+7] && j+7 <= Right_Max)	// 连续八个黑点
										{
											if (!data[i*160 + j+8] && j+8 <= Right_Max)	// 连续九个黑点
											{
												if (!data[i*160 + j+9] && j+9 <= Right_Max)	// 连续十个黑点
												{
													if (!data[i*160 + j+10] && j+10 <= Right_Max)	// 连续11个黑点
													{
														White_Flag = 0;	// 认为不是干扰，不做任何处理，下次搜索白点
														j += 10;
													}
													else if (j+10 <= Right_Max)
													{
														data[i*160 + j] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+1] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+2] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+3] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+4] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+5] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+6] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+7] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+8] = 255;	// 仅有连续10个黑点，滤除掉
														data[i*160+j+9] = 255;	// 仅有连续10个黑点，滤除掉
														
														j += 10;
													}
													else
													{
														j += 10;
													}
												}
												else if (j+9 <= Right_Max)
												{
													data[i*160 + j] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+1] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+2] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+3] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+4] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+5] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+6] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+7] = 255;	// 仅有连续九个黑点，滤除掉
													data[i*160+j+8] = 255;	// 仅有连续九个黑点，滤除掉
													
													j += 9;
												}
												else
												{
													j += 9;
												}
											}
											else if (j+8 <= Right_Max)
											{
												data[i*160 + j] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+1] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+2] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+3] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+4] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+5] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+6] = 255;	// 仅有连续八个黑点，滤除掉
												data[i*160+j+7] = 255;	// 仅有连续八个黑点，滤除掉
												
												j += 8;
											}	
											else
											{
												j += 8;
											}
										}
										else if (j+7 <= Right_Max)
										{
											data[i*160 + j] = 255;	// 仅有连续七个黑点，滤除掉
											data[i*160+j+1] = 255;	// 仅有连续七个黑点，滤除掉
											data[i*160+j+2] = 255;	// 仅有连续七个黑点，滤除掉
											data[i*160+j+3] = 255;	// 仅有连续七个黑点，滤除掉
											data[i*160+j+4] = 255;	// 仅有连续七个黑点，滤除掉
											data[i*160+j+5] = 255;	// 仅有连续七个黑点，滤除掉
											data[i*160+j+6] = 255;	// 仅有连续七个黑点，滤除掉										
											
											j += 7;
										}	
										else
										{
											j += 7;
										}
									}
									else if (j+6 <= Right_Max)
									{
										data[i*160 + j] = 255;	// 仅有连续六个黑点，滤除掉
										data[i*160+j+1] = 255;	// 仅有连续六个黑点，滤除掉
										data[i*160+j+2] = 255;	// 仅有连续六个黑点，滤除掉
										data[i*160+j+3] = 255;	// 仅有连续六个黑点，滤除掉
										data[i*160+j+4] = 255;	// 仅有连续六个黑点，滤除掉
										data[i*160+j+5] = 255;	// 仅有连续六个黑点，滤除掉
										
										j += 6;
									}	
									else
									{
										j += 6;
									}
								}
								else if (j+5 <= Right_Max)
								{
									data[i*160 + j] = 255;	// 仅有连续五个黑点，滤除掉
									data[i*160+j+1] = 255;	// 仅有连续五个黑点，滤除掉
									data[i*160+j+2] = 255;	// 仅有连续五个黑点，滤除掉
									data[i*160+j+3] = 255;	// 仅有连续五个黑点，滤除掉
									data[i*160+j+4] = 255;	// 仅有连续五个黑点，滤除掉									
									
									j += 5;
								}	
								else
								{
									j += 5;
								}
							}
							else if (j+4 <= Right_Max)
							{
								data[i*160 + j] = 255;	// 仅有连续四个黑点，滤除掉
								data[i*160+j+1] = 255;	// 仅有连续四个黑点，滤除掉
								data[i*160+j+2] = 255;	// 仅有连续四个黑点，滤除掉
								data[i*160+j+3] = 255;	// 仅有连续四个黑点，滤除掉
								
								j += 4;
							}	
							else
							{
								j += 4;
							}
						}
						else if (j+3 <= Right_Max)
						{
							data[i*160 + j] = 255;	// 仅有连续三个黑点，滤除掉
							data[i*160+j+1] = 255;	// 仅有连续三个黑点，滤除掉
							data[i*160+j+2] = 255;	// 仅有连续三个黑点，滤除掉
							
							j += 3;
						}	
						else
						{
							j += 3;
						}
					}
					else if (j+2 <= Right_Max)
					{
						data[i*160 + j] = 255;	// 仅有连续两个黑点，滤除掉
						data[i*160+j+1] = 255;	// 仅有连续两个黑点，滤除掉
						
						j += 2;
					}	
					else
					{
						j += 2;
					}
				}
				else if (j+1 <= Right_Max)
				{
					data[i*160 + j] = 255;	// 有一个黑点，滤除掉
					
					j += 1;
				}	
				else
				{
					j += 1;
				}
			}
		}
	}
//	if (White_Flag)
//	{
//		Jump_Count++;	// 视为一次跳变
//	}
	
	return Jump_Count;	// 返回跳变点计数
}

/*
*	尽头搜索
*
*	说明：从某一点开始竖直向上搜索，返回最远行坐标
*/
uchar Limit_Scan(uchar i, uchar *data, uchar Point)
{
	for ( ; i >= 20; i--)
	{
		if (!data[160*i + Point])	// 搜索到黑点
		{
			break;
		}
	}
	
	return i;	// 返回最远行坐标
}

/*
*	第一行特殊处理
*
*	说明：先使用第60行中点作为第59行(第一行)搜线起始位置，成功搜索到左右边界后
 	将第59行中点赋值给第60行便于下一帧图像使用。如果第60行中点在本行为黑点，再
 	分别使用左遍历和右遍历的方法搜索边界，以赛道宽度较大的结果作为第59行边界，
 	若仍然搜索不到边界或数据异常认为出界，出界返回0
*/
uchar First_Line_Handle(uchar *data)
{
	uchar i;	// 控制行
	uchar Weight_Left, Weight_Right;	// 左右赛道宽度
	uchar Mid_Left, Mid_Right;
	uchar res;
	
	i = 59;
	
	res = Corrode_Filter(i, data, 1, 159);	// 使用腐蚀滤波算法先对本行赛道进行预处理，返回跳变点数量
	Jump[59] = res;
	
	if (!data[i*160 + Mid_Line[61]])	// 第61行中点在第59行为黑点
	{
		Weight_Left = Traversal_Left(i, data, &Mid_Left, 1, 159);	// 从左侧搜索边界
		Weight_Right = Traversal_Right(i, data, &Mid_Right, 1, 159);// 从右侧搜索边界
		if (Weight_Left >= Weight_Right && Weight_Left)	// 左赛道宽度大于右赛道宽度且不为0
		{
			Traversal_Left_Line(i, data, Left_Line, Right_Line);	// 使用左遍历获取赛道边界
		}
		else if (Weight_Left < Weight_Right && Weight_Right)
		{
			Traversal_Right_Line(i, data, Left_Line, Right_Line);	// 使用右遍历获取赛道边界
		}
		else	// 说明没查到
		{
			return 0;
		}
	}
	else
	{
		Traversal_Mid_Line(i, data, Mid_Line[i+2], 1, 159, Left_Line, Right_Line, Left_Add_Line, Right_Add_Line);	// 从前一行中点向两边扫描
	}
	
	Left_Line[61] = Left_Line[59];
	Right_Line[61] = Right_Line[59];
	Left_Add_Line[61] = Left_Add_Line[59];
	Right_Add_Line[61] = Right_Add_Line[59];
	if (Left_Add_Flag[59] && Right_Add_Flag[59])
	{
		Mid_Line[59] = Mid_Line[61];
	}
	else
	{
		Mid_Line[59] = (Right_Line[59] + Left_Line[59]) / 2;
		Mid_Line[61] = Mid_Line[59];	// 更新第60行虚拟中点，便于下一帧图像使用
	}
	if (Left_Add_Flag[59])
	{
		Left_Add_Start = i;	// 记录补线开始行
		Left_Ka = 0;
		Left_Kb = Left_Add_Line[59];
	}
	if (Right_Add_Flag[i])
	{
		Right_Add_Start = i;	// 记录补线开始行
		Right_Ka = 0;
		Right_Kb = Right_Add_Line[59];
	}
	
	Width_Real[61] = Width_Real[59];
	Width_Add[61] = Width_Add[59];
	Width_Min = Width_Add[59];
	
	return 1;
}

/*
*	从左侧开始搜索边界，返回赛道宽度
*
*	说明：本函数仅仅作为探测赛道使用，仅返回赛道宽度，不保存边界数据
*/
uchar Traversal_Left(uchar i, uchar *data, uchar *Mid, uchar Left_Min, uchar Right_Max)
{
	uchar j, White_Flag = 0;
	uchar Left_Line = Left_Min, Right_Line = Right_Max;
	
	for (j = Left_Min; j <= Right_Max; j++)	// 边界坐标 1到159
	{
		if (!White_Flag)	// 先查找左边界
		{
			if (data[i*160 + j])	// 检测到白点
			{
				Left_Line = j;	// 记录当前j值为本行左边界
				White_Flag = 1;	// 左边界已找到，必有右边界
				
				continue;	// 结束本次循环，进入下次循环
			}
		}
		else
		{
			if (!data[i*160 + j])//检测黑点
			{
				Right_Line = j-1;//记录当前j值为本行右边界
				
				break;	// 左右边界都找到，结束循环
			}
		}
	}
	
	if (!White_Flag)	// 未找到左右边界
	{
		return 0;
	}
	else
	{
		*Mid = (Right_Line + Left_Line) / 2;
		
		return (Right_Line - Left_Line);
	}
}

/*
*	从右侧开始搜索边界，返回赛道宽度
*
*	说明：本函数仅仅作为探测赛道使用，仅返回赛道宽度，不保存边界数据
*/
uchar Traversal_Right(uchar i, uchar *data, uchar *Mid, uchar Left_Min, uchar Right_Max)
{
	uchar j, White_Flag = 0;
	uchar Left_Line = Left_Min, Right_Line = Right_Max;
	
	for (j = Right_Max; j >= Left_Min; j--)	// 边界坐标 1到159
	{
		if (!White_Flag)	// 先查找右边界
		{
			if (data[i*160 + j])	// 检测到白点
			{
				Right_Line = j;	// 记录当前j值为本行右边界
				White_Flag = 1;	// 右边界已找到，必有左边界
				
				continue;	// 结束本次循环，进入下次循环
			}
		}
		else
		{
			if (!data[i*160 + j])	//检测黑点
			{
				Left_Line = j+1;	//记录当前j值为本行左边界
				
				break;	// 左右边界都找到，结束循环
			}
		}
	}
	
	if (!White_Flag)	// 未找到左右边界
	{
		return 0;
	}
	else
	{
		*Mid = (Right_Line + Left_Line) / 2;
			
		return (Right_Line - Left_Line);
	}
}

/*
*	从左侧开始搜索边界，保存赛道边界，返回1成功 0失败
*
*	说明：本函数使用后将保存边界数据
*/
uchar Traversal_Left_Line(uchar i, uchar *data, uchar *Left_Line, uchar *Right_Line)
{
	uchar j, White_Flag = 0;
	
	Left_Line[i] = 1;
	Right_Line[i] = 159;
	
	for (j = 1; j < 160; j++)	// 边界坐标 1到159
	{
		if (!White_Flag)	// 先查找左边界
		{
			if (data[i*160 + j])	// 检测到白点
			{
				Left_Line[i] = j;	// 记录当前j值为本行左边界
				White_Flag = 1;		// 左边界已找到，必有右边界
				
				continue;	// 结束本次循环，进入下次循环
			}
		}
		else
		{
			if (!data[i*160 + j])	//检测黑点
			{
				Right_Line[i] = j-1;//记录当前j值为本行右边界
				
				break;	// 左右边界都找到，结束循环
			}
		}
	}
	if (White_Flag)
	{
		Left_Add_Line[i] = Left_Line[i];
		Right_Add_Line[i] = Right_Line[i];
		Width_Real[i] = Right_Line[i] - Left_Line[i];
		Width_Add[i] = Width_Real[i];
	}
	
	return White_Flag;	// 返回搜索结果
}

/*
*	从右侧开始搜索边界，保存赛道边界，返回1成功 0失败
*
*	说明：本函数使用后将保存边界数据
*/
uchar Traversal_Right_Line(uchar i, uchar *data, uchar *Left_Line, uchar *Right_Line)
{
	uchar j, White_Flag = 0;
	
	Left_Line[i] = 1;
	Right_Line[i] = 159;
	
	for (j = 159; j > 0; j--)	// 边界坐标 1到159
	{
		if (!White_Flag)	// 先查找右边界
		{
			if (data[i*160 + j])	// 检测到白点
			{
				Right_Line[i] = j;	// 记录当前j值为本行右边界
				White_Flag = 1;		// 右边界已找到，必有左边界
				
				continue;	// 结束本次循环，进入下次循环
			}
		}
		else
		{
			if (!data[i*160 + j])	//检测黑点
			{
				Left_Line[i] = j+1;//记录当前j值为本行左边界
				
				break;	// 左右边界都找到，结束循环
			}
		}
	}
	if (White_Flag)
	{
		Left_Add_Line[i] = Left_Line[i];
		Right_Add_Line[i] = Right_Line[i];
		Width_Real[i] = Right_Line[i] - Left_Line[i];
		Width_Add[i] = Width_Real[i];
	}
	
	return White_Flag;	// 返回搜索结果
}

/*
*	从中间向两边搜索边界
*
*	说明：本函数使用后将保存边界数据
*/
void Traversal_Mid_Line(uchar i, uchar *data, uchar Mid, uchar Left_Min, uchar Right_Max, uchar *Left_Line, uchar *Right_Line, uchar *Left_Add_Line, uchar *Right_Add_Line)
{
	uchar j;
	
	Left_Add_Flag[i] = 1;	// 初始化补线标志位
	Right_Add_Flag[i] = 1;
	
	Left_Min = range_protect(Left_Min, 1, 159);	// 限幅，防止出错
	if (Left_Add_Flag[i+2])
	{
		Left_Min = range_protect(Left_Min, Left_Add_Line[i+2]-30, 159);
	}
	Right_Max = range_protect(Right_Max, 1, 159);
	if (Right_Add_Flag[i+2])
	{
		Right_Max = range_protect(Right_Max, 1, Right_Add_Line[i+2]+30);
	}
	
	Right_Line[i] = Right_Max;
	Left_Line[i] = Left_Min;	// 给定边界初始值
	
	for (j = Mid; j >= Left_Min; j--)	// 以前一行中点为起点向左查找边界
	{
		if (!data[i*160 + j])	// 检测到黑点
		{
			Left_Add_Flag[i] = 0;	//左边界不需要补线，清除标志位
			Left_Line[i] = j+1;	//记录当前j值为本行实际左边界
			Left_Add_Line[i] = j+1;	// 记录实际左边界为补线左边界
			
			break;
		}
	}
	for (j = Mid; j <= Right_Max; j++)	// 以前一行中点为起点向右查找右边界
	{
		if (!data[i*160 + j])	//检测到黑点
		{
			Right_Add_Flag[i] = 0;		//右边界不需要补线，清除标志位
			Right_Line[i] = j-1;	//记录当前j值为本行右边界
			Right_Add_Line[i] = j-1;	// 记录实际右边界为补线左边界
			
			break;
		}
	}
	if (Left_Add_Flag[i+2])	// 左边界前一行需要补线
	{
		if (Left_Add_Line[i] <= Left_Add_Line[i+2])	// 本行限定就要严格
		{
			Left_Add_Flag[i] = 1;
		}
	}
	if (Right_Add_Flag[i+2])// 右边界前一行需要补线
	{
		if (Right_Add_Line[i] >= Right_Add_Line[i+2])	// 本行限定就要严格
		{
			Right_Add_Flag[i] = 1;
		}
	}
	if (Left_Add_Flag[i])	// 左边界需要补线
	{
		if (!data[(i-2)*160 + Left_Add_Line[i+2]] || !data[(i-4)*160 + Left_Add_Line[i+2]])	// 可能是反光干扰
		{
			Left_Add_Flag[i] = 0;	//左边界不需要补线，清除标志位
			Left_Line[i] = Left_Add_Line[i+2];		//记录当前j值为本行实际左边界
			Left_Add_Line[i] = Left_Add_Line[i+2];	// 记录实际左边界为补线左边界
		}
		else
		{
			if (i >= 55)	// 前6行
			{
				Left_Add_Line[i] = Left_Line[59];	// 使用底行数据
			}
			else
			{
				Left_Add_Line[i] = Left_Add_Line[i+2];	// 使用前2行左边界作为本行左边界
			}
		}
	}
	if (Right_Add_Flag[i])	// 右边界需要补线
	{
		if (!data[(i-2)*160 + Right_Add_Line[i+2]] || !data[(i-4)*160 + Right_Add_Line[i+2]])	// 可能是反光干扰
		{
			Right_Add_Flag[i] = 0;	//左边界不需要补线，清除标志位
			Right_Line[i] = Right_Add_Line[i+2];		//记录当前j值为本行实际左边界
			Right_Add_Line[i] = Right_Add_Line[i+2];	// 记录实际左边界为补线左边界
		}
		else
		{
			if (i >= 55)	// 前6行
			{
				Right_Add_Line[i] = Right_Line[59];	// 使用底行数据
			}
			else
			{
				Right_Add_Line[i] = Right_Add_Line[i+2];	// 使用前2行右边界作为本行右边界
			}
		}
	}
	Width_Real[i] = Right_Line[i] - Left_Line[i];			// 计算实际赛道宽度
	Width_Add[i] = Right_Add_Line[i] - Left_Add_Line[i];	// 计算补线赛道宽度
	
//	data[80 * i + Right_Line[i] + 2] = 0;//右移2位显示左边界，方便调试观察
//	data[80 * i + Left_Line[i] - 2] = 0;//左移2位显示右边界，方便调试观察
}

/*
*	补线修复
*
*	说明：有始有终才使用，直接使用两点斜率进行补线
*/
void Line_Repair(uchar Start, uchar Stop, uchar *data, uchar *Line, uchar *Line_Add, uchar *Add_Flag, uchar Mode)
{
	float res;
	uchar i, End;	// 控制行
	uchar Hazard_Width;
	float Ka, Kb;
	
	if ((Mode == 1) && (Right_Add_Start <= Stop) && Stop && Start <= 53 && !Left_Hazard_Flag)	// 左边界补线
	{
		for (i = Start+2; i >= Stop+2;)
		{
			i -= 2;
			Line_Add[i] = range_protect(Right_Add_Line[i] - Width_Add[i+2]+5, 1, Right_Add_Line[i]); 
			Width_Add[i] = Width_Add[i+2]-3;
			
			if (Width_Add[i] <= 20)
			{
				Line_Count = i;
				break;
			}
		}
	}
	if ((Mode == 2) && (Left_Add_Start <= Stop) && Stop && Start <= 53 && !Right_Hazard_Flag)	// 右边界补线
	{
		for (i = Start+2; i >= Stop+2;)
		{
			i -= 2;
			Line_Add[i] = range_protect(Left_Add_Line[i] + Width_Add[i+2]-5, Left_Add_Line[i], 159); 
			Width_Add[i] = Width_Add[i+2]-3;
			
			if (Width_Add[i] <= 20)
			{
				Line_Count = i;
				break;
			}
		}
	}
	else
	{
		if (Stop)	// 有始有终
		{
			if (((Mode == 1 && Left_Add_Start >= 55) || (Mode == 2 && Right_Add_Start >= 55))
				&& ((Mode == 1 && Line_Add[Left_Add_Stop] > Line_Add[59]) 
				|| (Mode == 2 && Line_Add[Right_Add_Stop] < Line_Add[59])))// 只有较少行需要补线
			{
				if(Stop >= Line_Count + 4)
				{
					End = Stop - 4;
				}
				else if(Stop >= Line_Count + 2)
				{
					End = Stop - 2;
				}
				else
				{
					End = 0;
				}
				if (End)
				{
					Ka = 1.0*(Line_Add[Stop] - Line_Add[End]) / (Stop - End);
					Kb = 1.0*Line_Add[Stop] - (Ka * Stop);
					
					if (Mode == 1)
					{
						if (Line_Add[59] > 59 * Ka + Kb)
						{
							Ka = 1.0*(Line_Add[Start] - Line_Add[Stop-2]) / (Start - (Stop-2));
							Kb = 1.0*Line_Add[Start] - (Ka * Start);
						}
					}
					else if (Mode == 2)
					{
						if (Line_Add[59] < 59 * Ka + Kb)
						{
							Ka = 1.0*(Line_Add[Start] - Line_Add[Stop-2]) / (Start - (Stop-2));
							Kb = 1.0*Line_Add[Start] - (Ka * Start);
						}
					}
				}
				else
				{
					Ka = 1.0*(Line_Add[Start] - Line_Add[Stop]) / (Start - Stop);
					Kb = 1.0*Line_Add[Start] - (Ka * Start);
				}
				
				for (i = 61; i > Stop; )
				{
					i -= 2;
					res = i * Ka + Kb;
					Line_Add[i] = range_protect((int32)res, 1, 159);
				}
			}
			else	// 将起始行和结束行计算斜率补线
			{
				if (Start <= 57)
				{
					Start +=2;
				}
				if (Stop >= Line_Count + 4)
				{
					Stop -= 4;
				}
				else if (Stop >= Line_Count + 2)
				{
					Stop -= 2;
				}
				Ka = 1.0*(Line_Add[Start] - Line_Add[Stop]) / (Start - Stop);
				Kb = 1.0*Line_Add[Start] - (Ka * Start);
				
				if (Mode == 1)
				{
					if (Line_Add[59] > 59 * Ka + Kb)
					{
						Ka = 1.0*(Line_Add[Start] - Line_Add[Stop]) / (Start - Stop);
						Kb = 1.0*Line_Add[Start] - (Ka * Start);
					}
				}
				else if (Mode == 2)
				{
					if (Line_Add[59] < 59 * Ka + Kb)
					{
						Ka = 1.0*(Line_Add[Start] - Line_Add[Stop]) / (Start - Stop);
						Kb = 1.0*Line_Add[Start] - (Ka * Start);
					}
				}
				
				for (i = Stop; i <= Start; )
				{
					i += 2;
					res = i * Ka + Kb;
					Line_Add[i] = range_protect((int32)res, 1, 159);
				}
			}
		}
	}
	
	if (Mode == 1 && Left_Hazard_Flag)
	{
		for (i = Left_Hazard_Flag; i < 59; )
		{
			i += 2;
			Line_Add[i] = range_protect((int32)Line_Add[i-2]-5, Line_Add[i], 159);
		}
	}
	else if (Mode == 2 && Right_Hazard_Flag)
	{
		for (i = Right_Hazard_Flag; i < 59; )
		{
			i += 2;
			Line_Add[i] = range_protect((int32)Line_Add[i-2]+5, 1, Line_Add[i]);
		}
	}
}

/*
*	中线修复
*
*	说明：普通弯道丢线使用平移赛道方式，中点到达边界结束
*/
void Mid_Line_Repair(uchar count, uchar *data)
{
	int res;
	uchar i;	// 控制行
	uchar Hazard_Width;
	
//	if (Left_Hazard_Flag)
//	{
//		Line_Count = Left_Hazard_Flag;
//		Hazard_Width = Left_Add_Line[Left_Hazard_Flag];
//		for (i = 61; i >= Left_Hazard_Flag+2;)
//		{
//			i -= 2;
//			Left_Add_Line[i] = 0.7*Hazard_Width + 0.3*Left_Add_Line[i];
//		}
//	}
//	if (Right_Hazard_Flag)
//	{
//		Line_Count = Right_Hazard_Flag;
//		Hazard_Width = Right_Add_Line[Right_Hazard_Flag];
//		for (i = 61; i >= Right_Hazard_Flag+2;)
//		{
//			i -= 2;
//			Right_Add_Line[i] = 0.7*Hazard_Width + 0.3*Right_Add_Line[i];
//		}
//	}
	
	for (i = 61; i >= count+2;)
	{
		i -= 2;
		Mid_Line[i] = (Right_Add_Line[i] + Left_Add_Line[i]) / 2;	// 计算赛道中点
		Width_Add[i] = Right_Add_Line[i] - Left_Add_Line[i];		// 计算赛道宽度
			
	/**//*************************** 上位机显示边界 ***************************/
	/**/data[i*160 + Left_Add_Line[i] + 6] = 0;	// 上位机显示补线后的左边界，不用时屏蔽
	/**/data[i*160 + Right_Add_Line[i] - 6] = 0;	// 上位机显示补线后的右边界，不用时屏蔽
	/**/data[i*160 + Mid_Line[i]] = 0;			// 上位机显示中线，不用时屏蔽
	/**/data[i*160 + Left_Line[i] + 1] = 0;		// 上位机显示原始左边界，不用时屏蔽
	/**/data[i*160 + Right_Line[i] - 1] = 0;		// 上位机显示原始右边界，不用时屏蔽
	/**//*************************** 上位机显示边界 ***************************/
	}
	Mid_Line[61] = Mid_Line[59];
}

/****************** 新算法 ******************/

/*
*	加权平均
*
*	说明：权重是乱凑的，效果不好
*/
uchar Point_Weight(void)
{
	uchar i ,Point, Point_Mid;
	static char Last_Point = 80;
	int32 Sum = 0, Weight_Count = 0;
	
	if (Line_Count <= 20)
	{
		Line_Count = 20;
	}
	
	if (Out_Side || Line_Count >= 53)	//出界或者摄像头图像异常
	{
		if (Last_Point == 80)
		{
			Point = Last_Point;
		}
		else if (Last_Point < 80)
		{
			Point = 1;
		}
		else if (Last_Point > 80)
		{
			Point = 159;
		}
	}
	else
	{
		for (i = 61; i >= Line_Count; )		//使用加权平均
		{
			i -= 2;
			Sum += Mid_Line[i] * Weight[59-i];
			Weight_Count += Weight[59-i];
		}
		Point = range_protect(Sum / Weight_Count, 1, 159);

							/*** 障碍物特殊情况处理 ***/
//		if (Left_Hazard_Flag)			//左侧有障碍物且需要补线，即使误判也不会造成影响
//		{
//			Point = Mid_Line[Left_Hazard_Flag]+6;	//使用障碍物出现的那一行中点作为目标点
//			if (Left_Hazard_Flag < 80)
//			{
//				Point += 3;
//			}
//		}
//		else if (Right_Hazard_Flag)	//右测有障碍物且需要补线，即使误判也不会造成影响
//		{
//			Point = Mid_Line[Right_Hazard_Flag]-6;//使用障碍物出现的那一行中点作为目标点
//			if (Right_Hazard_Flag < 80)
//			{
//				Point -= 5;
//			}
//		}
//		Point = Mid_Line[59];
		Point = range_protect(Point, 2, 158);
		Last_Point = Point;
		
							/***** 使用最远行数据和目标点作为前瞻 *****/
		if (Line_Count >= 25)
		{
			Point_Mid = Mid_Line[60-30];
		}
		else
		{
			Point_Mid = Mid_Line[60-Line_Count];
		}
	}
	Foresight = 0.8 * Error_Transform(Point_Mid, 80)	//使用最远行偏差和加权偏差确定前瞻
			  + 0.2 * Error_Transform(Point, 	 80);
	
	return Point;
}

char Error_Transform(uchar Data, uchar Set)
{
	char Error;
	
	Error = Set - Data;
	if (Error < 0)
	{
		Error = -Error;
	}
	
	return Error;
}



/*!
*  @brief      二值化图像解压（空间 换 时间 解压）
*  @param      dst             图像解压目的地址
*  @param      src             图像解压源地址
*  @param      srclen          二值化图像的占用空间大小
*  @since      v5.0            img_extract(img, imgbuff,CAMERA_SIZE);
*  Sample usage:
*/
void img_extract(uint8 *dst, uint8 *src, uint32 srclen)
{
  uint8 tmpsrc;
  while(srclen --)
  {
    tmpsrc = *src++;
    *dst++ = colour[ (tmpsrc >> 7 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 6 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 5 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 4 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 3 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 2 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 1 ) & 0x01 ];
    *dst++ = colour[ (tmpsrc >> 0 ) & 0x01 ];
  }
}

/*!
*  @brief      二值化图像压缩（空间 换 时间 压缩）
*  @param      dst             图像压缩目的地址
*  @param      src             图像压缩源地址
*  @param      srclen          二值化图像的占用空间大小
*  @since      v5.0            img_extract(img, imgbuff,CAMERA_SIZE);
*  Sample usage:
*/
void img_recontract(uint8 *dst, uint8 *src, uint32 srclen)
{
  uint8 tmpsrc;
  uint8 i, j, k;
  uint32 Count = 0;
  
  for (i = 0; i < 60/2; i++)
  {
	  for (j = 0; j < 160/8; j++)
	  {
		  Count = (i*2+1)*160+j*8;
		  for (k = 0; k < 8; k++)		  
		  {
			  tmpsrc <<= 1;
			  if (!dst[Count++])
			  {
				  tmpsrc |= 1;
			  }
		  }
		  *src++ = tmpsrc;
	  }
  }
  
//  while(srclen --)
//  {
//    tmpsrc=0;
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x80;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x40;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x20;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x10;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x08;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x04;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x02;
//    }
//    
//    if(*dst++ == colour[black])
//    {
//      tmpsrc = tmpsrc + 0x01;
//    }
//    
//    *src++ = tmpsrc;
//  }
}

/*!
*  @brief      取边界线
*  @param      dst             图像压缩目的地址
*  @param      src             图像压缩源地址
*  @param      srclen          二值化图像的占用空间大小
*  @since      v5.0            img_extract(img, imgbuff,CAMERA_SIZE);
*  Sample usage:
*/
void img_getline(uint8 *dst, uint8 *src, uint32 srclen)
{
  uint8 tmpsrc,buff_y,buff_x;
  
  while(srclen --)
  {
    tmpsrc=0;
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x80;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x40;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x20;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x10;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x08;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x04;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if(*dst != buff_y || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x02;
      *(dst-1)=colour[black];
    }
    
    buff_x = *(dst + 80);
    buff_y = *dst++;
    *(dst-1)=colour[white];
    if((*dst != buff_y && srclen % 10 !=0 ) || (buff_y != buff_x && srclen > 9))
    {
      tmpsrc = tmpsrc + 0x01;
      *(dst-1)=colour[black];
    }
    
    *src++ = tmpsrc;
  }
}
