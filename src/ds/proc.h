/*
 * Marcel the Shell -- a shell written in C
 * Copyright (C) 2016 Chad Sharp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef M_proc_H
#define M_proc_H

#define ARGV_INIT_SIZE 1024

#include <sys/types.h>
#include <termios.h>
#include "dyn_array.h"

// Struct to model a single command (process)
typedef struct proc {
    dyn_array *argv; // Arguments to be passed to execvp
    dyn_array *env; // Environment variables in the form "VAR=VALUE"
    pid_t pid; // Pid of command
    int fds[3]; // File descriptors for input, output, error
    _Bool completed; // Command has finished executing
    _Bool stopped; // Command has been stopped
    int exit_code; // Status code proc exited with
    struct proc *next; // Pointer to next piped proc
} proc;

proc *new_proc(void);
void free_proc(proc *c);


typedef struct proc_io {
    char *path;
    int oflag;
} proc_io;

typedef struct job {
    struct job *next;
    char *name; // Name of command
    size_t index; // Index in job table
    proc *root; // First command
    proc_io io[3]; // stdin, stdout and stderr
    pid_t pgid; // Proc group ID for job
    _Bool notified; // User has been notified of state change
    _Bool bkg; // Job should execute in background
    struct termios tmodes; // Terminal modes for job
} job;

job *new_job(void);
void free_single_job(job *j);

#endif
