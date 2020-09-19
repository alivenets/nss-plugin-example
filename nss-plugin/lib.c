#include <nss.h>

#include <pwd.h>
#include <grp.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "helpers.h"

typedef struct GetentData {
    /*
     * Comment from nss-systemd:
     * As explained in NOTES section of getpwent_r(3) as 'getpwent_r() is not really reentrant since it
     * shares the reading position in the stream with all other threads', we need to protect the data in
     * GetentData from multithreaded programs which may call setpwent(), getpwent_r(), or endpwent()
     * simultaneously. So, each function locks the data by using the mutex below.
     */
    pthread_mutex_t mutex;
    size_t array_index;
    size_t count;
} GetentData;

static GetentData getgrent_data = {
    .mutex = PTHREAD_MUTEX_INITIALIZER
};

/*
 * The NSS plugin will return list of users with groups from the static configuration
 */
static const char *dynamic_group_membership[]= { "com_example_dynamicuser", NULL };

static const struct group group_array[] = {
    {
        .gr_name = (char*) "com_example_dynamicuser",
        .gr_gid = 100000,
        .gr_passwd = (char*) "*", /* locked */
        .gr_mem = (char*[]) { NULL },
    },
    {
        .gr_name = (char*) "service-client",
        .gr_gid = 1001,
        .gr_passwd = (char*) "x", /* locked */
        .gr_mem = (char*[]) { NULL },
    },
};

static const struct passwd user_array[] = {
    {
        .pw_uid = 100000,
        .pw_gid = 100000,
        .pw_name = "com_example_dynamicuser",
        .pw_passwd = "x", /* locked */
        .pw_gecos = (char*) "com.example.dynamicuser",
        .pw_dir = (char*) "/nonexistent",
        .pw_shell = (char*) "/bin/false",
    },
};

static bool file_exists(char *filename)
{
    FILE *file = NULL;

    if ((file = fopen(filename, "r")) == NULL) {
        return false;
    } else {
      fclose(file);
    }
    return true;
}

static int copy_to_passwd(const struct passwd *user_info, struct passwd *result, char *buffer, size_t buf_len)
{
    *result = *user_info;
    return 0;
}

static int copy_to_group(const struct group *group_info, struct group *result, char *buffer, size_t buf_len)
{
    *result = *group_info;

    if (file_exists("/tmp/enable-dynamic-group") && strcmp(result->gr_name, "service-client") == 0) {
        result->gr_mem = (char**)dynamic_group_membership;
    }

    return 0;
}

enum nss_status _nss_example_getpwnam_r(const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    memset(result, 0, sizeof(*result));

    for (size_t i = 0; i < countof(user_array); ++i) {
        const struct passwd *user_info = &user_array[i];
        if (strcmp(name, user_info->pw_name) == 0) {
            int ret = copy_to_passwd(user_info, result, buffer, buflen);

            if (ret == 0)
                return NSS_STATUS_SUCCESS;
            else {
                if (errnop && ret != 0)
                    *errnop = ret;
                return NSS_STATUS_TRYAGAIN;
            }
        }
    }

    return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_example_getpwuid_r(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    for (size_t i = 0; i < countof(user_array); ++i) {
        const struct passwd *user_info = &user_array[i];

        if (uid == user_info->pw_uid) {
            if (errnop)
                *errnop = copy_to_passwd(user_info, result, buffer, buflen);
            return NSS_STATUS_SUCCESS;
        }
    }

    return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_example_getgrnam_r(const char *name, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    memset(result, 0, sizeof(*result));

    for (size_t i = 0; i < countof(group_array); ++i) {
        const struct group *group_info = &group_array[i];
        if (strcmp(name, group_info->gr_name) == 0) {
            int ret = copy_to_group(group_info, result, buffer, buflen);
            if (ret == 0)
                return NSS_STATUS_SUCCESS;
            else {
                if (errnop && ret != 0)
                    *errnop = ret;
                return NSS_STATUS_TRYAGAIN;
            }
        }
    }

    return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_example_getgrgid_r(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    for (size_t i = 0; i < countof(group_array); ++i) {
        const struct group *group_info = &group_array[i];
        if (gid == group_info->gr_gid) {
            if (errnop)
                *errnop = copy_to_group(group_info, result, buffer, buflen);
            return NSS_STATUS_SUCCESS;
        }
    }

    return NSS_STATUS_NOTFOUND;
}

enum nss_status _nss_example_endgrent(void)
{
    GetentData *p = &getgrent_data;

    assert(p);

    __attribute__((cleanup(pthread_mutex_unlock_assertp))) pthread_mutex_t *_l = NULL;
    _l = pthread_mutex_lock_assert(&p->mutex);

    p->array_index = (size_t)-1;
    p->count = 0;

    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_example_setgrent(int stayopen)
{
    __attribute__((cleanup(pthread_mutex_unlock_assertp))) pthread_mutex_t *_l = NULL;

    _l = pthread_mutex_lock_assert(&getgrent_data.mutex);

    int r;

    getgrent_data.array_index = 0;

    return NSS_STATUS_SUCCESS;
}


enum nss_status _nss_example_getgrent_r(struct group *result, char *buffer, size_t buflen, int *errnop)
{
    int r;

    assert(result);
    assert(errnop);

    __attribute__((cleanup(pthread_mutex_unlock_assertp))) pthread_mutex_t *_l = NULL;

    _l = pthread_mutex_lock_assert(&getgrent_data.mutex);

    if (getgrent_data.array_index == countof(group_array)) {
        return NSS_STATUS_NOTFOUND;
    }

    const struct group *group_info = &group_array[getgrent_data.array_index];
    *errnop = copy_to_group(group_info, result, buffer, buflen);
    getgrent_data.array_index++;

    return NSS_STATUS_SUCCESS;
}
