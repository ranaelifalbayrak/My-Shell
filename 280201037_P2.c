#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

#define history_size 10
char *history[history_size];
int current_command = 0;
int total_command = 0;
char cmd_line[100];
char *command;
char *args[10];
int background;
int successfull;

void print_cwd()
{
    char *cwd;
    cwd = (char *)malloc(100 * sizeof(char));
    getcwd(cwd, 100);
    printf("%s\n", cwd);
    free(cwd); 
}

void change_directory(char *path)
{
    if (path == NULL)
    {
        path = getenv("HOME");
    }
    if (chdir(path) != 0)
    {
        perror(" ");
    }
    else
    {
        char *new_cwd;
        new_cwd = (char *)malloc(100 * sizeof(char));
        if (getcwd(new_cwd, 100) != NULL)
        {
            setenv("PWD", new_cwd, 1);
            free(new_cwd); 
        }
    }
}

void update_history(char *command)
{
    if (total_command >= history_size)
    {
        free(history[0]);
        for (int i = 1; i < history_size; i++)
        {
            history[i - 1] = history[i];
        }
        history[history_size - 1] = strdup(command);
    }
    else
    {
        history[total_command] = strdup(command);
        total_command++;
    }
}

void show_history()
{
    for (int i = 0; i < total_command; i++)
    {
        printf("[%d] %s\n", i + 1, history[i]);
    }
}

int exec_sys_cmd(int background, char *args[])
{
    int status;
    pid_t pid = fork();
    if (pid == -1)
    {
        perror("fork failed");
        return 1;
    }
    else if (pid == 0)
    {
        execvp(args[0], args);
        perror("execvp failed");
        exit(EXIT_FAILURE);
    }
    else
    {
        if (!background)
        {
            waitpid(pid, &status, 0);
            if (WIFEXITED(status))
            {
                return WEXITSTATUS(status); 
            }
            else
            {
                return 1; 
            }
        }
        else
        {
            printf("Background pid: %d\n", pid);
            return 0;
        }
    }
}

void execute_command_with_pipe(char *first_cmd[], char *second_cmd[])
{
    int fd[2];
    if (pipe(fd) == -1)
    {
        perror("pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 == 0)
    {
        // first child process
        close(fd[0]);
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

        if (execvp(first_cmd[0], first_cmd) == -1)
        {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid2 = fork();
    if (pid2 == 0)
    {
        // second child process
        close(fd[1]);
        dup2(fd[0], STDIN_FILENO);
        close(fd[0]);

        if (execvp(second_cmd[0], second_cmd) == -1)
        {
            perror("execvp failed");
            exit(EXIT_FAILURE);
        }
    }

    // Parent process
    close(fd[0]);
    close(fd[1]);
    waitpid(pid1, NULL, 0);
    waitpid(pid2, NULL, 0);
}

int main()
{
    while (1)
    {
        background = 0;
        printf("myshell>");
        fgets(cmd_line, sizeof(cmd_line), stdin);
        cmd_line[strcspn(cmd_line, "\n")] = 0;

        char cmd_copy[100]; // compy cmd line
        strcpy(cmd_copy, cmd_line);

        command = strtok(cmd_line, " ");
        int position = 0;
        while (command != NULL)
        {
            args[position++] = command;
            command = strtok(NULL, " ");
        }
        args[position] = NULL;

        if (position == 0)
            continue;

        if (strcmp(args[position - 1], "&") == 0)
        {
            background = 1;
            args[--position] = NULL; // remove "&" from args
        }

        if (strcmp(args[0], "pwd") == 0)
        {
            print_cwd();
            update_history(cmd_copy);
        }
        else if (strcmp(args[0], "cd") == 0)
        {
            change_directory(args[1]);
            update_history(cmd_copy);
        }
        else if (strcmp(args[0], "history") == 0)
        {
            update_history(cmd_copy);
            show_history();
        }
        else if (strcmp(args[0], "exit") == 0)
        {
            exit(0);
        }
        else
        {
            if (strstr(cmd_copy, "|"))
            {
                update_history(cmd_copy);
                char *parts[2];
                parts[0] = strtok(cmd_copy, "|");
                parts[1] = strtok(NULL, "");

                char *first_cmd[10], *second_cmd[10];
                int i = 0;
                first_cmd[i] = strtok(parts[0], " ");
                while (first_cmd[i] != NULL)
                {
                    first_cmd[++i] = strtok(NULL, " ");
                }

                i = 0;
                second_cmd[i] = strtok(parts[1], " ");
                while (second_cmd[i] != NULL)
                {
                    second_cmd[++i] = strtok(NULL, " ");
                }

                execute_command_with_pipe(first_cmd, second_cmd);
                continue;
            }
            else if (strstr(cmd_copy, "&&"))
            {
                update_history(cmd_copy);
                char *parts[2];
                parts[0] = strtok(cmd_copy, "&&");
                parts[1] = strtok(NULL, "");
                parts[1] = strtok(parts[1], " ");
                parts[1] = strtok(NULL, "");
                printf("%s", parts[1]);

                char *first_cmd[10], *second_cmd[10];
                int i = 0;
                first_cmd[i] = strtok(parts[0], " ");
                while (first_cmd[i] != NULL)
                {
                    first_cmd[++i] = strtok(NULL, " ");
                }

                if (exec_sys_cmd(background, first_cmd) == 0)
                {
                    i = 0;
                    second_cmd[i] = strtok(parts[1], " ");
                    while (second_cmd[i] != NULL)
                    {
                        second_cmd[++i] = strtok(NULL, " ");
                    }
                    exec_sys_cmd(background, second_cmd);
                }
            }

            else
            {
                exec_sys_cmd(background, args);
                update_history(cmd_copy);
            }
        }
    }
    return 0;
}
