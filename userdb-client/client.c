#include "client.h"

#include <stdio.h>

#include <gio/gio.h>
#include <glib-object.h>
#include <glib.h>

#define DEFAULT_USERDB_SERVICE_PATH "unix:path=/tmp/user-db.sock"

static GVariant *call_dbus(const char *methodName, GVariant *methodArgs)
{
    GDBusConnection *connection = NULL;
    GDBusProxy *proxy = NULL;
    GVariant *response = NULL;
    GError *error = NULL;

    char **groups = NULL;

    connection = g_dbus_connection_new_for_address_sync(
            DEFAULT_USERDB_SERVICE_PATH, G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT, NULL, NULL, &error);

    if (error) {
        fprintf(stderr, "Failed to connect to UserDB: %s\n", error->message);
        g_error_free(error);
        goto finish0;
    }

    proxy = g_dbus_proxy_new_sync(connection, 0, NULL, NULL, "/com/example/UserDb", "com.example.UserDb", NULL, &error);
    if (error) {
        fprintf(stderr, "Failed to create proxy: %s\n", error->message);
        g_error_free(error);
        goto finish1;
    }

    response = g_dbus_proxy_call_sync(proxy, methodName, methodArgs, G_DBUS_CALL_FLAGS_NONE, -1, NULL, &error);

    if (error) {
        fprintf(stderr, "Failed to issue method call %s: %s\n", methodName, error->message);
        g_error_free(error);
        goto finish2;
    }

finish2:
    g_object_unref(proxy);
finish1:
    g_object_unref(connection);
finish0:
    return response;
}

void free_group_entry(struct GroupEntry *entry)
{
    if (entry->name) {
        free(entry->name);
        entry->name = NULL;
    }

    if (entry->members) {
        char **s = &entry->members[0];

        while (*s != NULL) {
            free(*s);
            s++;
        }
    }
}

void free_user_entry(UserEntry *entry)
{
    if (entry->name) {
        free(entry->name);
        entry->name = NULL;
    }
}
char **list_groups(size_t *const pCount)
{
    char **groups = NULL;

    GVariant *response = call_dbus("ListGroups", NULL);
    if (!response) {
        fprintf(stderr, "Failed to get response\n");
        goto finish2;
    }

    GVariant *tupleElem = g_variant_get_child_value(response, 0);
    if (!tupleElem) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    size_t count = 0;
    /* Parse the response message */
    const gchar **constGroups = g_variant_get_strv(tupleElem, &count);

    if (constGroups == NULL) {
        fprintf(stderr, "Failed to get string array\n");
        if (pCount)
            *pCount = 0;
        goto finish4;
    }
    else {
        groups = malloc(sizeof(char *) * (count + 1));
        memset(groups, 0, sizeof(char *) * (count + 1));

        for (size_t i = 0; i < count; ++i) {
            groups[i] = strdup(constGroups[i]);
        }

        groups[count] = NULL;
        if (pCount)
            *pCount = count;
    }

finish4:
    g_variant_unref(tupleElem);
finish3:
    g_variant_unref(response);
finish2:
    return groups;
}

char **list_users(size_t *const pCount)
{
    char **users = NULL;

    GVariant *response = call_dbus("ListUsers", NULL);
    if (!response) {
        fprintf(stderr, "Failed to get response\n");
        goto finish2;
    }
    GVariant *tupleElem = g_variant_get_child_value(response, 0);
    if (!tupleElem) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    size_t count = 0;
    /* Parse the response message */
    const gchar **constUsers = g_variant_get_strv(tupleElem, &count);

    if (constUsers == NULL) {
        fprintf(stderr, "Failed to get string array\n");
        if (pCount)
            *pCount = 0;
        goto finish4;
    }
    else {
        users = malloc(sizeof(char *) * (count + 1));
        memset(users, 0, sizeof(char *) * (count + 1));

        for (size_t i = 0; i < count; ++i) {
            users[i] = strdup(constUsers[i]);
        }

        users[count] = NULL;
        if (pCount)
            *pCount = count;
    }

finish4:
    g_variant_unref(tupleElem);
finish3:
    g_variant_unref(response);
finish2:
    return users;
}

