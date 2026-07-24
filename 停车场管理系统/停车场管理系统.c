#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct ParkTime                            //记录停车时长
{
    int in_year,in_month,in_day,in_hour;           //车辆入库时间
    int out_year,out_month,out_day,out_hour;       //车辆出库时间
}pkt;

typedef struct CarInformation
{
    char car_number[20];          	//车牌号
    char car_color[10];           	//车身颜色
    char car_type[10];            	//车型
    pkt parktime;                 	//车辆进出时间结构体
    int park_fee;                 	//停车费用
                                  	
    struct CarInformation *next;  	//链表后继指针
}carnode;

void AddInfo(carnode *head);                                        //添加车辆信息 
void CurrentTime(pkt *p);                                           //获取当前时间 
int CheckNumber(carnode *head,char targetnumber[20]);               //查重车牌（返回0、1） 
void DeleteInfo(carnode *head,char targetnumber[20]);               //删除车辆信息 
void PrintInfo(carnode *target);                                    //输出车辆信息 
void SearchInfo(carnode *head,char targetnumber[20]);               //查找车辆信息 
void ModifyInfo(carnode *head,char targetnumber[20]);	            //修改车辆信息 
void CountParkHour(carnode *head, char targetnumber[20]);           //统计停车时间 
void PrintAll(carnode *head);                                       //显示所有车辆信息
void SaveCarToFile(carnode *head);                                  //车辆信息写入文件 
void ShowAllAndTime(carnode *head);                                 //显示所有车辆停车时长及费用
 
