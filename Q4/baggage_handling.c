/*
 * Airport baggage simulation: producer (2s/item), consumer (4s/item),
 * bounded buffer (capacity 5), monitor every 5s.
 * Synchronization: pthread_mutex_t + pthread_cond_t (notempty, notfull).
 *
 * Shared state (belt, indices, count, totals) is accessed only under mtx.
 * Condition variables: producer waits on not_full when belt is full;
 * consumer waits on not_empty when belt is empty. Producer signals not_empty
 * after insert; consumer signals not_full after remove. When loading is
 * finished, producer sets done_loading and broadcasts so a blocked consumer
 * can observe termination.
 */

#define _GNU_SOURCE
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

enum {
	BELT_CAP = 5,
	PROD_MS = 2000,
	CONS_MS = 4000,
	MONITOR_MS = 5000,
	TOTAL_ITEMS = 20
};

static int belt[BELT_CAP];
static int head, tail, count;

static long long total_loaded;
static long long total_dispatched;

static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t not_empty = PTHREAD_COND_INITIALIZER;
static pthread_cond_t not_full = PTHREAD_COND_INITIALIZER;

static volatile bool done_loading;

static void
ms_sleep(int ms)
{
	usleep((useconds_t)ms * 1000);
}

static void *
producer_fn(void *arg)
{
	int id;

	(void)arg;
	for (id = 1; id <= TOTAL_ITEMS; id++) {
		pthread_mutex_lock(&mtx);
		while (count == BELT_CAP)
			pthread_cond_wait(&not_full, &mtx);
		belt[tail] = id;
		tail = (tail + 1) % BELT_CAP;
		count++;
		total_loaded++;
		printf("[Loader] Placed luggage ID %d on belt (belt size=%d)\n", id,
		    count);
		fflush(stdout);
		pthread_cond_signal(&not_empty);
		pthread_mutex_unlock(&mtx);
		ms_sleep(PROD_MS);
	}
	pthread_mutex_lock(&mtx);
	done_loading = true;
	pthread_cond_broadcast(&not_empty);
	pthread_mutex_unlock(&mtx);
	return NULL;
}

static void *
consumer_fn(void *arg)
{
	int item;

	(void)arg;
	for (;;) {
		pthread_mutex_lock(&mtx);
		while (count == 0 && !done_loading)
			pthread_cond_wait(&not_empty, &mtx);
		if (count == 0 && done_loading) {
			pthread_mutex_unlock(&mtx);
			break;
		}
		item = belt[head];
		head = (head + 1) % BELT_CAP;
		count--;
		total_dispatched++;
		printf("[Aircraft] Removed luggage ID %d (belt size=%d)\n", item,
		    count);
		fflush(stdout);
		pthread_cond_signal(&not_full);
		pthread_mutex_unlock(&mtx);
		ms_sleep(CONS_MS);
	}
	return NULL;
}

static void *
monitor_fn(void *arg)
{
	(void)arg;
	for (;;) {
		ms_sleep(MONITOR_MS);
		pthread_mutex_lock(&mtx);
		printf(
		    "[Monitor] loaded=%lld dispatched=%lld belt_size=%d\n",
		    (long long)total_loaded, (long long)total_dispatched, count);
		fflush(stdout);
		if (done_loading && total_dispatched == total_loaded
		    && count == 0) {
			pthread_mutex_unlock(&mtx);
			break;
		}
		pthread_mutex_unlock(&mtx);
	}
	return NULL;
}

int
main(void)
{
	pthread_t prod, cons, mon;

	head = tail = count = 0;
	total_loaded = total_dispatched = 0;
	done_loading = false;

	if (pthread_create(&prod, NULL, producer_fn, NULL) != 0) {
		perror("pthread_create producer");
		return 1;
	}
	if (pthread_create(&cons, NULL, consumer_fn, NULL) != 0) {
		perror("pthread_create consumer");
		return 1;
	}
	if (pthread_create(&mon, NULL, monitor_fn, NULL) != 0) {
		perror("pthread_create monitor");
		return 1;
	}

	pthread_join(prod, NULL);
	pthread_join(cons, NULL);
	pthread_join(mon, NULL);

	printf("Simulation finished: total_loaded=%lld total_dispatched=%lld\n",
	    (long long)total_loaded, (long long)total_dispatched);
	return 0;
}
