/*
    shell208.c
    Joshua Song, 22 February 2022

*/

#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#define MAX_COMMAND_LINE_LENGTH 100

void initialize();
void execute_command(char *command_line);
void cleanup();

/**
 * @brief display set of features for this shell
 */
void help()
{
    printf("\n******************************************\n\n");
    printf("A Simple Command Shell\n\n");
    printf("Type \"help\" to see the list of features of this shell.\n\n");
    printf("List of commands supported\n");
    printf("\t- ls\n");
    printf("\t- exit\n");
    printf("\t- all other commands executable through execvp()\n");
    printf("\t- Single pipe and redirection are also supported!\n");
    printf("\nHave Fun!!!\n\n");
    printf("******************************************\n");
}

/**
 * @brief calculate the length of a string
 *
 * @param input_string string to count length
 * @return int that represents the length of the string
 */
int my_strlen(char *input_string)
{
    int i;
    for (i = 0; input_string[i] != '\0'; i++)
        ;
    return i;
}

/**
 * @brief Get the length args object
 *
 * @param command_line command line that was put in
 * @return int that represents the number of commands
 */
int get_length_args(char *command_line)
{
    int original_len = my_strlen(command_line);
    char *copied = malloc(original_len);

    strcpy(copied, command_line);

    int length = 0;

    char *command = strtok(copied, " ");

    while (command != NULL)
    {
        length++;
        command = strtok(NULL, " ");
    }

    free(copied);

    return length;
}

/**
 * @brief executes all the commands
 *
 * @param argv commands to run
 * @param run_background if command is run in background
 * @return int that represents the result of function
 */
int exec(char *argv[], int run_background)
{
    pid_t pid;
    int status;

    // Can use pid to check reaped child pid & Collect Zombie
    pid = waitpid(-1, &status, WNOHANG);

    pid = fork();

    if (pid < 0)
    {
        perror("Trouble forking");
        exit(1);
    }

    if (pid != 0)
    {
        fflush(stdout);
        /* Parent */
        if (run_background == 0)
        {
            pid = waitpid(pid, &status, 0);
        }

        fflush(stdout);
    }
    else
    {
        /* Child */
        fflush(stdout);
        execvp(argv[0], argv);
        perror("exec failed");
        fflush(stdout);
    }

    return 0;
}

/**
 * @brief executes commands with pipe
 *
 * @param argv1 first command that is at the left of the pipe
 * @param argv2 second command that is at the right of the pipe
 * @param run_background if command is run in background
 * @return int that represents the result of function
 */
int exec_pipe(char **argv1, char **argv2, int run_background)
{

    int fd[2];
    int status;

    if (pipe(fd) < 0)
    {
        perror("Trouble creating pipe");
        exit(1);
    }

    int pid1;
    // Can use pid to check reaped child pid & Collect Zombie
    pid1 = waitpid(-1, &status, WNOHANG);

    pid1 = fork();

    if (pid1 < 0)
    {
        perror("Trouble forking");
        exit(1);
    }

    if (pid1 == 0) // child process
    {
        if (dup2(fd[1], STDOUT_FILENO) == -1)
        {
            perror("Trouble redirecting stdout");
        }
        close(fd[0]);
        close(fd[1]);
        execvp(argv1[0], argv1);
        perror("execvp in first command failed");
        exit(1);
    }
    else // parent process
    {
        int pid = fork();
        if (pid < 0)
        {
            perror("Trouble forking");
            exit(1);
        }

        if (pid == 0) // 2nd child process
        {
            close(fd[1]);
            if (dup2(fd[0], STDIN_FILENO) == -1)
            {
                perror("Trouble redirecting stdin");
            }
            close(fd[0]);

            execvp(argv2[0], argv2);
            perror("execvp in second command failed");
            exit(1);
        }
        else // 2nd parent process
        {
            close(fd[0]);
            close(fd[1]);
            if (run_background == 0)
            {
                waitpid(pid1, &status, 0);
                waitpid(pid, &status, 0);
            }
        }
    }

    return 0;
}

/**
 * @brief executes commands redirecting command's stdout to a file
 *
 * @param commands commands to execute
 * @param fileName file to receive redirection
 * @param run_background if command is run in background
 * @return int that represents the result of function
 */
int redirect_command_stdout(char **commands, char **fileName, int run_background)
{

    pid_t pid;
    int status;

    // Can use pid to check reaped child pid & Collect Zombie
    pid = waitpid(-1, &status, WNOHANG);

    pid = fork();

    if (pid != 0)
    {
        fflush(stdout);

        if (run_background == 0)
        {
            pid = waitpid(pid, &status, 0);
        }

        fflush(stdout);
    }
    else
    {
        /* Child */
        fflush(stdout);

        const char *file_name = fileName[0];
        int fd = open(file_name, O_RDWR | O_CREAT | O_TRUNC, 0644);
        if (fd < 0)
        {
            perror("Trouble opening file");
            exit(1);
        }

        if (dup2(fd, STDOUT_FILENO) < 0)
        {
            perror("Trouble dup2-ing to stdout");
            close(fd); // Clean up open things before you go
            exit(1);
        }

        close(fd);

        execvp(commands[0], commands);

        fflush(stdout);
    }

    return 0;
}

