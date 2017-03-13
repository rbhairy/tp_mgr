Problem Statement
=================

Write a thread pool manager in C/C++.  You submit jobs with priority and it
executes them inside a pool. The thread pool manager has fixed number of
threads (#threads are specified when the pool is started in the beginning).
Each job is represented by a function and each function can take different
arguments.  When you submit the job, specify the time the job should be started
(ie job could be started immediately or could be scheduled at a specific time).
The thread pool manager should have an ability to stop/pause/reschedule
existing jobs to allow higher priority jobs to run via a CLI. Build a score
board to list existing jobs, their priority and state. You could add ability to
change priority of running jobs

Design
------
Please see the .png image

NOTE:
Currently timer is disabled, there is a bug which i am cuurently fixing. which
means all the task will be directly pririoty queue.


How to compile
==============
$ make clean && make

How to run
==========

./tp_mgr_cli

After this, 'tp_mgr_cli' shell would appear, where you run following commands

tm> help

Thread Poll Manager Interactive CLI
Usage: <CMD> <CMD_ARGS>
        help: to print help
        add: To add new task
        pause: pause to running task
        resume: resume a paused task
        stop: To stop currently running task
        list: list out all the tasks



UNIT TESTING
============
Int this unit testing, user task just puting counter value into coresponding
job-file after every second. max_count is assigned randomly to each task, so
that we can perform other operation on such as pause, resume, stop

For more information look at:
        void counter_job(void *pvt_data);





1. TEST1
--------
In this test case, I have added 5 tasks with ascending priority. worker pool
have only 2 workers, the expected job execution should be as follows.
     job1 (priority = 1)
     job2 (priority = 2)
     job5 (priority = 5)
     job4 (priority = 4)
     job3 (priority = 3)

In this test case we have been succesfully, able to pause and resume the task.






TEST1 Output
~~~~~~~~~~~

~/WORK/assignment$ ./tp_mgr_cli

Thread Poll Manager Interactive CLI
Usage: Please type 'help' for help or 'quit' for exit

tm> STEC: poller_thread_fn is running
STEC: worker-0 running
STEC: worker-1 running

tm>
tm>
tm> add job 1 2
tm> STEC: worker-0 got task
running JOB......

tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          RUNNING   [worker-0]
-----------------------
tm> add job2 2 3
tm> STEC: worker-1 got task
running JOB......

tm> add job3 3 4
tm>
tm>
tm> add job4 4 5
tm> add job5 5 6
tm>
tm>
tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          RUNNING   [worker-0]
2    2          RUNNING   [worker-1]
3    3          IDLE
4    4          IDLE
5    5          IDLE
-----------------------
tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          RUNNING   [worker-0]
2    2          RUNNING   [worker-1]
3    3          IDLE
4    4          IDLE
5    5          IDLE
-----------------------
tm> lJOB Done......
STEC: worker-0 got task
running JOB......
ist

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          RUNNING   [worker-1]
3    3          IDLE
4    4          IDLE
5    5          RUNNING   [worker-0]
-----------------------
tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          RUNNING   [worker-1]
3    3          IDLE
4    4          IDLE
5    5          RUNNING   [worker-0]
-----------------------
tm> JOB Done......
STEC: worker-1 got task
running JOB......

tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          COMPLETED
3    3          IDLE
4    4          RUNNING   [worker-1]
5    5          RUNNING   [worker-0]
-----------------------
tm> JOB Done......
STEC: worker-0 got task
running JOB......

tm>
tm>
tm> pause job 3

[ERROR] Invalid no of argumets given!
Valid pause command format is: pause task_id

tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          COMPLETED
3    3          RUNNING   [worker-0]
4    4          RUNNING   [worker-1]
5    5          COMPLETED
-----------------------
tm> pause 3
tm>
tm>
tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          COMPLETED
3    3          PAUSED
4    4          RUNNING   [worker-1]
5    5          COMPLETED
-----------------------
tm> JOB Done......

tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          COMPLETED
3    3          PAUSED
4    4          COMPLETED
5    5          COMPLETED
-----------------------
tm> resume 3
tm> STEC: worker-0 got task
PAUSE TO Running

tm>
tm>
tm>
tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          COMPLETED
3    3          RUNNING   [worker-0]
4    4          COMPLETED
5    5          COMPLETED
-----------------------
tm> JOB Done......

tm> list

Task List
--   --------   -----
id   priority   state
--   --------   -----
1    1          COMPLETED
2    2          COMPLETED
3    3          COMPLETED
4    4          COMPLETED
5    5          COMPLETED
-----------------------
tm> ^C




TEST2 Output
============