static void get_gvariant_group_members(GVariant *membersVariant, struct GroupEntry *pEntry)
{
    size_t count = 0;
    /* Parse the response message */
    const gchar **constMembers = g_variant_get_strv(membersVariant, &count);

    pEntry->members = malloc(sizeof(char *) * (count + 1));
    memset(pEntry->members, 0, sizeof(char *) * (count + 1));

    for (size_t i = 0; i < count; ++i) {
        pEntry->members[i] = strdup(constMembers[i]);
    }

    pEntry->members[count] = NULL;
    pEntry->membersCount = count;
}

int get_group_by_name(const char *name, struct GroupEntry *pEntry)
{
    int ret = -1;

    GVariant *response = call_dbus("GetGroupByName", g_variant_new("(s)", name));
    if (!response) {
        fprintf(stderr, "Failed to get response\n");
        goto finish4;
    }
    GVariant *gidVariant = g_variant_get_child_value(response, 0);
    if (!gidVariant) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    pEntry->gid = g_variant_get_uint32(gidVariant);
    g_variant_unref(gidVariant);

    GVariant *membersVariant = g_variant_get_child_value(response, 1);
    if (!membersVariant) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    get_gvariant_group_members(membersVariant, pEntry);

    g_variant_unref(membersVariant);

    pEntry->name = strdup(name);

    ret = 0;

finish3:
    g_variant_unref(response);
finish4:
    return ret;
}

int get_group_by_id(gid_t gid, struct GroupEntry *pEntry)
{
    int ret = -1;

    GVariant *response = call_dbus("GetGroupById", g_variant_new("(u)", gid));
    if (!response) {
        fprintf(stderr, "Failed to get response\n");
        goto finish4;
    }
    GVariant *strVariant = g_variant_get_child_value(response, 0);
    if (!strVariant) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    pEntry->name = strdup(g_variant_get_string(strVariant, NULL));
    g_variant_unref(strVariant);

    GVariant *membersVariant = g_variant_get_child_value(response, 1);
    if (!membersVariant) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    get_gvariant_group_members(membersVariant, pEntry);

    g_variant_unref(membersVariant);

    pEntry->gid = gid;

    ret = 0;

finish3:
    g_variant_unref(response);
finish4:
    return ret;
}

int get_user_by_name(const char *name, UserEntry *pEntry)
{
    int ret = -1;

    GVariant *response = call_dbus("GetUserByName", g_variant_new("(s)", name));
    if (!response) {
        fprintf(stderr, "Failed to get response\n");
        goto finish4;
    }
    GVariant *uidVariant = g_variant_get_child_value(response, 0);
    GVariant *gidVariant = g_variant_get_child_value(response, 1);
    if (!uidVariant || !gidVariant) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    pEntry->name = strdup(name);

    pEntry->uid = g_variant_get_uint32(uidVariant);
    g_variant_unref(uidVariant);

    pEntry->gid = g_variant_get_uint32(gidVariant);
    g_variant_unref(gidVariant);

    ret = 0;

finish3:
    g_variant_unref(response);
finish4:
    return ret;
}

int get_user_by_id(uid_t uid, UserEntry *pEntry)
{
    int ret = -1;

    GVariant *response = call_dbus("GetUserById", g_variant_new("(u)", uid));
    if (!response) {
        fprintf(stderr, "Failed to get response\n");
        goto finish4;
    }
    GVariant *strVariant = g_variant_get_child_value(response, 0);
    GVariant *gidVariant = g_variant_get_child_value(response, 1);

    if (!strVariant || !gidVariant) {
        fprintf(stderr, "Failed to get tuple element\n");
        goto finish3;
    }

    pEntry->name = strdup(g_variant_get_string(strVariant, NULL));
    g_variant_unref(strVariant);

    pEntry->uid = uid;

    pEntry->gid = g_variant_get_uint32(gidVariant);
    g_variant_unref(gidVariant);

    ret = 0;

finish3:
    g_variant_unref(response);
finish4:
    return ret;
}
