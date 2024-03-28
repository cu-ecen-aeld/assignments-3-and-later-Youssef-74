#include <stdarg.h>
#include "manipulate_file.h"


int file_create(char *pathname, ...)
{
    int fd, number, flags = 0;
    va_list ap;

    va_start(ap, pathname);

    number = va_arg(ap, int);
    while(number-- > 0)
    {
        flags |= va_arg(ap, int);
    }

    fd = open(pathname, flags, S_IRWXU | S_IRWXG | S_IRWXO);
    if(fd == -1)
    {
        ERROR_HANDLER(open);
    }

    va_end(ap);

    return fd;
}

int file_write(int fd, char *buffer, int size)
{
    ssize_t s;
    // set the pointer file to the end of the file 
    lseek(fd, 0, SEEK_END);

    // write to the file
    if ((s = write(fd, buffer, size)) == -1) {
        perror("Failed to write to file");
        //close(fd);
        pthread_exit(NULL);
    }
    return s;
}

int file_size(int fd)
{
    int front, end;
    
    end = lseek(fd, 0, SEEK_END);
    front = lseek(fd, 0, SEEK_SET);
    
    return end - front;
}

int file_read(int fd, char *buffer, int size, off_t offset)
{
    int rc = 0;
    // Set the file offset to the starting position
    if (lseek(fd, offset, SEEK_SET) == -1) {
        perror("Failed to seek to start position");
        close(fd);
        pthread_exit(NULL);
    }

    rc = read(fd, buffer, size);
    if(rc == -1){
        perror("Failed to read from file");
        close(fd);
        pthread_exit(NULL);
    }
    return rc;
}

void file_delete(char *file)
{
    int rc = 0;
    struct stat status;
    memset(&status, 0, sizeof(struct stat));

    rc = stat(file, &status);
    if (rc != -1) {
        remove(file);
    }
}
