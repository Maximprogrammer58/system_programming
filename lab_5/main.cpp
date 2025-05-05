#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <unistd.h>

using namespace std;

struct UserInfo {
    string username;
    string password_hash;
    string home_dir;
    int uid;
    vector<string> groups;
    vector<string> admin_groups;
};

vector<string> split(const string &s, char delimiter) {
    vector<string> tokens;
    string token;
    istringstream tokenStream(s);
    while (getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

map<string, vector<string>> read_group_admins(ifstream& gshadow) {
    map<string, vector<string>> group_admins;
    string line;

    gshadow.clear();
    gshadow.seekg(0);

    while (getline(gshadow, line)) {
        vector<string> parts = split(line, ':');
        if (parts.size() >= 3) {
            string group_name = parts[0];
            vector<string> admin_list = split(parts[2], ',');

            for (const auto& admin : admin_list) {
                if (!admin.empty()) {
                    group_admins[admin].push_back(group_name);
                }
            }
        }
    }
    return group_admins;
}

map<string, vector<string>> read_user_groups(ifstream& group_file) {
    map<string, vector<string>> user_groups;
    string line;

    group_file.clear();
    group_file.seekg(0);

    while (getline(group_file, line)) {
        vector<string> parts = split(line, ':');
        if (parts.size() >= 4) {
            string group_name = parts[0];
            string members = parts[3];
            vector<string> member_list = split(members, ',');

            for (const auto& member : member_list) {
                if (!member.empty()) {
                    user_groups[member].push_back(group_name);
                }
            }
        }
    }
    return user_groups;
}

vector<UserInfo> read_users(ifstream& passwd, ifstream& shadow,
                           const map<string, vector<string>>& user_groups,
                           const map<string, vector<string>>& group_admins) {
    vector<UserInfo> users;
    string passwd_line, shadow_line;

    passwd.clear();
    passwd.seekg(0);
    shadow.clear();
    shadow.seekg(0);

    while (getline(passwd, passwd_line) && getline(shadow, shadow_line)) {
        vector<string> passwd_parts = split(passwd_line, ':');
        vector<string> shadow_parts = split(shadow_line, ':');

        if (passwd_parts.size() >= 6 && shadow_parts.size() >= 2) {
            UserInfo user;
            user.username = passwd_parts[0];
            user.uid = stoi(passwd_parts[2]);
            user.home_dir = passwd_parts[5];
            user.password_hash = shadow_parts[1];

            if (user_groups.count(user.username)) {
                user.groups = user_groups.at(user.username);
            }

            if (group_admins.count(user.username)) {
                user.admin_groups = group_admins.at(user.username);
            }

            users.push_back(user);
        }
    }
    return users;
}

void print_users(const vector<UserInfo>& users) {
    for (const auto& user : users) {
        cout << "Username: " << user.username << endl;
        cout << "UID: " << user.uid << endl;
        cout << "Home directory: " << user.home_dir << endl;
        cout << "Password hash: " << user.password_hash << endl;

        cout << "Groups: ";
        for (const auto& group : user.groups) {
            bool is_admin = find(user.admin_groups.begin(),
                               user.admin_groups.end(),
                               group) != user.admin_groups.end();
            cout << group;
            if (is_admin) {
                cout << "(*)";
            }
            cout << " ";
        }

        cout << "\n----------------------------------------\n";
    }
}

int main() {
    ifstream file_shadow("/etc/shadow", ifstream::in);
    ifstream file_gshadow("/etc/gshadow", ifstream::in);

    if (setuid(getuid()) < 0) {
        exit(-1);
    }

    ifstream file_passwd("/etc/passwd", ifstream::in);
    ifstream file_group("/etc/group", ifstream::in);

    if (!file_passwd || !file_shadow || !file_gshadow || !file_group) {
        cerr << "Error opening system files" << endl;
        return 1;
    }

    auto group_admins = read_group_admins(file_gshadow);
    auto user_groups = read_user_groups(file_group);
    auto users = read_users(file_passwd, file_shadow, user_groups, group_admins);

    print_users(users);

    return 0;
}