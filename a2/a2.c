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
    sem_t *sem; // pe post de lock
    sem_t *semBarrier; // permite sa treaca doar 4 threaduri simultan

}BARRIER_STRUCT;

typedef struct{
    int proc_nr;
    int th_nr;
    sem_t *sync1;
    sem_t *sync2;
}TH_SYNC; 

int order = 1; // pentru a porni threadurile in ordine
int ok = 0;
int closed = -1;

//pentru bariere
int condition=0;//4 threaduri trebuie sa fie pornite cand t15 isi incheie executia
int finished=0;//cand t15 se va finaliza

//pentru sincronizare intre procese diferite
sem_t semSync1,semSync2;
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
  
    //  if (params->thread_nr == 5)
    // {
    //     sem_wait(&semSync2); //3.5 nu porneste pana nu termina 2.4
    // }

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
    //  if (closed == 5)
    // {
    //     sem_post(&semSync1); //2.1 nu incepe decat daca a terminat
    // }

    pthread_cond_broadcast(params->cond2); // signal for thread 3 that thread 2 ended

    return NULL;
}

void* th_barrier(void* param)
{
    BARRIER_STRUCT *s = (BARRIER_STRUCT*)param;
    int nr_perm;

    if(s->thread_id == 15)
    {
        condition = 1; //T15 intra primul
        sem_wait(s->semBarrier);
        info(BEGIN, 7, s->thread_id);

        sem_wait(s->sem);// thread-ul si-a terminat executia
        info(END, 7, s->thread_id);

        finished = 1; //le da drumul celorlalte thread-uri pt ca T15 e finalizat
        sem_post(s->semBarrier);
    }
    else
    {
        while(!condition);
        sem_getvalue(s->semBarrier, &nr_perm); //odata ce s-au scazut 4 si a ajuns la 0
        if(nr_perm == 0){
            sem_post(s->sem);
        }
        sem_wait(s->semBarrier);
        info(BEGIN, 7, s->thread_id);
        while(!finished);/// Wait for thread 15 to finish
        info(END, 7, s->thread_id);
        sem_post(s->semBarrier);
    }

    return NULL;
}
void* th_diff_proc(void* arg)
{  
    TH_SYNC *params = (TH_SYNC*)arg;
   
    info(BEGIN, params->proc_nr, params->th_nr);
    // if (params->th_nr == 1)
    // {
    //    sem_wait(&semSync1);
    // }
    //  if (params->th_nr == 4)
    // {
    //     sem_post(&semSync2);
    // }
    info(END, params->proc_nr, params->th_nr);
   
    return NULL;
}

int main()
{
    init();

      
                   
                    sem_init(&semSync1, 0, 0);
                    sem_init(&semSync2, 0, 0); //semaphore Barrier (cu cel mult 4 threaduri)
    info(BEGIN, 1, 0);

    // process 2
    if (fork() == 0)
    {

        info(BEGIN, 2, 0);
         pthread_t s_tid[5];
          TH_SYNC s_params[5];
         for (int i = 0; i<5; i++)
                    {
                        s_params[i].proc_nr = 2;
                        s_params[i].th_nr = i+1;
                        s_params[i].sync1= &semSync1;

                        s_params[i].sync2= &semSync2;
                        pthread_create(&s_tid[i], NULL, th_diff_proc, &s_params[i]);
                    }

                    for (int i = 0; i<5; i++)
                    {
                        pthread_join(s_tid[i], NULL);
                    }
                    
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
                params[i].sync1 = &semSync1;
                params[i].sync2 = &semSync2;
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
                    sem_t sem,semBarrier;
                    BARRIER_STRUCT params[40];
                    pthread_t tid[40];

                    sem_init(&sem, 0, 0);
                    sem_init(&semBarrier, 0, 4); //semaphore Barrier (cu cel mult 4 threaduri)

                    for(int i = 0; i < 40; i++)
                    {
                        params[i].sem = &sem;
                        params[i].semBarrier = &semBarrier;
                        params[i].thread_id = i + 1;
                        pthread_create(&tid[i], NULL, th_barrier, &params[i]);
                    }
                    for(int i = 0; i< 40; i++){
                        pthread_join(tid[i], NULL);
                    }

                        sem_destroy(&sem);
                        sem_destroy(&semBarrier);
                    
                info(END, 7, 0);
            }
            else
            {
                wait(NULL);
                info(END, 1, 0);
            }
        }
    }
    
    sem_destroy(&semSync1);
    sem_destroy(&semSync2);

    return 0;
}
