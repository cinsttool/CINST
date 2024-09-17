#include <filesystem>
#include "fs_util.h"
using namespace std;
void filesStartWith(const std::string& pathstr, std::vector<std::string>& files, const std::string& prefix)
{
    filesystem::path path(pathstr);
    if (!filesystem::exists(path) || !filesystem::is_directory(path)) {
        fprintf(stderr, "dir %s not exist\n", pathstr.c_str());
    }

    filesystem::directory_iterator dir(path);
    for (auto& ite: dir) {
        if (ite.status().type() == filesystem::file_type::regular) {
            string filename = ite.path().filename().string();
            if (startsWith(filename, prefix)) {
                files.push_back(filename);
            }
        }
    }
}