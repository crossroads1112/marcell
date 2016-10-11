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

#include <stdio.h> // readline
#include <stdlib.h> // calloc, getenv
#include <string.h> // strerror, strcmp

#include <unistd.h> // getcwd

#include <readline/readline.h> // readline, rl_complete
#include <readline/history.h> // add_history

#include "ds/proc.h" // proc, job etc.
#include "execute.h" // launch_job, initialize_builtins
#include "jobs.h" // initialize_job_control, do_job_notification
#include "lexer.h" // YY_BUFFER_STATE, yy_delete_buffer, yy_scan_string
#include "macros.h" // Stopif, Free
#include "parser.h" // yyparse
#include "signals.h" // initialize_signal_handling, sig_flags...

#define MAX_PROMPT_LEN 1024
int exit_code;

static void prepare_for_processing(void);
static void gen_prompt(char *buf);
static char *get_input(void);

// This has to ba a macro because sigsetjmp is picky about the its stack frame
// it returns into
// Turns on asynchronous handling for SIGCHLD and is the destination for the
// sigsetjmp calls in the handler. When returning from longjmp, ensures
// readline's temporary variables are cleaned up. Additionally, informs signal
// handler that it should longjmp out
#define prepare_for_input()                                             \
    do {                                                                \
        sig_handle(SIGCHLD);                                            \
        /* siglongjmp from signal handler returns here */               \
        while (sigsetjmp(sigbuf, 1)) {                                  \
            /*Cleanup readline in order to get new prompt*/             \
            /*rl_free_line_state();                               */        \
            /*rl_cleanup_after_signal();                          */        \
            /*RL_UNSETSTATE( RL_STATE_ISEARCH                     */        \
            /*             | RL_STATE_NSEARCH                     */        \
            /*             | RL_STATE_VIMOTION                    */        \
            /*             | RL_STATE_NUMERICARG                  */        \
            /*             | RL_STATE_MULTIKEY );                 */        \
            /*rl_line_buffer[rl_point = rl_end = rl_mark = 0] = 0;*/        \
            int c;  \
            while ((c = getchar()) != EOF) printf("%c",c);              \
            printf("\n");                                               \
        }                                                               \
        run_queued_signals();                                           \
        sig_flags |= WAITING_FOR_INPUT;                                 \
    } while (0)


int main(void)
{

    Stopif(!initialize_builtins(), return M_FAILED_INIT,
           "Could not initialize builtin commands");
    Stopif(!initialize_job_control(), return M_FAILED_INIT,
           "Could not initialize job control");
    initialize_signal_handling();

    // Use tab for shell completion
    rl_bind_key('\t', rl_complete);
    rl_set_signals();

    // buffer for stdin
    char *buf = NULL;

    prepare_for_input();
    while ((buf = get_input())) {
        prepare_for_processing();

        job *j = new_job();
        j->name = malloc((strlen(buf) + 1) * sizeof *j->name);
        Assert_alloc(j->name);
        strcpy(j->name, buf);

        add_history(buf);

        YY_BUFFER_STATE b = yy_scan_string(buf);
        if ((yyparse(j) == 0) && *((proc**) j->procs.data)) {
            register_job(j);
            launch_job(j);
        } else {
            Cleanup(j, free_single_job);
        }

        Cleanup(b, yy_delete_buffer);
        Free(buf);

        exit_code = do_job_notification();
        prepare_for_input();
    }
    return exit_code;
}


// For symmetry with prepare_for_input. Turns off async handling for SIGCHLD
// and informs signal handler that it shouldn't longjmp out
static void prepare_for_processing(void)
{
    sig_flags &= ~WAITING_FOR_INPUT;
    sig_default(SIGCHLD);
}


// Prints prompt and returns line entered by user. Returned string must be
// freed. Returns NULL on EOF
static char *get_input(void)
{
    char prompt_buf[MAX_PROMPT_LEN] = {0};
    gen_prompt(prompt_buf);
    return readline(prompt_buf);
}

// Creates shell prompt based on username and current directory
static void gen_prompt(char *buf)
{
    char *user = getenv("USER");
    char *dir = getcwd(NULL, 1024);
    char sym = (strcmp(user, "root")) ? '$' : '#';
    snprintf(buf, MAX_PROMPT_LEN, "%-3d [%s:%s] %c ", (unsigned char) exit_code,
             user, dir, sym);
    Free(dir);
}
