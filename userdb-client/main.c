#include "client.h"

#include <stdio.h>

int main(void)
{
    printf("[CLIENT] ListGroups\n");
    char **groups = list_groups(NULL);

    char **s;

    if (groups) {
        s = &groups[0];
        while (*s != NULL) {
            printf("[CLIENT] - %s\n", *s);
            struct GroupEntry entry = {};
            get_group_by_name(*s, &entry);
            printf("[CLIENT]   name=%s, gid=%d,members=[", entry.name, entry.gid);
            char **m = &entry.members[0];
            while (*m != NULL) {
                printf("%s,", *m);
                m++;
            }
            printf("]\n");
            s++;
        }
    }

    printf("[CLIENT] ListUsers\n");
    char **users = list_users(NULL);

    if (users) {
        s = &users[0];

        while (*s != NULL) {
            printf("[CLIENT] - %s\n", *s);
            UserEntry userEntry;
            get_user_by_name(*s, &userEntry);
            printf("[CLIENT]   uid=%d,gid=%d\n", userEntry.uid, userEntry.gid);
            s++;
        }
    }
}
