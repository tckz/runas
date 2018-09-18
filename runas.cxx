#include <algorithm>
#include <memory>
#include <string>

#include <cstdio>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdint>

#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>

/*
 * g++ -o t -Wall -std=c++1y t.cxx
 */

static const std::string option_leader = "--";

static bool starts_with(const std::string& s, const std::string& prefix) {
    if (s.size() < prefix.size()) {
        return false;
    }
    return std::equal(prefix.begin(), prefix.end(), s.begin());
}

static void usage(const char* argv0, const char* mes) {
    fprintf(stderr, "usage: %s [--setuid uid] [--setgid gid] /path/to/command [argv...]\n", argv0);
    if (mes != nullptr) {
        fputs("\n", stderr);
        fputs(mes, stderr);
    }
}

static bool isdigit(const std::string& s) {
    return std::all_of(s.cbegin(), s.cend(),
        [](const char c) {
            return isdigit(c);
        });
}

static bool getuid(const char* name, uid_t* uid) {

    if (isdigit(name)) {
        *uid = (uid_t)atoi(name);
        return true;
    }

    struct passwd pwd = {0};
    struct passwd* result = nullptr;

    auto len = sysconf(_SC_GETPW_R_SIZE_MAX);
    std::unique_ptr<char[]> buf(new char[len]());

    auto err = getpwnam_r(name, &pwd, buf.get(), len, &result);
    if (result == nullptr) {
        fprintf(stderr, "*** getpwnam_r: user=%s, %s\n", name, err == 0 ? "not found" : strerror(err));
        return false;
    }

    *uid = result->pw_uid;
    return true;
}

static bool getgid(const char* name, gid_t* gid) {

    if (isdigit(name)) {
        *gid = (gid_t)atoi(name);
        return true;
    }

    struct group pwd = {0};
    struct group* result = nullptr;
    auto len = sysconf(_SC_GETGR_R_SIZE_MAX);
    std::unique_ptr<char[]> buf(new char[len]());

    auto err = getgrnam_r(name, &pwd, buf.get(), len, &result);
    if (result == nullptr) {
        fprintf(stderr, "*** getgrnam_r: group=%s, %s\n", name, err == 0 ? "not found" : strerror(err));
        return false;
    }

    *gid = result->gr_gid;
    return true;
}

int main(int argc, char* argv[]) {

    int args_begin = 0;
    uid_t uid = UINT32_MAX;
    gid_t gid = UINT32_MAX;
    for (int i = 1; i < argc; i++) {
        const char* arg = argv[i];
        const char* next_arg = i < (argc - 1) ? argv[i + 1] : nullptr;
        if (starts_with(arg, option_leader)) {
            if (std::string("--setuid") == arg) {
                if (next_arg == nullptr) {
                    usage(argv[0], "*** --setuid requires uid\n");
                    return 1;
                }
                if (!getuid(next_arg, &uid)) {
                    return 2;
                }
                i++;
            } else if (std::string("--setgid") == arg) {
                if (next_arg == nullptr) {
                    usage(argv[0], "*** --setgid requires gid\n");
                    return 1;
                }
                if (!getgid(next_arg, &gid)) {
                    return 2;
                }
                i++;
            } else if (std::string("--help") == arg) {
				usage(argv[0], nullptr);
				return 0;
            } else {
                break;
            }
        } else {
            args_begin = i;
            break;
        }
    }

    if (args_begin == 0) {
        usage(argv[0], nullptr);
        return 1;
    }

    if (gid != UINT32_MAX) {
        if (setgid(gid) == -1) {
            perror("*** Failed to setgid()");
            return 2;
        }
    }

    if (uid != UINT32_MAX) {
        if (setuid(uid) == -1) {
            perror("*** Failed to setuid()");
            return 2;
        }
    }

    std::unique_ptr<char*[]> p(new char*[argc]());
    for (int i = args_begin; i < argc; i++) {
        p[i - args_begin] = argv[i];
    }

    execvp(argv[args_begin], p.get());
    perror("*** Failed to execvp()");
    return 3;
}

