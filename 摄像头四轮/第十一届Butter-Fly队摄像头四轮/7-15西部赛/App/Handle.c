#include "Handle.h"

uint8 colour[2] = {255, 0},black=1,white=0; //0 和 1 分别对应的颜色
//注：山外的摄像头 0 表示 白色，1表示 黑色

uchar Weight[60] = {1,  1,  1,  1,  1,    1,  1,  1,  1,  2,
					3,  4,  5,  7,  9,   10, 12, 14, 16, 19,
				   21, 20, 19, 19, 18,   15, 13, 11,  8,  6,
				    4,  3,  3,  2,  1,    1,  1,  1,  1,  1,
					1,  1,  1,  1,  1,    1,  1,  1,  1,  1,
					1,  1,  1,  1,  1,    1,  1,  1,  1,  1};	//加权平均参数

uchar Left_Line[63], Mid_Line[63], Right_Line[63];	//左中右边界线
uchar Left_Line_New[63], Right_Line_New[63];		//边界补线的坐标
uchar Width[61], Width_Min;							//每行赛道的宽度
uchar Left_Add[61], Right_Add[61]; 					//左右边界补线标志位
uchar Left_Add_Flag, Right_Add_Flag;				//左右边界补线总标志位
uchar Left_Add_Start, Right_Add_Start;				//记录补线的开始位置
uchar Left_Add_End, Right_Add_End;					//记录补线的结束位置
uchar Left_Max, Right_Min;							//左右边界极值，用于连续S弯
uchar Left_y, Right_y;								//左右边界极值的纵坐标
uchar Point_x, Point_y;								//目标点的横纵坐标
uchar Mid_Count = 0;		   						//能够识别的最远处中点行数
uchar Foresight_Left, Foresight_Right;				//前瞻左右边界极值
uchar Turn_Left = 0, Turn_Right = 0;
uchar Repair_Flag;
uchar Hazard_Left_Flag,Hazard_Right_Flag;			//障碍物标志位
uchar Left_Jam, Right_Jam;							//左右干扰信号
uchar Starting_Line_Flag;							//起跑线标志位
int32  Last_Angle = 0;
float Last_Slope = 0;
float Left_Last_Slope = 0;
float Right_Last_Slope = 0;

uchar Point_Mid = 0;
uchar Foresight = 0;	//前瞻控制速度
uchar Out_Side = 0;		//出界计数

