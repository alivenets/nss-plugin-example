#include <algorithm>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <grp.h>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "userdb_common.h"
#include "userdb_stub.h"

struct GroupRecord
{
    std::string name;
    gid_t gid;
};

struct UserRecord
{
    std::string name;
    uid_t uid;
    gid_t gid;
};

// Static configuration of groups to be extended. May be also generated in runtime
// FIXME: get group ID dynamically
static const std::vector<GroupRecord> extendedGroups = {{"service-client", 1001}};

static const std::vector<UserRecord> dynamicUsers = {
        {"com_example_dynamicuser", 100000, 100000},
        {"com_example_dynamicuser2", 100001, 100001},
};

static bool fileExists(const char *filename)
{
    FILE *file = NULL;
    if ((file = fopen(filename, "r")) == NULL) {
        return false;
    }
    else {
        fclose(file);
    }

    return true;
}

class UserDb : public ::com::example::UserDbStub
{
private:
    std::optional<std::tuple<std::string, gid_t, std::vector<std::string>>> getInternalGroup(
            std::string name, gid_t gid)
    {
        auto it = std::find_if(dynamicUsers.begin(), dynamicUsers.end(), [=](const auto &i) {
            if (name.empty())
                return i.gid == gid;
            return i.name == name;
        });
        if (it == dynamicUsers.end()) {
            return std::nullopt;
        }

        return std::make_tuple(it->name, it->gid, std::vector<std::string> {});
    }

    std::optional<std::tuple<std::string, gid_t, std::vector<std::string>>> getGroup(
            std::string name, gid_t gid, MethodInvocation &msg)
    {
        bool b = false;
        auto t = getInternalGroup(name, gid);
        if (t)
            return t.value();

        if (std::find_if(extendedGroups.begin(), extendedGroups.end(), [=](const auto &i) {
                if (name.empty())
                    return i.gid == gid;
                return i.name == name;
            }) == extendedGroups.end()) {
            msg.ret(Gio::DBus::Error(Gio::DBus::Error::Code::FAILED, "Unknown group"));
            return std::nullopt;
        }

        struct group gr = {};
        struct group *grp;
        char buf[2048];
        int ret = -1;

        if (name.empty())
            ret = getgrgid_r(gid, &gr, buf, sizeof(buf), &grp);
        else
            ret = getgrnam_r(name.c_str(), &gr, buf, sizeof(buf), &grp);

        if (ret < 0) {
            msg.ret(Gio::DBus::Error(Gio::DBus::Error::Code::FAILED, "Failed to get group by name"));
            return std::nullopt;
        }

        name = gr.gr_name;
        gid = gr.gr_gid;

        std::vector<std::string> membership;
        for (char **p = &gr.gr_mem[0]; *p != NULL; ++p) {
            membership.push_back(*p);
        }

        // Extend the membership dynamically depending on system state
        if (fileExists("/tmp/enable-dynamic-group") && name == "service-client") {
            membership.push_back("com_example_dynamicuser");
        }

        return std::make_tuple(name, gid, membership);
    }

    std::optional<std::tuple<std::string, uid_t, gid_t>> getUser(std::string name, uid_t uid, MethodInvocation &msg)
    {
        auto it = std::find_if(dynamicUsers.begin(), dynamicUsers.end(), [=](const auto &i) {
            if (name.empty())
                return i.uid == uid;
            return i.name == name;
        });

        if (it == dynamicUsers.end()) {
            msg.ret(Gio::DBus::Error(Gio::DBus::Error::Code::FAILED, "Unknown user"));
            return std::nullopt;
        }

        return std::make_tuple(it->name, it->uid, it->gid);
    }

public:
    void GetGroupByName(const Glib::ustring &name, MethodInvocation &msg) override
    {
        std::cout << "[SERVICE] UserDb::GetGroupByName: name=" << name << std::endl;
        auto o = getGroup(name, 0, msg);
        if (!o) {
            return;
        }
        auto t = o.value();
        for (auto &i : std::get<2>(t)) {
            std::cout << "# " << i << ", ";
        }
        std::cout << std::endl;
        msg.ret(std::get<1>(t), ::com::example::UserDbTypeWrap::stdStringVecToGlibStringVec(std::get<2>(t)));
    }

