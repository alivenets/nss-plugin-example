#ifndef _USERDB_CLIENT_H
#define _USERDB_CLIENT_H

#include <string.h>

#include <grp.h>
#include <pwd.h>

typedef struct GroupEntry
{
    char *name;
    gid_t gid;
    char **members;
    size_t membersCount;
} GroupEntry;

typedef struct UserEntry
{
    char *name;
    uid_t uid;
    gid_t gid;
} UserEntry;

void free_group_entry(GroupEntry *entry);

void free_user_entry(UserEntry *entry);

char **list_groups(size_t *pCount);

char **list_users(size_t *pCount);

int get_group_by_name(const char *name, GroupEntry *pEntry);

int get_group_by_id(gid_t gid, struct GroupEntry *pEntry);

int get_user_by_name(const char *name, UserEntry *pEntry);

int get_user_by_id(uid_t uid, UserEntry *pEntry);

#endif // _USERDB_CLIENT_H