uchar Image_GetLine(uchar *data)	//获取左中右边界线
{
	char i = 59;	//i代表行
	uchar Line_Count, Left_Temp, Right_Temp;
	char temp;
	float Add_Slope;
	
	Hazard_Left_Flag = 0;	//左右障碍物标志位清零
	Hazard_Right_Flag = 0;
	Left_Jam = 0;			//左右干扰信号清零
	Right_Jam = 0;
	Starting_Line_Flag = 0;	//起跑线标志位清零
	
	Mid_Count = 0;
	Left_Max = 2;
	Right_Min = 78;
	Left_y = 59;
	Right_y = 59;
	Width_Min = 80;
	Repair_Flag = 0;
	Left_Add_Start = 0;
	Right_Add_Start = 0;
	Left_Add_End = 0;
	Right_Add_End = 0;
	Foresight_Left = 2;
	Foresight_Right = 78;
	Width[60]     = 80;
	Left_Add[60]  = 0;
	Right_Add[60] = 0;
	Left_Line[60] = 2;
	Left_Line[61] = 2;
	Left_Line[62] = 1;
	Mid_Line[60]  = 40;
	Mid_Line[61]  = 40;
	Mid_Line[62]  = 40;
	Right_Line[60]= 78;
	Right_Line[61]= 78;
	Right_Line[62]= 79;	//人为确定最下方两行图像的边界
	
	for (i = 59; i >= 20; i--)	//只采集前40行的图像，因为太远的图像不稳定，参考价值不大
	{
		Left_Add[i] = 1;	//扫描每行之前都认为左右边界都需要补线
		Right_Add[i] = 1;
		
		if (!data[80 * i + Mid_Line[i + 1]])//前一行中点在本行为黑点
		{
			if (i == 59)	//第一行
			{
				First_Line_Add(data);	//使用第一行专用处理函数
				if (Right_Line[59] - Left_Line[59] < 40)	//第一行数据异常
				{
					Traversal_Left_Side(59, data);	//重新使用边界扫描
					Traversal_Right_Side(59, data);
				}
				
				continue;
			}
			else	//到达尽头
			{
				break;
			}
		}
		else
		{			
			Traversal_Mid_Line(i, data);	//从前一行中点向两边寻找边界
			if (i >= 55)
			{
				if (Right_Line[i] - Left_Line[i] < 40)	//第一行数据异常
				{
					Traversal_Left_Side(i, data);	//重新使用边界扫描
					Traversal_Right_Side(i, data);
				}
			}
		}		
		
		/* 第一次轮对图像进行处理，在需要补线的时候利用斜率向上补线
		   这样补线只是为了向上找边界时更准确，不易在斜入十字时误判
		   补线后的边界不能直接使用，仍要修复后才可以使用 */
		
		Width[i] = Right_Line[i] - Left_Line[i];	//计算本行赛道宽度
		if ((Width[i] >= Width[i+1]) || (Width[i] >= Width_Min))	//本行赛道宽度大于等于上一行赛道宽度，认为遇到突变
		{
			Repair_Flag = 1;	//需要补线
			
			if (Left_Add[i+1])	//前一行补线了
			{
				if (Left_Line[i] < Left_Line_New[i+1]-1)	//与前一行的左边界实线比较	
				{
					Left_Add[i] = 1;
				}
			}
			else	//前一行没有补线
			{
				if (Left_Line[i] < Left_Line[i+1]-1)	//与前一行的左边界实线比较	
				{
					Left_Add[i] = 1;
				}
			}
			
			if (Right_Add[i+1])	//前一行右边界补线了	
			{
				if (Right_Line[i] > Right_Line_New[i+1]+1)	//与前一行的右边界实线进行比较
				{
					Right_Add[i] = 1;
				}
			}
			else	//前一行右边界没有补线
			{
				if (Right_Line[i] > Right_Line[i+1]+1)		//与前一行的右边界实线进行比较
				{
					Right_Add[i] = 1;
				}
			}
		}
		else	/**************************   进行障碍检测   ******************/
		{
			if (i != 59 && i > 30)	//第一行不算,太远的也不管
			{
				if ((Right_Line[i] + 5 < Right_Line[i+1]) && !Right_Add[i+1])//本行与前一行右边界有较大突变
				{
					if (!data[80*(i-1)+Right_Line[i]] && !data[80*(i-2)+Right_Line[i]])	//连续三行都是黑色
					{			
						if (!Hazard_Right_Flag)			//右侧障碍物标志位还没有置位
						{
							Hazard_Right_Flag = i;		//认为右半侧赛道有突变，记录障碍出现位置
						}
					}
					else
					{
						if (!Right_Jam)		//如果右侧干扰信号还未置位
						{
							Right_Jam = i;	//认为右侧出现干扰，记录干扰信号位置
						}
						Traversal_Right_Side(i, data);	//重新使用边界寻线法寻找干扰行边界
					}
				}
				if (Left_Line[i] > Left_Line[i+1] + 5 && !Left_Add[i+1])	//本行与前一行左边界有较大突变且前一行没有补线
				{
					if (!data[80*(i-1)+Left_Line[i]] && !data[80*(i-2)+Left_Line[i]])	//连续三行都是黑色
					{
						if (!Hazard_Left_Flag)			//左侧障碍物标志位还没有置位
						{
							Hazard_Left_Flag = i;		//认为左半侧赛道有障碍物，记录障碍出现位置
						}
					}
					else
					{
						if (!Left_Jam)		//如果左侧干扰信号还未置位
						{
							Left_Jam = i;	//认为左侧出现干扰，记录干扰信号位置
						}
						Traversal_Left_Side(i, data);	//重新使用边界寻线法寻找干扰行边界
					}
				}			
			}
		}
		/*******************************   障碍检测结束   **********************/
		
		if (Left_Add[i])	//左边需要补线
		{	
			if (!Left_Add_Start)	//如果还没有记录开始补线位置
			{
				Left_Add_Start = i;	//记录左边界补线开始位置
			}
			if (i >= 50)		//能够识别的行数非常少，不具备参考价值
			{
				
				Add_Slope = Left_Last_Slope;	//使用上一帧图像的左边界斜率
				if (Add_Slope > 0)
				{
					Add_Slope = 0;
				}
				temp = (char)((i - 59) * Add_Slope + Left_Line[59]);	//通过斜率推算补线的位置
			}
			else
			{
				if (Left_Add_Start >= 50)
				{
					Add_Slope = Left_Last_Slope;
				}
				else	//使用前几行的斜率进行补线
				{
					Add_Slope = 1.0*(Left_Line[Left_Add_Start+6] - Left_Line[Left_Add_Start+1]) / 5;	//计算能识别的前几行图像斜率
				}
				if (Add_Slope > 0)	//限幅
				{
					Add_Slope = 0;
				}
				temp = (char)((i - (Left_Add_Start+1)) * Add_Slope + Left_Line[Left_Add_Start+1]);//通过斜率推算补线的位置
				Left_Last_Slope = Add_Slope;	//更新上次左边界斜率
			}
						
			Left_Line_New[i] = range_protect(temp, 2,78);	//不直接修改边界，只保存在补线数组里			

													/* 第一次补线，只记录，不在图像上显示 */
//			data[80 * i + Left_Line_New[i] + 2] = 0;	//右移2位显示左边界，方便调试观察	  
		}
		if (Right_Add[i])	//右边需要补线
		{	
			if (!Right_Add_Start)	//如果还没有记录开始补线位置
			{
				Right_Add_Start = i;	//记录左边界补线开始位置
			}
			if (i >= 50)		//能够识别的行数非常少，不具备参考价值
			{				
				Add_Slope = Right_Last_Slope;	//使用上一帧图像的右边界斜率
				if (Add_Slope < 0)
				{
					Add_Slope = 0;
				}
				temp = (char)((i - 59) * Add_Slope + Right_Line[59]);//通过斜率推算补线的位置
			}
			else
			{
				if (Right_Add_Start >= 50)
				{					
					Add_Slope = Right_Last_Slope;	
				}
				else
				{
					Add_Slope = 1.0*(Right_Line[Right_Add_Start+6] - Right_Line[Right_Add_Start+1]) / 5;	//计算能识别的前几行图像斜率
				}
				if (Add_Slope < 0)	//限幅
				{
					Add_Slope = 0;
				}
				temp = (char)((i - (Right_Add_Start+1)) * Add_Slope + Right_Line[Right_Add_Start+1]);//通过斜率推算补线的位置
				Right_Last_Slope = Add_Slope;	//更细上次右边界斜率
			}
						
			Right_Line_New[i] = range_protect(temp, 2, 78);		//不直接修改边界，只保存在补线数组里
			
													/* 第一次补线，只记录，不在图像上显示 */			
//			data[80 * i + Right_Line_New[i] - 2]  = 0;	//左移2位显示右边界，方便调试观察	  
		}
		if (Left_Add[i] && Right_Add[i])	//两边都需要补线
		{
			Width[i] = Right_Line_New[i] - Left_Line_New[i];	//重新计算本行赛道宽度
		}
		else	//不需要补线或只有一边需要补线
		{
			if (Left_Add[i])	//此处最多只有一边会需要补线
			{
				Width[i] = Right_Line[i] - Left_Line_New[i];	//重新计算本行赛道宽度
			}
			else if (Right_Add[i])
			{
				Width[i] = Right_Line_New[i] - Left_Line[i];	//重新计算本行赛道宽度
			}
			else
			{
				Width[i] = Right_Line[i] - Left_Line[i];		//可能会有干扰，也重新计算一次赛道宽度
			}
			if (Width[i] < Width_Min)
			{
				Width_Min = Width[i];	//更新最小赛道宽度
			}
			if (Left_Line[i] > Foresight_Left)	//更新左边界最大值
			{
				Foresight_Left = Left_Line[i];
			}
			if (Right_Line[i] < Foresight_Right)//更新右边界最小值
			{
				Foresight_Right = Right_Line[i];
			}
			
			if (!Left_Add[i] && Left_Add[i+1] && !Left_Add_End)	//左边不需要补线且前一行需要补线,且是左边第一段补线
			{
				Left_Add_End = 1;	//只记录第一段补线的目标点坐标，忽略后面的
				Left_y = i;			//记录左边界补线结束的位置
				if (Left_Line[i] > Left_Max)
				{
					Left_Max = Left_Line[i];	//更新左边界最大值					
				}
			}
			if (!Right_Add[i] && Right_Add[i+1] && !Right_Add_End)	//右边不需要补线且前一行需要补线,且是右边第一段补线
			{
				Right_Add_End = 1;	//只记录第一段补线的目标点坐标，忽略后面的
				Right_y = i;		//记录右边界补线结束的位置
				if (Right_Line[i] < Right_Min)
				{
					Right_Min = Right_Line[i];	//更新右边界最小值
				}
			}			
		}
		
		if (Left_Add[i])
		{
			Left_Temp = Left_Line_New[i];
		}
		else
		{
			Left_Temp = Left_Line[i];
		}
		if (Right_Add[i])
		{
			Right_Temp = Right_Line_New[i];
		}
		else
		{
			Right_Temp = Right_Line[i];
		}
		Mid_Line[i] = (Left_Temp + Right_Temp) / 2;	//求出本行赛道中点，只用作参考，不在图像上显示
	}
	
	if (Left_Jam && Right_Jam)	//左右都存在干扰
	{
		Starting_Line_Flag = 1;		//认为检测到起跑线
	}
	
	Line_Count = i;	//保存成功分析的边界的行数
	
	/********** 测试 *********/
//	save_var(VAR6, Hazard_Left_Flag);
//	save_var(VAR6, Right_Add_Start);
//	save_var(VAR2, Right_Add_Start);
//	save_var(VAR2, Left_Add_Start);
//	save_var(VAR2, Hazard_Left_Flag);
	save_var(VAR2, Hazard_Right_Flag);
//	save_var(VAR2, Left_Jam);
//	save_var(VAR2, Right_Jam);
//	save_var(VAR2, Starting_Line_Flag);
		
	/* 第二轮图像处理，使用左右边界的最值将左右边界修复的更加合理 */
	if (Right_Add_Start && Left_Add_Start && !Hazard_Left_Flag && !Hazard_Right_Flag)	//左右都需要补线且没有障碍物
	{
		Left_Line_Repair(Left_y, Left_Max, data);	//左边界补线修正
		Right_Line_Repair(Right_y, Right_Min, data);//右边界补线修正
	}

	
	/* 第三轮图像处理，使用修复过的左右边界重新计算中点并在图像中显示出来 */		
	Mid_Line_Repair(Line_Count, data);			//中线修复
	
	if (Mid_Count <= 2)	//成功分析的图像不超过五行认为出界，停车
	{
		Out_Side++;
		led(LED0,LED_OFF);
		if (Out_Side >= 50)
		{
//			Run_Flag = 0;			
		}
	}
	else
	{
		Out_Side = 0;
	}
		
	return Mid_Count;
}

