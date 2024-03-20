

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include "aesdsocket.h"

static void signal_handler(int signal_number)
{
    USER_LOGGING("%s","Caught signal, exiting");

    signal_sign = 1;
}

int signal_setup(int number, ...)
{
    int signal, rc;
    va_list ap;
    struct sigaction new_action;

    // Initialize new_action struct
    memset(&new_action, 0, sizeof(struct sigaction));
    
    new_action.sa_handler = signal_handler;
    va_start(ap, number);
    while(number > 0)
    {
        signal = va_arg(ap, int);
        rc = sigaction(signal, &new_action, NULL);
        if(rc != 0) {
            break;
        }
        number--;
    }

    va_end(ap);

    return rc;
    
}
