#define _POSIX_C_SOURCE 200809L

#include <basic/basic.h>
#include <basic/mem_arena.h>

#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdatomic.h>
#include <time.h>

// Todo: bulletproof this

typedef void (fn_platform_work_proc)(void *data);

typedef struct {
    fn_platform_work_proc *proc;
    void *data;
} PlatformWork;

typedef struct {
    pthread_mutex_t mutex;
    sem_t sem;
    i32 thread_count;
    pthread_t *tids;
    volatile u64 entries_popped;    // sync'd with mutex/sem
    volatile u64 entries_pushed;    // sync'd with mutex/sem
    volatile u64 entries_completed; // sync'd with __sync_fetch_and_add & __sync_bool_compare_and_swap
    volatile PlatformWork entries[64];
} PlatformWorkQueue;

void platform_work_queue_push(PlatformWorkQueue *queue, fn_platform_work_proc *proc, void *data) {
    u64 entry_index;
    u64 entries_pushed;
    u64 entries_popped;
    u64 diff;
    struct timespec sleep_duration;

    sleep_duration.tv_sec = 0;
    sleep_duration.tv_nsec = 500 * 1000; // 500*1000nanoseconds = 500microseconds = 0.5milliseconds

    for (;;) {
        pthread_mutex_lock(&queue->mutex);

        entries_pushed = queue->entries_pushed;
        entries_popped = queue->entries_popped;
        diff = entries_pushed - entries_popped;
        if (diff < ARRAY_COUNT(queue->entries)) {
            break;
        }

        pthread_mutex_unlock(&queue->mutex);
        nanosleep(&sleep_duration, 0);
    }

    assert(entries_pushed < U64_MAX);
    entry_index = entries_pushed % ARRAY_COUNT(queue->entries);
    queue->entries[entry_index].proc = proc;
    queue->entries[entry_index].data = data;
    queue->entries_pushed++;
    pthread_mutex_unlock(&queue->mutex);
    sem_post(&queue->sem);
}

PlatformWork platform_work_queue_pop(PlatformWorkQueue *queue) {
    PlatformWork work;
    i64 entry_index;

    sem_wait(&queue->sem);
    pthread_mutex_lock(&queue->mutex);

    entry_index = queue->entries_popped++ % ARRAY_COUNT(queue->entries);
    work = queue->entries[entry_index];

    pthread_mutex_unlock(&queue->mutex);

    return work;
}

void *platform_work_runner(void *data) {
    PlatformWorkQueue *queue = data;
    PlatformWork work;

    for (;;) {
        work = platform_work_queue_pop(queue);
        work.proc(work.data);
        __sync_fetch_and_add(&queue->entries_completed, 1);
    }

    return 0;
}

void platform_work_queue_wait_for_completion(PlatformWorkQueue *queue) {
    volatile b32 finished;
    struct timespec ts = {0, 100*1000};

    i64 entries_pushed = queue->entries_pushed; // no more pushes happening, so no sync
    for (;;) {
        finished = __sync_bool_compare_and_swap(&queue->entries_completed, entries_pushed, 0);
        if (finished)
            break;
        else
            nanosleep(&ts, 0);
    }
}

void platform_work_queue_reset(PlatformWorkQueue *queue) {
    queue->entries_popped = 0;
    queue->entries_pushed = 0;
    queue->entries_completed = 0;
}

void platform_work_queue_create(i32 thread_count) {
    i32 i;
    PlatformWorkQueue *queue;

    queue = malloc(sizeof(*queue));

    sem_init(&queue->sem, 0, 0);
    pthread_mutex_init(&queue->mutex, 0);

    queue->tids = malloc(thread_count * sizeof(pthread_t));
    for (i = 0; i < thread_count; i++) {
        if (pthread_create(&queue->tids[i], 0, platform_work_runner, queue) != 0) {
            printf("could not create thread i = %d\n", i);
            break;
        }
    }
    if (!i) {
        printf("could not create a single thread\n");
        exit(0);
    }
    queue->thread_count = i;
    queue->entries_popped = 0;
    queue->entries_pushed = 0;
    queue->entries_completed = 0;
}

