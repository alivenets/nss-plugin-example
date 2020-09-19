#ifndef HELPERS_H_
#define HELPERS_H_

#include <assert.h>
#include <stdio.h>
#include <pthread.h>

#define countof(arr) (sizeof(arr) / sizeof(arr[0]))

static inline pthread_mutex_t* pthread_mutex_lock_assert(pthread_mutex_t *mutex) {
        assert(pthread_mutex_lock(mutex) == 0);
        return mutex;
}

static inline void pthread_mutex_unlock_assertp(pthread_mutex_t **mutexp) {
        if (*mutexp)
                assert(pthread_mutex_unlock(*mutexp) == 0);
}

#endif
