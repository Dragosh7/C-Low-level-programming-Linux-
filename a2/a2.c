#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"
#include <stdlib.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>
#include <stdbool.h>

typedef struct
{
    int process_nr;
    int thread_nr;
    pthread_mutex_t *lock;
    pthread_mutex_t *lock1;
    pthread_mutex_t *lock2;
    pthread_cond_t *cond;
    pthread_cond_t *cond1;
    pthread_cond_t *cond2;
    sem_t *sync1;
    sem_t *sync2;
} TH_STRUCT;

typedef struct 
{
    int thread_id;
    sem_t *logSemMutex; // pe post de lock
    sem_t *logSemBarrier; // permite sa treaca doar 4 threaduri simultan

}BARRIER_STRUCT;

int order = 1; // pentru a porni threadurile in ordine
int ok = 0;
int closed = -1;

void *same_proc(void *arg)
{
    TH_STRUCT *params = (TH_STRUCT *)arg;

    pthread_mutex_lock(params->lock);

    while (params->thread_nr != order)
    {
        pthread_cond_wait(params->cond, params->lock);
    }

    order++;
    pthread_cond_broadcast(params->cond);
    pthread_mutex_unlock(params->lock);

    if (params->thread_nr == 2)
    {
        pthread_mutex_lock(params->lock1);
        while (!ok) // thread 2 won't start until thread 3 will start
        {
            pthread_cond_wait(params->cond1, params->lock1);
        }
        pthread_mutex_unlock(params->lock1);
    }

    info(BEGIN, 3, params->thread_nr);

    if (params->thread_nr == 3)
    {
        pthread_mutex_lock(params->lock2);
        ok = 1;
        pthread_cond_broadcast(params->cond1); // signal for 2 that waiting for 3 is over

        while (closed != 2) // thread 3 won't end until thread 2 ends
        {
            pthread_cond_wait(params->cond2, params->lock2);
        }
        pthread_mutex_unlock(params->lock2);
    }

    info(END, 3, params->thread_nr);
    closed = params->thread_nr;            // knowing which thread has been closed
    pthread_cond_broadcast(params->cond2); // signal for thread 3 that thread 2 ended

    return NULL;
}
int validate=0;//closed
int cond=0;//ok

void* th_barrier(void* param)
{
    BARRIER_STRUCT *s = (BARRIER_STRUCT*)param;
    int nr_perm;

    if(s->thread_id == 15)
    {
        validate = 1; //ma asigur ca T10 intra primul
        sem_wait(s->logSemBarrier);
        info(BEGIN, 7, s->thread_id);

        sem_wait(s->logSemMutex);
        info(END, 7, s->thread_id);

        cond = 1; //le da drumul celorlalte thread-uri pt ca T10 e finalizat
        sem_post(s->logSemBarrier);
    }
    else
    {
        while(!validate);
        sem_getvalue(s->logSemBarrier, &nr_perm);
        if(nr_perm == 0){
            sem_post(s->logSemMutex);
        }
        sem_wait(s->logSemBarrier);
        info(BEGIN, 7, s->thread_id);
        while(!cond);
        info(END, 7, s->thread_id);
        sem_post(s->logSemBarrier);
    }

    return NULL;
}

int main()
{
    init();

    info(BEGIN, 1, 0);

    // process 2
    if (fork() == 0)
    {

        info(BEGIN, 2, 0);
        // process 4
        if (fork() == 0)
        {
            info(BEGIN, 4, 0);
            info(END, 4, 0);
        }
        else
        {
            wait(NULL);
            info(END, 2, 0);
        }
    }
    else
    {
        wait(NULL);
        // process 3
        if (fork() == 0)
        {

            info(BEGIN, 3, 0);
            // threaduri in procesul 3
            pthread_t tid[5];
            TH_STRUCT params[5];
            pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_t lock1 = PTHREAD_MUTEX_INITIALIZER;
            pthread_mutex_t lock2 = PTHREAD_MUTEX_INITIALIZER;
            pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
            pthread_cond_t cond1 = PTHREAD_COND_INITIALIZER;
            pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;

            for (int i = 0; i < 5; i++)
            {
                params[i].process_nr = 3;
                params[i].thread_nr = i + 1;
                params[i].lock = &lock;
                params[i].lock1 = &lock1;
                params[i].lock2 = &lock2;
                params[i].cond = &cond;
                params[i].cond1 = &cond1;
                params[i].cond2 = &cond2;
                pthread_create(&tid[i], NULL, same_proc, &params[i]);
            }

            for (int i = 0; i < 5; i++)
            {
                pthread_join(tid[i], NULL);
            }
            pthread_mutex_destroy(&lock);
            pthread_mutex_destroy(&lock1);
            pthread_mutex_destroy(&lock2);
            pthread_cond_destroy(&cond);
            pthread_cond_destroy(&cond1);
            pthread_cond_destroy(&cond2);

            // process 5
            if (fork() == 0)
            {
                info(BEGIN, 5, 0);
                info(END, 5, 0);
            }
            else
            {
                wait(NULL);
                if (fork() == 0)
                {
                    // process 6
                    info(BEGIN, 6, 0);
                    info(END, 6, 0);
                }
                else
                {
                    wait(NULL);
                    info(END, 3, 0);
                }
            }
        }
        else
        {
            wait(NULL);
            // process 7
            if (fork() == 0)
            {
                info(BEGIN, 7, 0);
                    sem_t logSem[2];
                    BARRIER_STRUCT params[40];
                    pthread_t tid[40];

                    sem_init(&logSem[0], 0, 0);
                    sem_init(&logSem[1], 0, 4); //semaphore Barrier (cu cel mult 4 threaduri)

                    for(int i = 0; i < 40; i++)
                    {
                        params[i].logSemMutex = &logSem[0];
                        params[i].logSemBarrier = &logSem[1];
                        params[i].thread_id = i + 1;
                        pthread_create(&tid[i], NULL, th_barrier, &params[i]);
                    }
                    for(int i = 0; i< 40; i++){
                        pthread_join(tid[i], NULL);
                    }

                    for(int i = 0; i < 2; i++){
                        sem_destroy(&logSem[i]);
                    }
                info(END, 7, 0);
            }
            else
            {
                wait(NULL);
                info(END, 1, 0);
            }
        }
    }

    return 0;
}
