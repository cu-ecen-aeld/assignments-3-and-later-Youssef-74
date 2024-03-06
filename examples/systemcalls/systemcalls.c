#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
*/
bool do_system(const char *cmd)
{

/*
 * TODO  add your code here
 *  Call the system() function with the command set in the cmd
 *   and return a boolean true if the system() call completed with success
 *   or false() if it returned a failure
*/
    int status = system(cmd);

    if (status == -1) {
        // Error invoking the system() call
        perror("system");
        return false;
    }

    // Check if the command was executed successfully
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
        // The command completed successfully
        return true;
    } else {
        // The command returned a non-zero exit status
        fprintf(stderr, "Command failed with exit status %d\n", WEXITSTATUS(status));
        return false;
    }
}

/**
* @param count -The numbers of variables passed to the function. The variables are command to execute.
*   followed by arguments to pass to the command
*   Since exec() does not perform path expansion, the command to execute needs
*   to be an absolute path.
* @param ... - A list of 1 or more arguments after the @param count argument.
*   The first is always the full path to the command to execute with execv()
*   The remaining arguments are a list of arguments to pass to the command in execv()
* @return true if the command @param ... with arguments @param arguments were executed successfully
*   using the execv() call, false if an error occurred, either in invocation of the
*   fork, waitpid, or execv() command, or if a non-zero return value was returned
*   by the command issued in @param arguments with the specified arguments.
*/

bool do_exec(int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];

/*
 * TODO:
 *   Execute a system command by calling fork, execv(),
 *   and wait instead of system (see LSP page 161).
 *   Use the command[0] as the full path to the command to execute
 *   (first argument to execv), and use the remaining arguments
 *   as second argument to the execv() command.
 *
*/
    va_end(args);

    // check if the command specified with absolute path 
    if(command != NULL && **command == '/')
    {
        // first create a child process using fork()
        pid_t pid = fork();
        // check if the child process creaed successfully 
        if(pid == -1){
            perror("fork");
            return false;
        }

        // if the child created successfully 
        if(pid == 0)
        {
            // in the child process 
            // execute the command using the execv() 
            execv(command[0], command);
            // If execv fails, print an error message
            perror("execv");
            //exit(EXIT_FAILURE);
            return false;
        }
        else {
            // in parent process 

            // wait for the child process to complete 
            int status;
            waitpid(pid, &status, 0);

            // Check if the child process exited successfully
            return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
        }


        return true;

    }

    return false;

}

/**
* @param outputfile - The full path to the file to write with command output.
*   This file will be closed at completion of the function call.
* All other parameters, see do_exec above
*/
bool do_exec_redirect(const char *outputfile, int count, ...)
{
    va_list args;
    va_start(args, count);
    char * command[count+1];
    int i;
    for(i=0; i<count; i++)
    {
        command[i] = va_arg(args, char *);
    }
    command[count] = NULL;
    // this line is to avoid a compile warning before your implementation is complete
    // and may be removed
    command[count] = command[count];


/*
 * TODO
 *   Call execv, but first using https://stackoverflow.com/a/13784315/1446624 as a refernce,
 *   redirect standard out to a file specified by outputfile.
 *   The rest of the behaviour is same as do_exec()
 *
*/
    va_end(args);

    // check if the command specified with absolute path 
    if(command != NULL && **command == '/')
    {
        // first of all overwrite the output file or create if not exist
        int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644); 

        // check if the file opened successfully 
        if(fd < 0)
        {
            perror("open");
            //exit(EXIT_FAILURE);
            return false;
        }

        else{
            // first create a child process using fork()
            pid_t pid = fork();
            // check if the child process created successfully 
            if(pid == -1){
                // Fork failed
                perror("fork");
                close(fd);
            }

            // if the child created successfully 
            if(pid == 0 && command != NULL && command[0][0] == '/')
            {
                // in the child process 
                // Redirect standard output to the file descriptor
                if (dup2(fd, STDOUT_FILENO) == -1) {
                    perror("dup2");
                    close(fd);
                    //exit(EXIT_FAILURE);
                    return false;
                }

                // Close the original file descriptor
                close(fd);

                // execute the command using the execv() 
                execv(command[0], command);
                // If execv fails, print an error message
                perror("execv");
                //exit(EXIT_FAILURE);
                return false;
            }
            else {
                // in parent process 

                // wait for the child process to complete 
                int status;
                waitpid(pid, &status, 0);

                // Check if the child process exited successfully
                return WIFEXITED(status) && (WEXITSTATUS(status) == 0);
            }

        }

        return true;
    }
    
    return false;
}
