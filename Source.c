#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <unistd.h>
#include <windows.h>

#define NUM_THREADS 2

#define UPDATE -1
#define GET 0
#define GETALL 1
#define FREETOUSE -2

pthread_cond_t jobdone;

char **queryFiles;

typedef struct
{
	float stanje;
	int state;
}users;

typedef struct
{
	int query;
	int target;
	float amount;
}querys;

pthread_mutex_t mut;

int numberOfusers;
users user[100];
querys querysForThread1[2][10];

void loadUsersData(char *file)
{
	int id;
	float numb;
	FILE *f = fopen(file, "r");

	while (!feof(f))
	{
		numberOfusers++;
		fscanf(f, "%d", &id);
		fscanf(f, "%f", &numb);
		user[id].stanje = numb;
		user[id].state = FREETOUSE;
	}
	fclose(f);
}

void loadQuerys(char *file, int n)
{
	int i, j;
	float hold;
	FILE *f;
	char q[10];
	j = 0;
	f = fopen(file, "r");
	while (!feof(f))
	{
		fscanf(f, "%s", &q);
		if (!strcmp(q, "GET"))
		{
			querysForThread1[n][j].query = GET;
			fscanf(f, "%d", &querysForThread1[n][j++].target);
		}
		else if (!strcmp(q, "GETALL"))
		{
			querysForThread1[n][j++].query = GETALL;
			//fscanf(f, "%d", &querysForThread1[j++].target);
		}
		else if (!strcmp(q, "UPDATE"))
		{
			querysForThread1[n][j].query = UPDATE;
			fscanf(f, "%d", &querysForThread1[n][j++].target);
			fscanf(f, "%f", &hold);
		}
	}
	fclose(f);
}

void get(int id, int tid)
{
	pthread_mutex_lock(&mut);
	while (user[id].state == UPDATE)
		pthread_cond_wait(&jobdone, &mut);
	printf("------- thread %d get nad %d pocinje.\n", tid, id);
	user[id].state = GET;
	pthread_mutex_unlock(&mut);

	printf("User %d has %.2f amount of money.\n", id, user[id].stanje);
	Sleep(1000);

	pthread_mutex_lock(&mut);
	user[id].state = FREETOUSE;
	printf("------- thread %d get nad %d zavrsava.\n", tid, id);
	pthread_cond_broadcast(&jobdone);
	pthread_mutex_unlock(&mut);
}

void update(int id, float s, int tid)
{
	pthread_mutex_lock(&mut);
	while (user[id].state != FREETOUSE)
		pthread_cond_wait(&jobdone, &mut);
	printf("------- thread %d update nad %d pocinje.\n", tid, id);
	user[id].state = UPDATE;
	pthread_mutex_unlock(&mut);

	Sleep(5000);
	user[id].stanje += s;
	printf("[UPDATE]\n");
	printf("User %d has %.2f amount of money.\n", id, user[id].stanje);

	pthread_mutex_lock(&mut);
	user[id].state = FREETOUSE;
	printf("------- thread %d update nad %d zavrsava.\n", tid, id);
	pthread_cond_broadcast(&jobdone);
	pthread_mutex_unlock(&mut);
}

void *executeQuerys(void *threadid)
{
	long tid;
	tid = (long)threadid;

	int i, j, target;
	float hold;
	FILE *f;
	char q[10];
	j = 0;
	f = fopen(queryFiles[tid + 1], "r");
	while (!feof(f))
	{
		fscanf(f, "%s", &q);
		if (!strcmp(q, "GET"))
		{
			fscanf(f, "%d", &target);
			get(target, tid);
		}
		else if (!strcmp(q, "GETALL"))
		{
			for (i = 0; i < numberOfusers ; i++)
			{
				get(i, tid);
			}
		}
		else if (!strcmp(q, "UPDATE"))
		{
			fscanf(f, "%d", &target);
			fscanf(f, "%f", &hold);

			update(target, hold, tid);
		}
	}
	fclose(f);

	pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
	queryFiles = argv;
	loadUsersData("Korisnici.txt");
	pthread_t thread[NUM_THREADS];
	pthread_attr_t attr;
	int rc;
	long t;
	void *status;

	pthread_cond_init(&jobdone, NULL);
	pthread_mutex_init(&mut, NULL);
	/* Initialize and set thread detached attribute */
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

	for (t = 0; t<NUM_THREADS; t++) {
		rc = pthread_create(&thread[t], &attr, executeQuerys, (void *)t);
		if (rc) {
			printf("ERROR; return code from pthread_create() is %d\n", rc);
			exit(-1);
		}
	}

	/* Free attribute and wait for the other threads */
	pthread_attr_destroy(&attr);
	for (t = 0; t<NUM_THREADS; t++) {
		rc = pthread_join(thread[t], &status);
		if (rc) {
			printf("ERROR; return code from pthread_join() is %d\n", rc);
			exit(-1);
		}
	}

	printf("Main: program completed. Exiting.\n");
	/*
	//loadQuerys("Upiti1.txt", 0);
	//loadQuerys("Upiti2.txt", 1);
	int i;
	for (i = 0; i < 10; i++)
	{
	printf("Query %d ima %d novca.\n", querysForThread1[1][i].query, querysForThread1[1][i].target);
	}
	*/
	/* Last thing that main() should do */
	pthread_cond_destroy(&jobdone);
	pthread_mutexattr_destroy(&mut);
	pthread_exit(NULL);

}