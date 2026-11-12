/**
 * @file tkl_cmd.c
 * @brief the implements of tuya os execute and script run
 * @version 0.1
 * @date 2019-08-15
 *
 * @copyright Copyright 2020-2021 Tuya Inc. All Rights Reserved.
 *
 */

#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "hal_cmd.h"

#define HAL_CMD_MAX 1024

int hal_cmd_run(const char *cmd, char *buffer,int *len)
{
    int pfd[2];
    char *c;
    FILE    *fp;
    pid_t   pid;
    int stat;
    int flag = 0;
    char *node = NULL;
    int i = 0;
    char *argv[32];
    int iret = -1;

    char cmd_buf[HAL_CMD_MAX] = {0};

    if(cmd == NULL) {
        fprintf(stderr,"invalid param\n"); 
        return -1;
    }

    if(strlen(cmd) <= 0 || strlen(cmd) >= HAL_CMD_MAX) {
        fprintf(stderr,"cmd is not valid\n");
        return -1;
    }

    strcpy(cmd_buf,cmd);

    node = strtok((char*)cmd_buf," ");

    if(node == NULL ||  strrchr(node,'/') == NULL) {
        fprintf(stderr,"run cmd need path\n");
        return -1;
    }

    if(access(node,F_OK|X_OK)) {
        fprintf(stderr,"%s is not valid cmd\n",node);
        return -1;
    }

    while(node != NULL && i< 32) {
//      printf("param:%s\n",node);
	argv[i] = node;
	i++;
        node = strtok(NULL," ");
    }

    argv[i] = NULL;


    if(buffer != NULL &&  len != NULL && *len  > 0) {
        flag = 1;
    }

    if(flag) {
        if (pipe(pfd) < 0) {
            fprintf(stderr,"pipe open fail\n");
            return -1;
        }
    }

    if ((pid = vfork()) < 0) {
        fprintf(stderr,"fork fail\n");

	if(flag) {
            close(pfd[0]);
            close(pfd[1]);
	}
        return  -1;   /* errno set by fork() */
    }
    
    if (pid == 0) {/* child */

        if(flag) {
            close(pfd[0]);
            if (pfd[1] != STDOUT_FILENO) {
                dup2(pfd[1], STDOUT_FILENO);
                close(pfd[1]);
            }
        }

        //execvp(argv[0],argv);
        execv(argv[0],argv);

        _exit(127);
    }

    if(flag) {
	int i;

        close(pfd[1]);
        if ( (fp = fdopen(pfd[0],"r")) == NULL) {
            close(pfd[0]);
	    return -1;
        }

        memset(buffer, 0x0, *len );
        read( pfd[0], buffer, *len);


        buffer[*len - 1] = '\0';
        *len = strlen(buffer) + 1;
	close(pfd[0]);
    }

    while (waitpid(pid, &stat, 0) < 0) {
        if (errno != EINTR) {
            return -1;
	 }
    }

    if (WIFEXITED(stat)) {
        if(WEXITSTATUS(stat) == 0) {
            iret = 0;
        }
    } 

    return iret;
}