void First_Line_Add(uchar *data)
{
	uchar i = 59;
	
	if (Turn_Left)
	{
		Traversal_Left_Line(i, &data[80 * i], &Left_Line[i], &Right_Line[i]);//使用左遍历获取边界
	}
	else if (Turn_Right)
	{
		Traversal_Right_Line(i, &data[80 * i], &Left_Line[i], &Right_Line[i]);//使用右遍历获取边界
	}
	else
	{
		if (Traversal_Left(&data[80 * i]) >= Traversal_Right(&data[80 * i]))//左边间距大于右边间距
		{
			Traversal_Left_Line(i, &data[80 * i], &Left_Line[i], &Right_Line[i]);//使用左遍历获取边界
		}
		else
		{
			Traversal_Right_Line(i, &data[80 * i], &Left_Line[i], &Right_Line[i]);//使用右遍历获取边界
		}
	}
				
	if (Left_Add[i])	//如果左边需要补线
	{
		Left_Line[i] = 2;		//以前一行的左边界作为本行左边界临时补线位置
	}
	if (Right_Add[i])	//如果右边需要补线
	{
		Right_Line[i] = 78;		//以前一行的右边界作为本行右边界临时补线位置
	}
}

uchar Point_Handle(void)
{
	uchar i;
	uchar Point = 40;
	int32 Sum = 0;
	
	if (Foresight_Left + 15 < Foresight_Right)		//位于直线或小S且没有障碍物
	{
		Point = (Foresight_Left + Foresight_Right) / 2;	//取左右极值中点作为目标点
	}
	else		/****** 简单加权求目标点 ******/
	{
		if (Mid_Count > 10)	
		{
			if (Mid_Count > 30)
			{
				Mid_Count = 30;
			}
			for (i = 0; i <= Mid_Count-10; i++)
			{
				Sum += Mid_Line[49-i];
			}
			Point = Sum / (Mid_Count - 10);
		}
		else
		{
			Point = Mid_Line[60 - Mid_Count];
		}
	}
	
				/****** 障碍物处理 ******/
	if (Hazard_Left_Flag)					//左侧有障碍物
	{
		Point = Mid_Line[Hazard_Left_Flag] + 0;	//使用障碍物出现的那一行中点作为目标点
	}
	else if (Hazard_Right_Flag)			//右测有障碍物
	{
		Point = Mid_Line[Hazard_Right_Flag] - 0;//使用障碍物出现的那一行中点作为目标点
	}
	
	
	if (Mid_Count >= 35)
	{
		Point_Mid = Mid_Line[60-35];
	}
	else
	{
		Point_Mid = Mid_Line[60-Mid_Count];
	}
	Foresight = 0.8 * Error_Transform(Point_Mid, 40)	//使用最远行偏差和加权偏差确定前瞻
			  + 0.2 * Error_Transform(Point, 	 40);
	
	return Point;
}

