#include<stdlib.h>
#include<string.h>
#include<stdio.h>
//定义结构体
typedef struct _custom
{
	int C_custkey;    	   	
	char C_mkgseg[10];
}Customer;				   

typedef struct _orders
{
	long O_orderkey;    
	int O_custkey;    	 
	char O_orderdate;	 
}Orders;				 

typedef struct _lineitem
{
	int L_orderkey;		
	long L_extendedprice;
	char L_shipdate; 
}Lineitem; 

typedef struct _result
{
	int L_orderkey;		
	char O_orderdate;	 
	double L_extendedprice;
}Result;
//初始化指针
Customer *custom=NULL;
Orders *order=NULL;
Lineitem *lineitem=NULL;
char ob1[]="customers.txt",ob2[]="orders.txt",ob3[]="lineitem.txt";
//原本想要使用同一个读取函数，但是结构体不一样，我没有想到怎么将其作为参数去赋值
Customer *Getcusdata(char object[])
{
	FILE *fp;
	int i=0,b;
	custom = (Customer *)malloc(101*sizeof(Customer));
	fp=fopen("customers.txt","r");
	if(fp == NULL)
	{	printf("%s is not exist!",object);
	}
	for(i=0;i<100;i++)
	{

		fscanf(fp,"%d|%s",&custom[i].C_custkey,&custom[i].C_mkgseg);//这里可以直接根据格式读取，不需要用strtok函数去分割字符串
		printf("%d\n",custom[i].C_mkgseg);
	}
	fclose(fp);
	return custom;
}
Orders *Getorderdata(char object[])
{
	FILE *fp;
	int i=0;
	order = (Orders *)malloc(1001*sizeof(Orders));
	fp=fopen(object,"r");
	if(fp == NULL)
	{	printf("%s open orders.txt!",object);
	}
	for(i=0;i<1000;i++)
	{
		fscanf(fp,"%d|%l|%s",&order[i].O_custkey,&order[i].O_orderkey,&order[i].O_orderdate);
		order[i].O_custkey=order[i].O_custkey%100;
		printf("%d",order[i].O_custkey);
	}
	fclose(fp);
	return order;
}
Lineitem *Getlinedata(char object[])
{
	FILE *fp;
	int i=0;
	lineitem = (Lineitem *)malloc(4001*sizeof(Lineitem));
	fp=fopen(object,"r");
	if(fp == NULL)
	{	printf("%s open orders.txt!",object);
	}
	for(i=0;i<4000;i++)
	{
		fscanf(fp,"%d|%l|%s",&lineitem[i].L_orderkey,&lineitem[i].L_extendedprice,&lineitem[i].L_shipdate);
		printf("%d",lineitem[i].L_orderkey);
	}
	fclose(fp);
	return 	lineitem;
}

Result * select(Customer * cus,Orders * ord,Lineitem * item,char * order_date,char * ship_date)
{
	int i,j,k,l=0,m=0;
	Result * result1=NULL;
	Result * result2=NULL;
	Result  temp;
	result1 = (Result *)malloc(1000*sizeof(Result));
	result2 = (Result *)malloc(1000*sizeof(Result));
	for(i=0;i<100;i++)
	{
		for(j=0;j<4000;j++)
		{
			for(k=0;k<1000;k++)
			if(cus[i].C_custkey==ord[j].O_custkey&&ord[j].O_orderkey==item[k].L_orderkey&&(strcmp(ord[j].O_orderdate,order_date)<0)&&(strcmp(item[k].L_shipdate,ship_date)>0))
			{
				result1[l].L_orderkey=item[k].L_orderkey;
				strcpy(result1[l].O_orderdate,ord[j].O_orderdate);
				result1[l].L_extendedprice=item[k].L_extendedprice;
				l++;
			}
		}
	}
	/*printf("求和\n\n\n");*/
	for(i=0;i<l;i++)
	{
		//printf("%d\n",i);
		if(i==0)
		{
			result2[m].L_orderkey = result1[i].L_orderkey;
			strcpy(result2[m].O_orderdate,result1[i].O_orderdate);
			result2[m].L_extendedprice = result1[i].L_extendedprice;
			continue;
		}
		if(result1[i].L_orderkey==result1[i-1].L_orderkey)
		{
			result2[m].L_extendedprice = result2[m].L_extendedprice + result1[i].L_extendedprice;
			
		}
		else
		{
			
			m++;
			result2[m].L_orderkey = result1[i].L_orderkey;
			strcpy(result2[m].O_orderdate,result1[i].O_orderdate);
			result2[m].L_extendedprice = result1[i].L_extendedprice;
			
		}
	}
	for(i=0;i<m-1;i++)
	{
		for(j=0;j<m-1-i;j++)
		{
			if(result2[j].L_extendedprice<result2[j+1].L_extendedprice)
			{
				temp.L_extendedprice=result2[j].L_extendedprice;
				temp.L_orderkey=result2[j].L_orderkey;
				strcpy(temp.O_orderdate,result2[j].O_orderdate);
				result2[j].L_extendedprice=result2[j+1].L_extendedprice;
				result2[j].L_orderkey=result2[j+1].L_orderkey;
				strcpy(result2[j].O_orderdate,result2[j+1].O_orderdate);
				result2[j+1].L_extendedprice=temp.L_extendedprice;
				result2[j+1].L_orderkey=temp.L_orderkey;
				strcpy(result2[j+1].O_orderdate,temp.O_orderdate);
			}
		}
	}
	return result2;
}

int run(char *a,char *b,char *c,int d)
{
	int i;
	custom =Getcusdata(ob1);
	order = Getorderdata(ob2);
	lineitem = Getlinedata(ob3);
	int limit=d;
	char order_date[]=" ";
	char ship_date[]=" ";
	strcpy(order_date,b);
	strcpy(ship_date,c);
	Result *result1=NULL;
	result1=select(custom,order,lineitem,order_date,ship_date);
	printf("L_orderkey|O_orderdate|revenue\n");
	for(i=0;i<limit;i++)
	{
		printf("%-10d|%-11s|%-20.2lf\n",result[i].L_orderkey,result[i].O_orderdate,result[i].L_extendedprice);
	}
	return 0;
}

int main(int argc,char **argv)
{
	int i;
	int a;

	unsigned int n=atoi(argv[4]);
	for(i=1;i<=n;i++)
	{
		unsigned int t=atoi(argv[4*i+4]);
		a=run(argv[4*i+1],argv[4*i+2],argv[4*i+3],t);
	}
	return 0;
}

