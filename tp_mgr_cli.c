#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "worker_pool.h"

#define TOKENS_BUF_SZ 128
#define TOKENS_DELIM " \t\r\n\a"

static int tm_help(int argc, char **argv);
static int tm_list_task(int argc, char **argv);
static int tm_add_task(int argc, char **argv);
static int tm_stop_task(int argc, char **argv);
static int tm_pause_task(int argc, char **argv);
static int tm_resume_task(int argc, char **argv);

const static struct cmd_list_ {
	const char *cmd_name;
        int (*cmd_fn) (int argc, char **argv);
} cmd_list[] = {
	{ "help", tm_help},
        { "add", tm_add_task},
        { "stop", tm_stop_task},
        { "pause", tm_pause_task},
        { "resume", tm_resume_task},
	{ "list", tm_list_task},
	{  NULL , NULL}
};

//global worker pool
worker_pool_t wpool;

/*
 *****************************************
                CLI routines
 ******************************************
 */
#define JOB_NAME_MAX 32

//JOB which will be submited to workers
typedef struct counter_job_ {
        char name[JOB_NAME_MAX];
        int cur_count;
        int max_count;
        FILE *fp;;
} counter_job_t;

void counter_job(void *pvt_data)
{
        counter_job_t *job = (counter_job_t*)pvt_data;

        //open job file
        job->fp = fopen(job->name, "w+");

	fprintf(stdout, "running JOB......\n");

        //count till max count
        while (job->cur_count < job->max_count) {
                yield_utask();
                job->cur_count++;
                fprintf(job->fp, "count %d\n", job->cur_count);
                fflush(job->fp);
                sleep(1);
        }

	fprintf(stdout, "JOB Done......\n");

        //close job file
        fclose(job->fp);
}

static int tm_help(int argc, char **argv)
{
	fprintf(stdout, "\nThread Poll Manager Interactive CLI\n");
	fprintf(stdout, "Usage: <CMD> <CMD_ARGS>\n");
	fprintf(stdout, "\t help: to print help\n");
	fprintf(stdout, "\t add: To add new task\n");
        fprintf(stdout, "\t pause: pause to running task\n");
        fprintf(stdout, "\t resume: resume a paused task\n");
	fprintf(stdout, "\t stop: To stop currently running task\n");
	fprintf(stdout, "\t list: list out all the tasks\n");
	return 0;
}

static int tm_list_task(int argc, char **argv)
{
        print_utask_list(&wpool);
	return 0;
}

static int tm_add_task(int argc, char **argv)
{
        counter_job_t *job;

        //add job
        if (argc == 4) {
                job = (counter_job_t*)malloc(sizeof(counter_job_t));
                strncpy(job->name, argv[1], JOB_NAME_MAX);
                job->cur_count = 0;
                job->max_count = rand() % 60;
                add_utask(&wpool, atoi(argv[2]), atoi(argv[3]), counter_job, job);

        } else {
                fprintf(stderr, "\n[ERROR] Invalid no of argumets given!\n");
                fprintf(stderr, "Valid add command format is: add <jobname> <priority> <wtime_sec>\n\n");
                return -1;
        }

	return 0;
}

static int tm_stop_task(int argc, char **argv)
{
        //stop task_id
        if (argc == 2) {
                stop_utask(&wpool, atoi(argv[1]));
        } else {
                fprintf(stderr, "\n[ERROR] Invalid no of argumets given!\n");
                fprintf(stderr, "Valid stop command format is: stop task_id\n\n");
                return -1;
        }
	return 0;
}

static int tm_pause_task(int argc, char **argv)
{
        //pause task_id
        if (argc == 2) {
                pause_utask(&wpool, atoi(argv[1]));
        } else {
                fprintf(stderr, "\n[ERROR] Invalid no of argumets given!\n");
                fprintf(stderr, "Valid pause command format is: pause task_id\n\n");
                return -1;
        }
	return 0;
}

static int tm_resume_task(int argc, char **argv)
{
        //resume task_id
        if (argc == 2) {
                resume_utask(&wpool, atoi(argv[1]));
        } else {
                fprintf(stderr, "\n[ERROR] Invalid no of argumets given!\n");
                fprintf(stderr, "Valid resume command format is: resume task_id\n\n");
                return -1;
        }
	return 0;
}

/*
 *************************************************
                Thread pool manager CLI shell
 *************************************************
 */
static char *read_line()
{
	char *line = NULL;
	size_t buf_sz = 0;

	//read a entire line from stdin stream
	getline(&line, &buf_sz, stdin);
	return line;
}

char **tokenize_line(char *line, int *n_tokens)
{
        int buf_sz = TOKENS_BUF_SZ, pos = 0;
        char **tokens, **tokens_orig;
        char *token;

        tokens = malloc(buf_sz * sizeof(char*));
        if (!tokens) {
                fprintf(stderr, "[ERROR] Failed to allocate memory for token\n");
                return NULL;
        }

        token = strtok(line, TOKENS_DELIM);
        while (token != NULL) {
                tokens[pos] = token;
                pos++;

                if (pos >= buf_sz) {
                        buf_sz += TOKENS_BUF_SZ;
                        tokens_orig = tokens;
                        tokens = realloc(tokens, buf_sz * sizeof(char*));
                        if (!tokens) {
                                fprintf(stderr, "[ERROR] Failed to allocate \
                                                memory for token\n");
                                free(tokens_orig);
                        }
                }
                token = strtok(NULL, TOKENS_DELIM);
        }
        tokens[pos] = NULL;
        *n_tokens = pos;
        return tokens;
}

static int execute(int argc, char **argv)
{
        int i, valid_cmd = 0;
	if (argv[0] == NULL) {
		return 1;
        }

        for (i = 0; (cmd_list[i].cmd_name != NULL); i++) {
                if (!strncmp(cmd_list[i].cmd_name, argv[0],
                            strlen(cmd_list[i].cmd_name))) {
                        valid_cmd = 1;
                        cmd_list[i].cmd_fn(argc, argv);
                        return 0;
                }
        }

        if (!valid_cmd) {
                fprintf(stderr, "[ERROR] invalid command '%s' given!\n", argv[0]);
                tm_help(argc, argv);
        }

        return 0;
}

static void loop_forever(void)
{
	char *line;
	int should_exit = 0;
	char **args;
        int argc;
        time_t t;

        srand((unsigned) time(&t));

        //initialize the worker pool
        worker_pool_init(&wpool, 2);

	do {
		printf("tm> ");
		line = read_line();
		args = tokenize_line(line, &argc);
		execute(argc, args);
	} while(!should_exit);

        //worker pool cleanup
        worker_pool_term(&wpool);
}

int main()
{
	fprintf(stdout, "\n\nThread Poll Manager Interactive CLI\n");
	fprintf(stdout, "Usage: Please type 'help' for help or 'quit' for exit\n\n");

	loop_forever();

	return 0;
}