uchar Point_Weight(void)
{
	char i ,Point;
	static char Last_Point = 40;
	int32 Sum = 0, Weight_Count = 0;
	
	if (Out_Side)	//出界或者摄像头图像异常
	{
		Point  = Last_Point;
		Point_Mid = 2;
	}
	else
	{
		if (Foresight_Left + 33 < Foresight_Right)		//位于直线或小S且没有障碍物
		{
			Point = (Foresight_Left + Foresight_Right) / 2;	//取左右极值中点作为目标点
		}
		else	//位于普通弯道或大S
		{
			for (i = 59; i >= 60-Mid_Count; i--)		//使用加权平均
			{
				Sum += Mid_Line[i] * Weight[59-i];
				Weight_Count += Weight[59-i];
			}
			Point = range_protect(Sum / Weight_Count, 2, 78);
		}
							/*** 障碍物特殊情况处理 ***/
		if (Hazard_Left_Flag && Left_Add_Start)			//左侧有障碍物且需要补线，即使误判也不会造成影响
		{
			Point = Mid_Line[Hazard_Left_Flag];	//使用障碍物出现的那一行中点作为目标点
			if (Hazard_Left_Flag < 40)
			{
				Point += 3;
			}
		}
		else if (Hazard_Right_Flag && Right_Add_Start)	//右测有障碍物且需要补线，即使误判也不会造成影响
		{
			Point = Mid_Line[Hazard_Right_Flag];//使用障碍物出现的那一行中点作为目标点
			if (Hazard_Right_Flag < 40)
			{
				Point -= 5;
			}
		}
		Point = range_protect(Point, 2, 78);
		Last_Point = Point;
		
		if (Point < 40)		/***** 以下这几句貌似并没有什么卵用，只对第一行特殊情况有微弱影响 ****/
		{
			Turn_Left = 1;
			Turn_Right = 0;
		}
		else if (Point > 40)
		{
			Turn_Right = 1;
			Turn_Left = 0;
		}
		else
		{
			Turn_Left = 0;
			Turn_Right = 0;
		}
							/***** 使用最远行数据和目标点作为前瞻 *****/
		if (Mid_Count >= 25)
		{
			Point_Mid = Mid_Line[60-30];
		}
		else
		{
			Point_Mid = Mid_Line[60-Mid_Count];
		}
	}
	Foresight = 0.8 * Error_Transform(Point_Mid, 40)	//使用最远行偏差和加权偏差确定前瞻
			  + 0.2 * Error_Transform(Point, 	 40);
	
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

int32 Curvature_Transform(uchar Now, uchar Point)	//曲率转化，结果放大1000倍
{
	char Positive = 1; 	//记录曲率正负
	int32 Curvature;	//曲率
	int32 L_AB, L_BC;	//两点组成的直角三角形两直角边
	int32 Error_AA;		//车身所在位置与赛道中点的距离
	
	L_AB = 59 - Point;
	if (Mid_Line[Point] >= Now)	//应向右转
	{
		L_BC = Mid_Line[Point] - Now;
		Positive = -1;	//曲率记为负数
	}
	else				//应向左转
	{
		L_BC = Now - Mid_Line[Point];
		Positive = 1;	//曲率记为正数
	}
	if (Now >= 35 && Now <= 45)	//车身在赛道中间
	{
		Curvature = 1000 * 2 * L_BC / (L_AB*L_AB + L_BC*L_BC);	//曲率放大100倍
	}
	else if (Now < 35)			//车身在赛道偏左的位置
	{
		Error_AA = 40 - Now;	//计算车身偏离赛道中点的距离
		Curvature = 1000 * 2 * (L_BC - Error_AA) / (L_AB*L_AB + (L_BC-Error_AA) * (L_BC-Error_AA));	//曲率放大1000倍
	}
	else if (Now > 45)			//车身位于赛道偏右的位置
	{
		Error_AA = Now - 40;	//计算车身偏离赛道中点的距离
		Curvature = 1000 * 2 * (L_BC + Error_AA) / (L_AB*L_AB + (L_BC+Error_AA) * (L_BC+Error_AA));	//曲率放大1000倍
	}
	
	Curvature *= Positive;
	
	return Curvature;
}

int32 Angle_Transform(uchar *data)	//目标角度转化
{
	double Angle_Temp = 0;
	double Slope = 0;	//斜率		
	int32 Angle = 0;	//角度
	
	Point_Scan();
	Slope = Slope_Display(data);			//计算目标点斜率并显示在图像中
	Angle_Temp = atan(Slope) * 180 / PI;	//使用反三角函数求出斜率对应角度
	Angle = Wave_Filtering(Angle_Temp, 5);	//将解算的角度进行深度为5的滑动滤波
	Last_Slope = Slope;	//更新上次斜率
	Last_Angle = Angle;	//更新上次目标角度
	
	if (Angle >= 0)
	{
		save_var(VAR2,Angle);					//将角度发回上位机
	}
	else
	{
		save_var(VAR2,-Angle);
	}
		
	return Angle;
}

void Mid_Filtering(uchar *data)		//中线滤波
{
	uchar i, Count;
	
	Count = 60 - Mid_Count;	//判断一共有多少多少可用中点
	for (i = 58; i > Count; i--)	//从下向上滤波
	{
		Mid_Line[i] = Mid_Line[i+1]*0.3 + Mid_Line[i]*0.7;
	}
	for (i = Count; i < 59; i++)	//从上向下互补滤波
	{
		Mid_Line[i] = Mid_Line[i-1]*0.3 + Mid_Line[i]*0.7;
	}
	for (i = 59; i >= Count; i--)
	{
		data[80*i + Mid_Line[i]] = 0;	//将中线在图像上显示出来
	}
}

uint32 Wave_Filtering(uint32 data, uchar Deep)
{
	static uint32 Filter_Buff[10] = {0};
	static uchar Filter_Count = 0;
	uint32 Sum = 0;
	uint32 Filter;
	uchar i;
	
	Filter_Buff[Filter_Count++] = data;
	if (Filter_Count == Deep)
	{
		Filter_Count = 0;
	}
	for (i = 0; i < Deep; i++)
	{
		Sum += Filter_Buff[i];
	}
	Filter = Sum / Deep;
	
	return Filter;
}

void Point_Scan(void)
{	
	if (Mid_Count > 30)
	{
		Point_x = Mid_Line[30];
		Point_y = 30;
	}
	else
	{
		Point_x = Mid_Line[60 - Mid_Count];//使用能识别的最远中点为目标点
		Point_y = 60 - Mid_Count;
	}
/*	Point_x = Mid_Line[60 - Mid_Count];//使用能识别的最远中点为目标点
	Point_y = 60 - Mid_Count;*/
	
	Point_x = range_protect(Point_x, 2, 78);
	Point_y = range_protect(Point_y, 20, 59);
}

double Slope_Display(uchar *data)
{
	char i;
	double Slope;	//斜率
	
	Slope = 1.0 * (40 - Point_x) / (60 - Point_y);	//计算目标点斜率
	
	for (i = 59; i >= Point_y; i--)
	{
		data[80 * i + (uchar)(40 - (60 - i)*Slope)] = 0;	
	}
	
	return Slope;
}

void Mid_Line_Repair(uchar count, uchar *data)
{
	uchar i, temp;
	uchar Left, Right;
	
	for (i = 59; i > count; i--)
	{
		if (Left_Add[i] && (Right_Add_Start || Hazard_Left_Flag))
		{
			Left = Left_Line_New[i];
		}
		else
		{
			Left = Left_Line[i];
		}
		if (Right_Add[i] && (Left_Add_Start || Hazard_Right_Flag))
		{
			Right = Right_Line_New[i];
		}
		else
		{
			Right = Right_Line[i];
		}
		temp = (Left + Right) / 2;	//左右边界都是修复过的，直接取中值
		Mid_Line[i] = range_protect(temp, 2, 78);	//限幅保护
		data[80*i + Left+2] = 0;	//左移2位显示左边界，方便调试观察
		data[80*i + Right-2] = 0;	//左移2位显示右边界，方便调试观察
		data[80*i + Mid_Line[i]] = 0;	//将中线在图像上显示出来
		if (Mid_Line[i] <= 2 || Mid_Line[i] >= 78)
		{
			i--;
			break;
		}
	}
	Mid_Count = 60 - (i+1);	//记录成功分析的中点的个数
//	Mid_Filtering(data);	//将中线执行从下到上再从下到上两轮滤波
}
						/* y为补线结束的行数，x为结束点横坐标 */
void Left_Line_Repair(uchar y, uchar x, uchar *data)//左边界补线修正
{
	int32 temp;
	uchar i, j, Row;
	float Slope;	//斜率
	
	for (i = 59; i > y; i--)
	{
		if (Left_Add[i])	//需要补线
		{
			if (i >= 55)	//第59行需要补线，从上向下补
			{
				Row = y;
				Slope = 1.0 * (Left_Line[y] - Left_Line[y-5]) / 5;
				if (Slope > 0)	//左边界补线斜率不能大于0
				{
					Slope = 0;
				}				
				for (j = Row+1; j <= 59; j++)
				{
					temp = (int32)((j-Row) * Slope + Left_Line[Row]);
					Left_Line_New[j] = range_protect(temp, 2,78);
					data[80*j + Left_Line_New[j] + 2] = 0;	//右移两位在图像上显示
				}
			}
			else	//从下向上补
			{
				Row = i + 1;	//从需要补线的前1行开始补线						
				Slope = 1.0 * (Left_Line[Row] - x) / (Row - y);	//计算目标点斜率
				if (Slope > 0)	//左边界补线斜率不能大于0
				{
					Slope = 0;
				}
				for (j = Row; j > y; j--)
				{
					temp = (int32)((j-Row)*Slope + Left_Line[Row]);				
					Left_Line_New[j] = range_protect(temp, 2, 78);
					data[80*j + Left_Line_New[j] + 2] = 0;	//右移两位在图像上显示						
				}
			}
			
			break;	//只需找到补线的起点即可，一次补完			
		}
	}
}

						/* x,y为右边界的最小值坐标 */
void Right_Line_Repair(uchar y, uchar x, uchar *data)//右边界补线修正
{
	int32 temp;
	uchar i, j, Row;
	float Slope;	//斜率
	
	for (i = 59; i > y; i--)
	{
		if (Right_Add[i])	//需要补线
		{
			if (i >= 55)	//第59行需要补线，从上向下补
			{
				Row = y;
				Slope = 1.0 * (Right_Line[y] - Right_Line[y-5]) / 5;
				if (Slope < 0)	//右边界补线斜率不能小于0
				{
					Slope = 0;
				}
				for (j = Row+1; j <= 59; j++)
				{
					temp = (int32)((j-Row) * Slope + Right_Line[Row]);
					Right_Line_New[j] = range_protect(temp, 2,78);
					data[80*j + Right_Line_New[j] - 2] = 0;	//左移两位在图像上显示
				}
			}
			else	//从下向上补
			{
				Row = i + 1;	//从需要补线的前1行开始补线			
				Slope = 1.0 * (Right_Line[Row] - x) / (Row - y);//计算目标点斜率
				if (Slope < 0)	//右边界补线斜率不能小于0
				{
					Slope = 0;
				}
				for (j = Row; j > y; j--)
				{
					temp = (int32)((j-Row)*Slope + Right_Line[Row]);
					Right_Line_New[j] = range_protect(temp, 2, 78);
					data[80*j + Right_Line_New[j] - 2] = 0;	//左移两位在图像上显示
				}
			}
			
			break;	//只需找到补线的起点即可，一次补完
		}
	}
}
						//i代表行数
void Traversal_Mid_Line(uchar i, uchar *data)	//从中间向两边扫描边界
{
	uchar j;
	
	for (j = Mid_Line[i + 1]; j >= 2; j--)	//以前一行中点为起点向左查找边界
	{
		if (!data[80*i+j] && !data[80*i+j-1])//检测到连续两个黑点，起到滤波的效果
		{
			Left_Add[i] = 0;		 //左边界不需要补线，清除标志位
			Left_Line[i] = j;		 //记录当前j值为第i行左边界
//			data[80 * i + j + 2] = 0;//右移2位显示左边界，方便调试观察
			
			break;
		}
	}
	for (j = Mid_Line[i + 1]; j < 79; j++)//以前一行中点为起点向右查找右边界
	{
		if (!data[80*i+j] && !data[80*i+j+1])//检测到连续两个黑点，起到滤波的效果
		{
			Right_Add[i] = 0;		 //右边界不需要补线，清除标志位
			Right_Line[i] = j;		 //记录当前j值为第i行右边界
//			data[80 * i + j - 2] = 0;//左移2位显示右边界，方便调试观察
			
			break;
		}
	}
	if (Left_Add[i])	//第一次补线直接补到左右边缘处
	{
		Left_Line[i] = 2;
		Left_Line_New[i] = 2;
	}
	if (Right_Add[i])
	{
		Right_Line[i] = 78;	
		Right_Line_New[i] = 78;
	}
}

void Traversal_Side_Line(uchar i, uchar *data)	//从两边向中间寻找边界，仅在出现干扰的情况下使用
{
	uchar j;
	
	for (j = Left_Line[i+1]; j < 78; j++)
	{
//		if (data[80*i+j] && data[80*i+j-1])//检测到连续两个白点，起到滤波的效果
		if (data[80*i+j])//检测到白点
		{
			Left_Add[i] = 0;		 //左边界不需要补线，清除标志位
			Left_Line[i] = j;		 //记录当前j值为第i行左边界
//			data[80 * i + j + 2] = 0;//右移2位显示左边界，方便调试观察
			
			break;
		}
	}
	for (j = Right_Line[i+1]; j > 2; j--)
	{
//		if (data[80*i+j] && data[80*i+j+1])//检测到连续两个白点，起到滤波的效果
		if (data[80*i+j])//检测到白点
		{
			Right_Add[i] = 0;		 //右边界不需要补线，清除标志位
			Right_Line[i] = j;		 //记录当前j值为第i行右边界
//			data[80 * i + j - 2] = 0;//左移2位显示右边界，方便调试观察
			
			break;
		}
	}
}
void Traversal_Left_Side(uchar i, uchar *data)
{
	uchar j;
	
	for (j = Left_Line[i+1]; j < 78; j++)
	{
//		if (data[80*i+j] && data[80*i+j-1])//检测到连续两个白点，起到滤波的效果
		if (data[80*i+j])//检测到白点
		{
			Left_Add[i] = 0;		 //左边界不需要补线，清除标志位
			Left_Line[i] = j;		 //记录当前j值为第i行左边界
			
			break;
		}
	}
}

void Traversal_Right_Side(uchar i, uchar *data)
{
	uchar j;
	
	for (j = Right_Line[i+1]; j > 2; j--)
	{
//		if (data[80*i+j] && data[80*i+j-1])//检测到连续两个白点，起到滤波的效果
		if (data[80*i+j])//检测到白点
		{
			Right_Add[i] = 0;		 //右边界不需要补线，清除标志位
			Right_Line[i] = j;		 //记录当前j值为第i行右边界
			
			break;
		}
	}
}

uchar Traversal_Left(uchar *data)	//左遍历边界, 返回左右边界的距离
{
	char j, White_Flag = 1;
	uchar Left_Line = 0, Right_Line = 79;
	
	for (j = 0; j < 78; j++)
	{
		if (White_Flag)	//先查找左边界
		{
			if (data[j] && data[j+1] && data[j+2])//检测到连续三个白点，起到滤波的效果
			{
				Left_Line = j;	//记录当前j值为本行左边界
			}
		}
		else
		{
			if (!data[j] && !data[j+1] && !data[j+2])//检测到连续三个黑点，起到滤波的效果
			{
				Right_Line = j;//记录当前j值为本行右边界
				
				break;
			}
		}
	}
	
	return (Right_Line - Left_Line);
}

uchar Traversal_Right(uchar *data)	//右遍历边界, 返回左右边界的距离
{
	char j, White_Flag = 1;
	uchar Left_Line = 0, Right_Line = 79;
	
	for (j = 79; j >= 2; j--)
	{
		if (White_Flag)	//先查找右边界
		{
			if (data[j] && data[j-1] && data[j-2])//检测到连续三个白点，起到滤波的效果
			{
				Right_Line = j;//记录当前j值为本行右边界
			}
		}
		else
		{
			if (!data[j] && !data[j-1] && !data[j-2])//检测到连续三个黑点，起到滤波的效果
			{
				Left_Line = j;	//记录当前j值为本行左边界
				
				break;
			}
		}
	}
	
	return (Right_Line - Left_Line);
}

void Traversal_Left_Line(uchar i, uchar *data, uchar *Left_Line, uchar *Right_Line)//左遍历边界, 记录左右边界
{
	char j, White_Flag = 1;
	
	for (j = 0; j < 78; j++)
	{
		if (White_Flag)	//先查找左边界
		{
			if (data[j] && data[j+1] && data[j+2])//检测到连续三个白点，起到滤波的效果
			{
				White_Flag = 0; //左边界已找到，清除标志位
				Left_Add[59] = 0;//左边界不需要补线，清除标志位
				*Left_Line = j;	//记录当前j值为本行左边界
			}
		}
		else	//找到左边界以后查找右边界
		{
			if (!data[j] && !data[j+1] && !data[j+2])//检测到连续三个黑点，起到滤波的效果
			{
				Right_Add[59] = 0;//右边界不需要补线，清除标志位
				*Right_Line = j;//记录当前j值为本行右边界
				
				break;
			}
		}
	}
}

void Traversal_Right_Line(uchar i, uchar *data, uchar *Left_Line, uchar *Right_Line)//右遍历边界, 记录左右边界
{
	char j, White_Flag = 1;
	
	for (j = 79; j >= 2; j--)
	{
		if (White_Flag)	//先查找右边界
		{
			if (data[j] && data[j-1] && data[j-2])//检测到连续三个白点，起到滤波的效果
			{
				White_Flag = 0; //右边界已找到，清除标志位
				Right_Add[59] = 0; 	//右边界不需要补线，清除标志位
				*Right_Line = j;//记录当前j值为本行右边界
			}
		}
		else	//找到右边界以后查找左边界
		{
			if (!data[j] && !data[j-1] && !data[j-2])//检测到连续三个黑点，起到滤波的效果
			{
				Left_Add[59] = 0;	//左边界不需要补线，清除标志位
				*Left_Line = j;	//记录当前j值为本行左边界

				break;
			}
		}
	}
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
  
  while(srclen --)
  {
    tmpsrc=0;
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x80;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x40;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x20;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x10;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x08;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x04;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x02;
    }
    
    if(*dst++ == colour[black])
    {
      tmpsrc = tmpsrc + 0x01;
    }
    
    *src++ = tmpsrc;
  }
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
