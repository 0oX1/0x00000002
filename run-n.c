#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include <pthread.h>
#define HAVE_STRUCT_TIMESPEC
typedef struct customer
{
	int c_custkey;
	char c_ment[10];
}cus;

typedef struct lineitme
{
	int l_orderkey;
	float l_price;
	char sdate[10];
}line;

typedef struct orders
{
	int o_orderkey;
	long int o_custkey;
	char odate[10];
}order;

typedef struct result
{
	int l_orderkey;
	char o_orderdate[10];
	double l_extendedprice;
}select_result;

void* ThreadFuc(void* t);
char** argvs;

void* ThreadFuc(void* can)
{
	int q = *(int*)(void*)can;
	printf("ID:%d\n", q);
	char order_date[] = " ";
	char ship_date[] = " ";
	int control_1, control_2, a = 0, b = 0, c = 0;
	strcpy(order_date, argvs[6 + q * 4]);
	strcpy(ship_date, argvs[7 + q * 4]);
	cus customer[1000];
	line lineitme[3000];
	order orders[10000];

	FILE* fp;
	if ((fp = fopen(argvs[1], "r")) == NULL)
	{
		printf("Can not1\n");
		exit(1);
	}
	while (!feof(fp))
	{
		fscanf(fp, "%d|%s", &customer[c].c_custkey, &customer[c].c_ment);
		c++;
	}
	fclose(fp);

	if ((fp = fopen(argvs[3], "r")) == NULL)
	{
		printf("Can not2\n");
		exit(1);
	}
	while (!feof(fp))
	{
		fscanf(fp, "%d|%f|%s", &lineitme[c].l_orderkey, &lineitme[c].l_price, &lineitme[c].sdate);
		c++;
	}
	fclose(fp);
	if ((fp = fopen(argvs[2], "r")) == NULL)
	{
		printf("Can not3\n");
		exit(1);
	}
	while (!feof(fp))
	{
		fscanf(fp, "%d|%ld|%s", &orders[c].o_orderkey, &orders[c].o_custkey, &orders[c].odate);
		c++;
	}
	fclose(fp);
	select_result result1[2000];
	select_result result2[2000];

	for (control_2 = 0; control_2 < 5000; control_2++)//orders j
	{
		for (control_1 = 0; control_1 < 1000; control_1++)//lineitme k
			if (customer[c].c_custkey == (orders[control_2].o_custkey % 100) && orders[control_2].o_orderkey == lineitme[control_1].l_orderkey && (strcmp(orders[control_2].odate, order_date) < 0) && (strcmp(lineitme[control_1].sdate, ship_date) > 0))
			{
				result1[a].l_orderkey = lineitme[control_1].l_orderkey;
				strcpy(result1[a].o_orderdate, orders[control_2].odate);
				result1[a].l_extendedprice = lineitme[control_1].l_price;
				a++;

			}
	}

	for (c = 0; c < a; c++)
	{
		if (c == 0)
		{
			result2[b].l_orderkey = result1[c].l_orderkey;
			strcpy(result2[b].o_orderdate, result1[c].o_orderdate);
			result2[b].l_extendedprice = result1[c].l_extendedprice;
			continue;
		}
		if (result1[c].l_orderkey == result1[c - 1].l_orderkey)
		{
			result2[b].l_extendedprice = result2[b].l_extendedprice + result1[c].l_extendedprice;

		}
		else
		{

			b++;
			result2[b].l_orderkey = result1[c].l_orderkey;
			strcpy(result2[b].o_orderdate, result1[c].o_orderdate);
			result2[b].l_extendedprice = result1[c].l_extendedprice;

		}

	}
	printf("l_orderkey|o_orderdate|revenue\n");
	for (c = 0; c < atoi(argvs[8 + 4 * q]); c++)
	{

		if (result2[c].l_orderkey == 0)
		{
			continue;
		}
		else
		{
			printf("%-10d|%-11s|%-20.2lf\n", result2[c].l_orderkey, result2[c].o_orderdate, result2[c].l_extendedprice);
		}

	}

}
int main(int argc, char** argv)
{
	argvs = argv;
	int  a, b;
	int i;
	void* retval;
	b = atoi(argv[4]);
	pthread_t* pthreads = (pthread_t*)malloc(sizeof(pthread_t) * p);
	for (a = 0; a < b; a++)
	{
		pthread_t single;
		pthreads[a] = single;
		
		int errorCode = pthread_create(&single, NULL, ThreadFuc, (void*)& a);
		if (errorCode)
		{
			printf("线程创建失败:%d\n", errorCode);
			continue;
		}
		pthread_join(pthreads[a], &retval);
	}
	return 0;
}




