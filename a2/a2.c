#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "a2_helper.h"

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
    else{
        wait(NULL);
        // process 3
        if (fork() == 0)
        {

            info(BEGIN, 3, 0);
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
                else{
                    wait(NULL);
                    info(END,3,0);
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
            info(END, 7, 0);
        }
        else{
            wait(NULL);
            info(END,1,0);
        }
        }

        
    }
       
        
    

    return 0;
}
