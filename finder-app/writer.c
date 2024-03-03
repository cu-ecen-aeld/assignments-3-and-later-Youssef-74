#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>

int main (int argc, char *argv[])
{
    // setup syslog logging using LOG_USER facility
    // open the syslog
    openlog(NULL, 0, LOG_USER);

    // check if you received two arguments 
    if(argc == 3)
    {
        int fd;
        fd  = open(argv[1], O_CREAT | O_WRONLY | O_TRUNC, 0766);
        // check if this directory exist and the file opened successfully
        if(fd == -1)
        {
            // log an error message notify there is no directory
            syslog(LOG_ERR, "Invalid Directory: %s \n please check the file directory!", argv[1]);
            return 1;
        }
        else {
            // in case the file opened successfully 
            // write to the file 
            const char *buf = argv[2];
            ssize_t nr;
            //size_t buf_size = sizeof(argv[2]);
            nr = write (fd, buf, strlen(buf));
            if(nr == -1)
            {
                // log an error message >> can't write to the file 
                syslog(LOG_ERR, "Can't write to the file");
                // close the file
                close(fd);
                return 1;
            }
            else {
                // log a debug message >> writing <string> to <file>
                syslog(LOG_DEBUG, "writing %s to %s", argv[2], argv[1]);
            }
        }
        // close the file
        close(fd); 
    }
    else 
    {
        // if not Log an error message >> Invalid number of argumantes                 
        syslog(LOG_ERR, "Invalid number of argumantes: %d", argc);
        return 1;
    }
    
    return 0;
     
}