int main() 
{ 
	int n;                                                                  //程序是否继续进行 
	char targetnumber[20];                                                  //目标车牌 
	carnode *head = (carnode*)malloc(sizeof(carnode));
	head->next = NULL;
	if (head == NULL)
	{
		printf("内存分配失败!\n");
		return -1;
	}
	int mode;
	while (1)
	{
		printf("================首页================\n");
	    printf("1.汽车信息模块\n");
	    printf("2.普通用户模块\n");
	    printf("3.管理员用户模块\n");
	    printf("......输入0退出程序......\n");
	    printf("====================================\n");
	    printf("请输入1、2、3来选择您需要的模块:");
	    scanf("%d",&mode);
    	if (mode == 0)                                                               //零、退出程序 
    	{
    		printf("已退出程序......\n");
    		return 0;
		}
	    
		else if (mode == 1)                                                          //一、汽车信息模块
	    {
	    	int func2;
	    	while(1)
	    	{
		        printf("====================================\n");                                  
		        printf("1.添加汽车信息\n");
		        printf("2.删除汽车信息\n");
		        printf("3.查找汽车信息\n");
		        printf("4.修改汽车信息\n");
		        printf("5.停车时间统计\n");
		        printf("6.汽车信息显示\n");
		        printf("7.汽车信息保存\n");
		        printf("......输入0以返回首页......\n");
		        printf("====================================\n");
		        printf("请输入数字来选择你的功能:");
		        scanf("%d",&func2);
		        if (func2 == 0)
				{
					break;
				 } 
		        switch (func2)
		        {
		        	case 1:
			        	{
			        		int i;
							int car_number;
							printf("请输入要存入信息的车辆数:");
							scanf("%d",&car_number);
							for (i = 1;i<=car_number;i++)
							{
								AddInfo(head);
								printf("*%d",i);
							}
							break;	
						}
		        	case 2:
						{
							printf("请输入要删除的车牌号:");
							scanf("%s",targetnumber);
							DeleteInfo(head,targetnumber);
							while (1)
							{
								printf("是否继续删除?(1/0):");
								scanf("%d",&n);
								if (n)
								{
									printf("请输入要删除的车牌号:");
									scanf("%s",targetnumber);
									DeleteInfo(head,targetnumber);
									continue;
								}
								break;
							}
							break;
						}
					case 3:
						{
							printf("请输入要查找的车牌号:");
							scanf("%s",targetnumber);
							SearchInfo(head,targetnumber);
							while (1)
							{
								printf("是否继续查找？(1/0):");
								scanf("%d",&n);
								if (n)
								{
									printf("请输入要查找的车牌号:");
									scanf("%s",targetnumber);
									SearchInfo(head,targetnumber);
									continue;
								 } 
								break;
							}
							break; 
						}
					case 4:
						{
							printf("请输入要修改的车牌号:");
							scanf("%s",targetnumber);
							ModifyInfo(head,targetnumber);
							while (1)
							{
								printf("是否继续修改？(1/0):");
								scanf("%d",&n);
								if (n)
								{
									printf("请输入要修改的车牌号:");
									scanf("%s",targetnumber);
									ModifyInfo(head,targetnumber);
									continue;
								 } 
								break;
							}
							break;
						}
					case 5:
						{
							printf("请输入要统计停车时长的车牌号:");
							scanf("%s",targetnumber);
							CountParkHour(head,targetnumber);
							while (1)
							{
								printf("是否继续统计？(1/0):");
								scanf("%d",&n);
								if (n)
								{
									printf("请输入要统计停车时长的车牌号:");
									scanf("%s",targetnumber);
									CountParkHour(head,targetnumber);
									continue;
								 } 
								break;
							}
							break;
						}
					case 6:
						{
							PrintAll(head);
							break;
						}
					case 7:
						{
							SaveCarToFile(head);
							break;
					 	}	 
					default:
						{
							printf("错误指令！！！请重新输入\n");
						}
				}	
			}
		}
		
		else if (mode == 2)                                                       //二、普通用户模块 
		{
			int func3;
			while (1)
			{
				printf("====================================\n");                                  
		        printf("1.查看汽车信息\n");
		        printf("2.查看停车时间及费用\n");
		        printf("......输入0以返回首页......\n");
		        printf("====================================\n");
		        printf("请输入数字来选择你的功能:");
		        scanf("%d",&func3);
		        if (func3 == 0)
		        {
		        	break;
				}
				switch (func3)
				{
					case 1:
						{
							printf("请输入查询的车牌号:");
							scanf("%s",targetnumber);
							SearchInfo(head,targetnumber);
							break;
						}
					case 2:
						
						{
							printf("请输入查询的车牌号:");
							scanf("%s",targetnumber);
							CountParkHour(head,targetnumber);
							break;
						}
					default:
						{
							printf("错误指令！！！请重新输入\n");
						}
				}
			}
		}
		 
		else if (mode == 3)                                                                //三、管理员模块 
		{
			char password[30] = {"1234567890"};
			char in_password[30];
			int func4;
			printf("请输入管理员密码:");
			scanf("%s",in_password);
			if (strcmp(in_password,password) == 0)
			{
				while (1)
				{		
					printf("====================================\n");                                  
			        printf("1.显示车库中所有车辆停车时长及费用\n");
			        printf("......输入0以返回首页......\n");
			        printf("====================================\n");
			        printf("请输入数字来选择你的功能:");
			        scanf("%d",&func4);
					if (func4 == 0)
					{
						break;
					}
					switch (func4)
					{
						case 1:
							{
								ShowAllAndTime(head);
								break; 
							}
						default:
							{
								printf("错误指令！！！请重新输入\n");
							}
					}
				}
			}
			else
			{
				printf("密码错误！！！即将返回首页......\n");
			}
		} 
	} 
}
    
//零、工具函数
//1.查重车牌（有则返回1，无则返回0）(遍历一遍)
int CheckNumber(carnode *head,char targetnumber[20])
{
	carnode *p = head->next;
	while (p != NULL)
	{
		if (strcmp(p->car_number,targetnumber) == 0)
		{
			return 1;
		}
		p = p->next;
	}
	return 0;
 } 
 
//2.获取系统当前时间 
void CurrentTime(pkt *p)
{
 	time_t now = time(NULL);
 	struct tm *inner = localtime(&now);
 	p->out_year = inner->tm_year + 1900;
 	p->out_month = inner->tm_mon + 1;
 	p->out_day = inner->tm_mday;
 	p->out_hour = inner->tm_hour;
}

//3.输出车辆信息
void PrintInfo(carnode *target)
{
	printf("车牌号: %s\n",target->car_number);
	printf("车身颜色: %s\n",target->car_color);
	printf("车型: %s\n",target->car_type);
 } 


