CC = gcc
CFLAGS := $(CFLAGS) -Wall -g
LDFLAGS := $(LDFLAGS) -lpthread
INCLUDES = -I./
SRCS = task.c task_timer.c worker.c worker_pool.c tp_mgr_cli.c
OBJS = $(SRCS:.c=.o)

MAIN = tp_mgr_cli

.PHONY: clean

all: $(MAIN)

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LDFLAGS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

clean:
	$(RM) *.o *~ $(MAIN)