/**
 * @brief executes commands redirecting command's stdin from a file
 *
 * @param commands commands to execute
 * @param fileName file to give redirection
 * @param run_background if command is run in background
 * @return int that represents the result of function
 */
int redirect_command_stdin(char **commands, char **fileName, int run_background)
{

    pid_t pid;
    int status;

    // Can use pid to check reaped child pid & Collect Zombie
    pid = waitpid(-1, &status, WNOHANG);

    pid = fork();

    if (pid != 0)
    {

        fflush(stdout);

        if (run_background == 0)
        {
            pid = waitpid(pid, &status, 0);
        }
        fflush(stdout);
    }
    else
    {
        /* Child */

        // open a file for read only
        const char *file_name = fileName[0];
        int fd = open(file_name, O_RDONLY);
        if (fd < 0)
        {
            perror("Trouble opening file");
            exit(1);
        }

        if (dup2(fd, STDIN_FILENO) < 0)
        {
            perror("Trouble dup2-ing to stdout");
            close(fd);
            exit(1);
        }

        close(fd);

        execvp(commands[0], commands);

        fflush(stdout);
    }

    return 0;
}

/**
 * @brief
 *
 * @param command_line the command line received from terminal
 */
void execute_command(char *command_line)
{

    // Length of string
    int strLen = strlen(command_line);

    int k, x;
    int run_background = 0;
    int isPipe = 0;
    int redirect_stdout = 0;
    int redirect_stdin = 0;

    // Check ampersand for background running
    if (command_line[strLen - 1] == 38)
    {
        // run in background
        run_background = 1;
        // remove & from command line
        command_line[--strLen] = '\0';
    }

    // Check pipe or redirection and save k as the index for special character
    for (k = 0; k < strLen - 1; k++)
    {
        // ACSII for |
        if (command_line[k] == 124)
        {
            isPipe = 1;
            break;
        }
        // ACSII for >
        if (command_line[k] == 62)
        {
            redirect_stdout = 1;
            break;
        }
        // ACSII for <
        if (command_line[k] == 60)
        {
            redirect_stdin = 1;
            break;
        }
    }

    // If the command contains pipe or redirection
    if (isPipe || redirect_stdin || redirect_stdout)
    {
        // Separate commands by pipe or redirection operator
        // Consider no space before or after |, < or > operator
        char *firstCommand = malloc(k + 1);
        char *secondCommand = malloc(strLen - k);

        char *argv1[MAX_COMMAND_LINE_LENGTH];
        char *argv2[MAX_COMMAND_LINE_LENGTH];

        for (x = 0; x < k; x++)
        {
            firstCommand[x] = command_line[x];
        }
        firstCommand[x] = '\0';

        int z = 0;
        for (x = k + 1; x < strLen; x++)
        {
            secondCommand[z] = command_line[x];
            z++;
        }
        secondCommand[z] = '\0';

        // Parse first command line
        char *token1 = strtok(firstCommand, " ");
        int i = 0;
        while (token1 != NULL)
        {
            argv1[i] = token1;
            token1 = strtok(NULL, " ");
            i++;
        }
        argv1[i] = NULL;

        // Parse second command line
        char *token2 = strtok(secondCommand, " ");
        int l = 0;
        while (token2 != NULL)
        {
            argv2[l] = token2;
            token2 = strtok(NULL, " ");
            l++;
        }
        argv2[l] = NULL;

        if (isPipe == 1)
        {
            exec_pipe(argv1, argv2, run_background);
            free(firstCommand);
            free(secondCommand);
        }
        else if (redirect_stdout == 1)
        {
            redirect_command_stdout(argv1, argv2, run_background);
            free(firstCommand);
            free(secondCommand);
        }
        else
        {
            redirect_command_stdin(argv1, argv2, run_background);
            free(firstCommand);
            free(secondCommand);
        }
    }
    else
    {
        // Length of Arguments
        int my_argc = get_length_args(command_line) + 1;

        char *args[MAX_COMMAND_LINE_LENGTH];

        // Parse command line
        char *token = strtok(command_line, " ");

        int i = 0;

        while (token != NULL)
        {
            args[i] = token;
            token = strtok(NULL, " ");
            i++;
        }
        args[i] = NULL;

        // Check if it is help command
        if (my_argc == 2 && strcmp(args[0], "help") == 0)
        {
            help();
        }
        else
        {
            exec(args, run_background);
        }
    }
}

void initialize()
{
}

void cleanup()
{
}

int main()
{
    initialize();

    // The main infinite loop
    char command_line[MAX_COMMAND_LINE_LENGTH];
    while (1)
    {
        printf("Shell By SeaShore$ ");
        if (fgets(command_line, MAX_COMMAND_LINE_LENGTH, stdin) != NULL)
        {
            // Remove trailing \n
            command_line[strcspn(command_line, "\n")] = 0;
            if (strcmp(command_line, "exit") == 0)
            {
                break;
            }
            execute_command(command_line);
        }
        else
        {
            printf("Something went wrong. Try again, I guess.\n");
        }
    }

    cleanup();

    return 0;
}