//一、汽车信息模块
//1.添加车辆信息
void AddInfo(carnode *head)        
{
	printf("\n");
    carnode *add = (carnode*)malloc(sizeof(carnode));                     //创建节点
    if (add == NULL)
    {
        printf("内存分配失败!\n");
        return;
    }
    add->next = NULL;
    printf("请输入车牌号:");                                              //输入车牌号+查重
    scanf("%s",add->car_number);
    if (CheckNumber(head,add->car_number))
    {
    	printf("该车辆已存在,无法重复添加!\n");                           //查重 
    	free(add);
    	return;
	}
	printf("请输入车辆颜色:");                                            //不包含出库 
    scanf("%s",add->car_color);
    printf("请输入车型(轿车/SUV/货车):");
    scanf("%s",add->car_type);
    printf("请输入车辆入库时间(年/月/日/时):");
    scanf("%d/%d/%d/%d",&add->parktime.in_year,
						&add->parktime.in_month,
						&add->parktime.in_day,
						&add->parktime.in_hour);
	add->park_fee = 0;
	carnode *p = head;                                                    //在链表末位插入新的车辆
	while (p->next != NULL)
	{
		p = p->next;
	 } 
	p->next = add;
	printf("车辆信息添加成功!");
}

//2.删除车辆信息
void DeleteInfo(carnode *head,char targetnumber[20])
{
	carnode *p,*pr;
	p = head->next;
	pr = head;
	if (head->next == NULL)
	{
		printf("系统暂无车辆信息\n");
		return;
	}
	while(p != NULL && strcmp(p->car_number,targetnumber) != 0)          //未查找到，指针后移 
	{
		pr = p;
		p = p->next;
	}
	if (p == NULL)                                                       //查找到车牌，删除，并释放内存 
	{
		printf("未查找到该车牌\n");
		return;
	}
	else                                                                 //遍历完链表，没有查找到 
	{
		pr->next = p->next;
		free(p);
		printf("车辆信息删除成功！\n");
	}
 } 

//3.查找车辆信息(如果找到了就输出车辆信息)
 void SearchInfo(carnode *head,char targetnumber[20])
 {
 	carnode *p;
 	p = head->next;
 	if (head->next == NULL)                      
	{
		printf("系统暂无车辆信息\n");
		return;
	}
 	while(p != NULL && strcmp(p->car_number,targetnumber) != 0)                     //未查找到，指针后移 
 	{
 		p = p->next;
	 }
	if (p == NULL)                                                                  //遍历完，未查找到目标车牌， 
	{
		printf("系统暂无车辆信息\n");
		return;		
	}
	else
	{
		PrintInfo(p);
	}
 }
 
 //4.修改车辆信息
void ModifyInfo(carnode *head,char targetnumber[20])
 {
 	carnode *p;
 	p = head->next;
 	if (head->next == NULL)                      
	{
		printf("系统暂无车辆信息\n");
		return;
	}
 	while(p != NULL && strcmp(p->car_number,targetnumber) != 0)                     //未查找到，指针后移 
 	{
 		p = p->next;
	 }
	if (p == NULL)                                                                  //遍历完，未查找到目标车牌， 
	{
		printf("系统暂无车辆信息\n");
		return;		
	}
	else
	{
		PrintInfo(p);
		printf("车牌修改:");
		scanf("%s",p->car_number);
		printf("颜色修改:");
		scanf("%s",p->car_color);
		printf("车型修改:");
		scanf("%s",p->car_type);
		printf("入库时间修改:");
		printf("年:");
		scanf("%d",&p->parktime.in_year);
		printf("月:");
		scanf("%d",&p->parktime.in_month);
		printf("日:");
		scanf("%d",&p->parktime.in_day);
		printf("时:");
		scanf("%d",&p->parktime.in_hour);
		printf("车辆信息修改完毕。");
	}
 } 
 
 //5.统计停车时间 (输出车辆停车信息)
void CountParkHour(carnode *head, char targetnumber[20])
{
    carnode *p = head->next;
    pkt nowTime;
    CurrentTime(&nowTime);                                                               //获取系统时间 
 	while (p != NULL && strcmp(p->car_number, targetnumber) != 0)                        //查找车辆 
    {
        p = p->next;
    }
    if (p == NULL)
    {
        printf("未查询到该车牌车辆，无法统计停车时长！\n");
        return;
    }
	p->parktime.out_year = nowTime.out_year;                                             //把当前系统时间赋值给车辆出库时间
    p->parktime.out_month = nowTime.out_month;
    p->parktime.out_day = nowTime.out_day;
    p->parktime.out_hour = nowTime.out_hour;

    int inTotalHour = p->parktime.in_year * 365 * 24
                    + p->parktime.in_month * 30 * 24
                    + p->parktime.in_day * 24
                    + p->parktime.in_hour;

    int outTotalHour = p->parktime.out_year * 365 * 24
                     + p->parktime.out_month * 30 * 24
                     + p->parktime.out_day * 24
                     + p->parktime.out_hour;
    int stayHour = outTotalHour - inTotalHour;
    if (stayHour < 0) stayHour = 0; 													 // 防止时间输入错乱负数
	int pricePerHour = 10;
    p->park_fee = stayHour * pricePerHour;
	
	printf("=====车辆停车时长统计结果=====\n");                                          //输出停车时长的信息 
    printf("车牌号：%s\n", p->car_number);
    printf("入库时间：%d年%d月%d日 %d时\n",
           p->parktime.in_year, p->parktime.in_month,
           p->parktime.in_day, p->parktime.in_hour);
    printf("当前出库时间：%d年%d月%d日 %d时\n",
           p->parktime.out_year, p->parktime.out_month,
           p->parktime.out_day, p->parktime.out_hour);
    printf("累计停留小时：%d 小时\n", stayHour);
    printf("停车总费用：%d 元（单价10元/小时）\n", p->park_fee);
}

