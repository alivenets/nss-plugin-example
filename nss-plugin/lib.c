#include <nss.h>

#include <grp.h>
#include <pwd.h>

#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <client.h>

#include "helpers.h"

typedef struct GetentData
{
    /*
     * Comment from nss-systemd:
     * As explained in NOTES section of getpwent_r(3) as 'getpwent_r() is not really reentrant since it
     * shares the reading position in the stream with all other threads', we need to protect the data in
     * GetentData from multithreaded programs which may call setpwent(), getpwent_r(), or endpwent()
     * simultaneously. So, each function locks the data by using the mutex below.
     */
    pthread_mutex_t mutex;
    size_t array_index;
    char **ext_db_entities;
    size_t entity_count;
} GetentData;

static GetentData getgrent_data = {.mutex = PTHREAD_MUTEX_INITIALIZER};

static char *init_group_struct(struct group *grp, char *buffer, size_t buflen, size_t *outlen)
{
    char *bufPos = buffer;
    int count = 0;

    memset(grp, 0, sizeof(*grp));

    count = sprintf(bufPos, "%s", "*");
    grp->gr_passwd = bufPos;
    bufPos += count + 1;

    if (outlen)
        *outlen = count;

    return bufPos;
}

static char *init_passwd_struct(struct passwd *pwd, char *buffer, size_t buflen, size_t *outlen)
{
    char *bufPos = buffer;
    int count = 0;

    memset(pwd, 0, sizeof(*pwd));

    count = sprintf(bufPos, "%s", "x");
    pwd->pw_passwd = bufPos;
    bufPos += count + 1;

    count = sprintf(bufPos, "%s", "/nonexistent");
    pwd->pw_dir = bufPos;
    bufPos += count + 1;

    count = sprintf(bufPos, "%s", "/bin/false");
    pwd->pw_shell = bufPos;
    bufPos += count + 1;

    if (outlen)
        *outlen = count;

    return bufPos;
}

enum nss_status _nss_example_getpwnam_r(
        const char *name, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    char *bufPos = init_passwd_struct(result, buffer, buflen, NULL);
    UserEntry userEntry;

    int ret = get_user_by_name(name, &userEntry);

    if (ret < 0) {
        return NSS_STATUS_NOTFOUND;
    }

    result->pw_uid = userEntry.uid;
    result->pw_gid = userEntry.gid;

    int count = sprintf(bufPos, "%s", userEntry.name);
    result->pw_name = bufPos;
    bufPos += count + 1;

    free_user_entry(&userEntry);

    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_example_getpwuid_r(uid_t uid, struct passwd *result, char *buffer, size_t buflen, int *errnop)
{
    char *bufPos = init_passwd_struct(result, buffer, buflen, NULL);
    UserEntry userEntry;
    int ret = get_user_by_id(uid, &userEntry);

    if (ret < 0) {
        return NSS_STATUS_NOTFOUND;
    }

    result->pw_uid = userEntry.uid;
    result->pw_gid = userEntry.gid;

    int count = sprintf(bufPos, "%s", userEntry.name);
    result->pw_name = bufPos;
    bufPos += count + 1;

    free_user_entry(&userEntry);

    return NSS_STATUS_SUCCESS;
}

void copy_group_entry(const GroupEntry *entry, struct group *result, char *buffer, size_t buflen)
{
    char *bufPos = buffer;
    int count = 0;

    count = sprintf(bufPos, "%s", entry->name);
    result->gr_name = bufPos;
    bufPos += count + 1;

    result->gr_gid = entry->gid;

    result->gr_mem = (char **)bufPos;
    memset(result->gr_mem, 0, sizeof(char *) * (count + 1));

    bufPos += sizeof(char *) * (count + 1);
    if (entry->membersCount > 0) {
        for (size_t i = 0; i < entry->membersCount; i++) {
            count = sprintf(bufPos, "%s", entry->members[i]);
            result->gr_mem[i] = bufPos;
            bufPos += count + 1;
        }
    }
}

enum nss_status _nss_example_getgrnam_r(
        const char *name, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    char *bufPos = init_group_struct(result, buffer, buflen, NULL);
    GroupEntry entry;

    int ret = get_group_by_name(name, &entry);

    if (ret < 0)
        return NSS_STATUS_NOTFOUND;

    copy_group_entry(&entry, result, bufPos, buflen);
    free_group_entry(&entry);
    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_example_getgrgid_r(gid_t gid, struct group *result, char *buffer, size_t buflen, int *errnop)
{
    char *bufPos = init_group_struct(result, buffer, buflen, NULL);
    GroupEntry entry;
    int ret = get_group_by_id(gid, &entry);

    if (ret < 0)
        return NSS_STATUS_NOTFOUND;

    copy_group_entry(&entry, result, bufPos, buflen);
    free_group_entry(&entry);
    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_example_endgrent(void)
{
    __attribute__((cleanup(pthread_mutex_unlock_assertp))) pthread_mutex_t *_l = NULL;
    _l = pthread_mutex_lock_assert(&getgrent_data.mutex);

    getgrent_data.array_index = (size_t)-1;

    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_example_setgrent(int stayopen)
{
    __attribute__((cleanup(pthread_mutex_unlock_assertp))) pthread_mutex_t *_l = NULL;
    _l = pthread_mutex_lock_assert(&getgrent_data.mutex);

    size_t count = 0;
    char **groups = list_groups(&count);

    getgrent_data.ext_db_entities = groups;
    getgrent_data.entity_count = count;
    getgrent_data.array_index = 0;

    return NSS_STATUS_SUCCESS;
}

enum nss_status _nss_example_getgrent_r(struct group *result, char *buffer, size_t buflen, int *errnop)
{
    int r;

    assert(result);
    assert(errnop);

    char *bufPos = init_group_struct(result, buffer, buflen, NULL);

    __attribute__((cleanup(pthread_mutex_unlock_assertp))) pthread_mutex_t *_l = NULL;
    _l = pthread_mutex_lock_assert(&getgrent_data.mutex);

    if (!getgrent_data.ext_db_entities)
        return NSS_STATUS_NOTFOUND;

    if (!getgrent_data.ext_db_entities[getgrent_data.array_index])
        return NSS_STATUS_NOTFOUND;

    GroupEntry entry;
    int ret = get_group_by_name(getgrent_data.ext_db_entities[getgrent_data.array_index], &entry);

    if (ret < 0)
        return NSS_STATUS_UNAVAIL;

    copy_group_entry(&entry, result, bufPos, buflen);
    free_group_entry(&entry);
    getgrent_data.array_index++;

    return NSS_STATUS_SUCCESS;
}