    void GetGroupById(guint32 gid, MethodInvocation &msg) override
    {
        std::cout << "[SERVICE] UserDb::GetGroupById: gid=" << gid << std::endl;
        auto o = getGroup("", gid, msg);
        if (!o)
            return;
        auto t = o.value();
        for (auto &i : std::get<2>(t)) {
            std::cout << "# " << i << ", ";
        }
        std::cout << std::endl;
        msg.ret(std::get<0>(t), ::com::example::UserDbTypeWrap::stdStringVecToGlibStringVec(std::get<2>(t)));
    }

    void GetUserByName(const Glib::ustring &name, MethodInvocation &msg) override
    {
        std::cout << "[SERVICE] UserDb::GetUserByName: name=" << name << std::endl;
        auto o = getUser(name, 0, msg);
        if (!o)
            return;
        auto t = o.value();
        msg.ret(std::get<1>(t), std::get<2>(t));
    }

    void GetUserById(guint32 uid, MethodInvocation &msg) override
    {
        std::cout << "[SERVICE] UserDb::GetUserById: uid=" << uid << std::endl;
        auto o = getUser("", uid, msg);
        if (!o)
            return;
        auto t = o.value();
        msg.ret(std::get<0>(t), std::get<2>(t));
    }

    void ListGroups(MethodInvocation &msg) override
    {
        std::cout << "[SERVICE] UserDb::ListGroups" << std::endl;
        std::vector<std::string> names;
        for (const auto &s : dynamicUsers) {
            std::cout << "[SERVICE] - " << s.name << std::endl;
            names.push_back(s.name);
        }

        for (const auto &s : extendedGroups) {
            std::cout << "[SERVICE] - " << s.name << std::endl;
            names.push_back(s.name);
        }
        msg.ret(::com::example::UserDbTypeWrap::stdStringVecToGlibStringVec(names));
    }

    void ListUsers(MethodInvocation &msg) override
    {
        std::cout << "[SERVICE] UserDb::ListUsers" << std::endl;
        std::vector<std::string> names;
        for (const auto &s : dynamicUsers) {
            std::cout << "[SERVICE] - " << s.name << std::endl;
            names.push_back(s.name);
        }
        msg.ret(::com::example::UserDbTypeWrap::stdStringVecToGlibStringVec(names));
    }
};

#define UNIX_SOCKET_FILE_NAME "/tmp/user-db.sock"
#define DEFAULT_BUS_PATH "unix:path=" UNIX_SOCKET_FILE_NAME

// Check that service is running:
// dbus-send --peer=unix:path=/tmp/user-db.sock --print-reply /com/example/UserDb com.example.UserDb.ListGroups
int main(void)
{
    Glib::init();
    Gio::init();

    const char *objectPath = "/com/example/UserDb";
    struct stat s;

    int r = stat(UNIX_SOCKET_FILE_NAME, &s);
    if (r == 0 && S_ISSOCK(s.st_mode)) {
        std::cerr << "File " << UNIX_SOCKET_FILE_NAME << " exists, deleting it..." << std::endl;
        unlink(UNIX_SOCKET_FILE_NAME);
    }

    // Set umask 0 to allow everybody to exchange messages with D-Bus serviceF
    umask(0);

    // Instantiate and run the main loop
    Glib::RefPtr<Glib::MainLoop> ml = Glib::MainLoop::create();

    UserDb userDb;
    Glib::RefPtr<Gio::DBus::Server> server;

    try {
        server = Gio::DBus::Server::create_sync(DEFAULT_BUS_PATH, Gio::DBus::generate_guid());
    }
    catch (const Glib::Error &ex) {
        std::cerr << "Error creating server at address: " << DEFAULT_BUS_PATH << ": " << ex.what() << "." << std::endl;
        return EXIT_FAILURE;
    }

    server->start();

    std::cout << "Server is listening at: " << server->get_client_address() << "." << std::endl;

    server->signal_new_connection().connect(
            [&](const Glib::RefPtr<Gio::DBus::Connection> &connection) {
                g_print("Connected to bus.\n");
                if (userDb.register_object(connection, objectPath) == 0) {
                    fprintf(stderr, "!!!\n");
                    return false;
                }
                return true;
            },
            false);

    ml->run();

    return 0;
}