//6.输出所有车辆信息
void PrintAll(carnode *head)
{
	carnode *p;
	p = head->next;
	if (p == NULL)
	{
		printf("暂无车辆信息！\n");
	}
	while (p != NULL)
	{
		PrintInfo(p);
		printf("\n");
		p = p->next;
	}
	printf("已显示全部车辆信息。\n");
 } 

//7.车辆信息保存
void SaveCarToFile(carnode *head)
{
    FILE *fp = fopen("car.txt", "w");                                              //以只写模式打开文件 
    if (fp == NULL)
    {
        printf("文件打开失败，无法保存信息！\n");
        return;
    }

    carnode *p = head->next;
    if (p == NULL)
    {
        printf("暂无车辆数据，无需保存\n");
        fclose(fp);                                                                //无车辆信息，关闭文件 
        return;
    }
    while (p != NULL)                                                              //循环遍历链表，把每辆车信息写入文件
    {
                                                                                   
        fprintf(fp, "%s %s %s %d %d %d %d %d %d %d %d %d\n",
                p->car_number,
                p->car_color,
                p->car_type,
                p->parktime.in_year, p->parktime.in_month, p->parktime.in_day, p->parktime.in_hour,
                p->parktime.out_year, p->parktime.out_month, p->parktime.out_day, p->parktime.out_hour,
                p->park_fee);
        p = p->next;
    }

    fclose(fp);                                                                    //关闭文件
    printf("所有车辆信息已成功保存到 car.txt 文件！\n");
} 

//8.显示所有车辆停车时长
void ShowAllAndTime(carnode *head)
{
	carnode *p = head->next;
	if (p == NULL)
    {
        printf("系统中暂无车辆......\n");
        return;
    }
    pkt nowTime;
    CurrentTime(&nowTime);                                                                      //获取系统时间 
 	while (p != NULL)                      
    {
    	p->parktime.out_year = nowTime.out_year;
	    p->parktime.out_month = nowTime.out_month;
	    p->parktime.out_day = nowTime.out_day;
	    p->parktime.out_hour = nowTime.out_hour;
	
	    int inTotalHour = p->parktime.in_year * 365 * 24
	                    + p->parktime.in_month * 30 * 24
	                    + p->parktime.in_day * 24
	                    + p->parktime.in_hour;
	
	    int outTotalHour = p->parktime.out_year * 365 * 24
	                     + p->parktime.out_month * 30 * 24
	                     + p->parktime.out_day * 24
	                     + p->parktime.out_hour;
	    int stayHour = outTotalHour - inTotalHour;
	    if (stayHour < 0) stayHour = 0; 														 // 防止时间输入错乱负数
		int pricePerHour = 10;
	    p->park_fee = stayHour * pricePerHour;
		
		printf("=====车辆停车时长统计结果=====\n");
	    printf("车牌号：%s\n", p->car_number);
	    printf("入库时间：%d年%d月%d日 %d时\n",
	           p->parktime.in_year, p->parktime.in_month,
	           p->parktime.in_day, p->parktime.in_hour);
	    printf("当前出库时间：%d年%d月%d日 %d时\n",
	           p->parktime.out_year, p->parktime.out_month,
	           p->parktime.out_day, p->parktime.out_hour);
	    printf("累计停留小时：%d 小时\n", stayHour);
	    printf("停车总费用：%d 元（单价10元/小时）\n", p->park_fee);
        p = p->next;
    }
    printf("车辆停车时间及费用全部显示完毕！\n");
} 