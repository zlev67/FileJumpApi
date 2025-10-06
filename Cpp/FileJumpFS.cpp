/* ============================================================================== =
*
*MIT License
*
*Copyright(c) 2025 Lev Zlotin
*
*Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and /or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
*The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
*THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* ============================================================================== =*/

#define _CRT_SECURE_NO_WARNINGS
#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <string>
#include <vector>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <algorithm>
#include <filesystem>
#include <windows.h>
#include <fcntl.h>   // defines O_TRUNC, O_CREAT on many toolchains
#include <io.h>      // sometimes needed on Windows

#include "FJAccess.h"
#include "CUrlTools.h"
namespace fs = std::filesystem;

struct HandleInfo {
    std::string localPath;
    bool dirty = false;
};

static bool verbose = false;
static std::unordered_map<uint64_t, HandleInfo> g_handles;
static std::mutex g_handles_mutex;
static uint64_t g_next_handle = 1;
static std::string g_tempDir;

// normalize a fuse path like "/a/b.txt" -> "a/b.txt" (no leading slash for remote API)
static std::string norm(const char* path) {
    std::string s(path ? path : "");
    if (!s.empty() && s[0] == '/') s.erase(0, 1);
    return s;
}

struct fuse_timespec filetime_to_timespec(FILETIME ft) 
{
    struct fuse_timespec ts;

    // Combine low and high parts into 64-bit value
    uint64_t filetime = ((uint64_t)ft.dwHighDateTime << 32) | ft.dwLowDateTime;

    // FILETIME is 100-nanosecond intervals since 1601-01-01 00:00:00 UTC
    // Unix epoch is 1970-01-01 00:00:00 UTC
    const uint64_t EPOCH_DIFFERENCE = 116444736000000000ULL;

    if (filetime < EPOCH_DIFFERENCE) {
        // Date is before Unix epoch
        ts.tv_sec = 0;
        ts.tv_nsec = 0;
        return ts;
    }

    // Subtract epoch difference
    uint64_t unix_100ns = filetime - EPOCH_DIFFERENCE;

    // Convert to seconds and nanoseconds
    ts.tv_sec = unix_100ns / 10000000ULL;           // 10^7 * 100ns = 1 second
    ts.tv_nsec = (unix_100ns % 10000000ULL) * 100;  // Remaining 100ns to nanoseconds

    return ts;
}

static int fj_getattr(const char* path, struct fuse_stat* stbuf, struct fuse_file_info* fi) {
    (void)fi;
    if(verbose)
        fprintf(stderr, "getattr: %s\n", path);
    memset(stbuf, 0, sizeof(struct stat));
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        return 0;
    }
    if (fi && fi->fh != 0)
    {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = (off_t)0;
        return 0;
    }
    FJAccess* access = FJAccess::getInstance();
    const struct FileInfo *entry = access->findFile(path);
    if (!entry) 
        return -ENOENT;
    stbuf->st_birthtim = filetime_to_timespec(entry->created_at);
    stbuf->st_ctim = filetime_to_timespec(entry->updated_at);
    stbuf->st_atim = stbuf->st_ctim;
    stbuf->st_mtim = stbuf->st_atim;

    if (entry->isDir) {
        stbuf->st_mode = S_IFDIR | 0777;
        stbuf->st_nlink = 2;
        stbuf->st_size = (off_t)0;
    }
    else {
        stbuf->st_mode = S_IFREG | 0777;
        stbuf->st_nlink = 1;
        stbuf->st_size = (off_t)entry->size;
    }
    delete entry;
    return 0;
}

static int fj_readdir(const char* path, void* buf, fuse_fill_dir_t filler,
    fuse_off_t offset, struct fuse_file_info* fi, enum fuse_readdir_flags flags) {
    (void)offset;
    (void)fi;
	(void)flags;
    if (verbose)
        fprintf(stderr, "readdir: %s\n", path);
    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);
    //std::string p = norm(path);

    FJAccess* access = FJAccess::getInstance();
    auto entries = access->getDirectoryContent(access->getDirectoryID(path));
    // list unique names (FileJump may allow duplicates)
    for (auto& e : entries) {
        struct fuse_stat st = { 0 };
        st.st_birthtim = filetime_to_timespec(e.created_at);
        st.st_ctim = filetime_to_timespec(e.updated_at);
        st.st_atim = st.st_ctim;
        st.st_mtim = st.st_atim;
        if (e.isDir)
        {
            st.st_mode = S_IFDIR | 0777;
            st.st_nlink = 2;
        }
        else 
        {
            st.st_mode = S_IFREG | 0777;
            st.st_nlink = 1;
            st.st_size = (off_t)e.size;
        }
        filler(buf, e.name.c_str(), &st, 0, (fuse_fill_dir_flags)0);
    }
    return 0;
}

static int fj_create(const char* path, fuse_mode_t mode, struct fuse_file_info* fi) {

    if (verbose)
        fprintf(stderr, "create: %s\n", path);
    
    // Check if file already exists
    FJAccess* access = FJAccess::getInstance();
    const struct FileInfo* entry = access->findFile(path);
    if (entry) {
        delete entry;
        return -EEXIST;  // File already exists
    }

    // Create a handle just like in fj_open
    std::lock_guard<std::mutex> lk(g_handles_mutex);
    uint64_t handle = g_next_handle++;
    std::string remote = path;

    // Create temp file path
    std::string tmp = g_tempDir + "/fj_" + std::to_string(handle) + "_" +
        (remote.empty() ? "root" : remote);
    fs::path p(tmp);
    fs::create_directories(p.parent_path());

    // Create empty local file
    std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
    if (!ofs.is_open()) {
        return -EIO;
    }
    ofs.close();

    // Store handle info
    HandleInfo hi;
    hi.localPath = tmp;
    hi.dirty = true;  // Mark as dirty since it's a new file
    g_handles[handle] = hi;
    fi->fh = handle;

    if (verbose)
        fprintf(stderr, "create: %s - success, handle=%llu\n", path, handle);
    return 0;
}

static int fj_open(const char* path, struct fuse_file_info* fi) 
{
    if (verbose)
        fprintf(stderr, "open: %s\n", path);
    std::lock_guard<std::mutex> lk(g_handles_mutex);
    uint64_t handle = g_next_handle++;
    std::string remote = norm(path);
    std::string tmp = g_tempDir + "/fj_" + std::to_string(handle) + "_" + (remote.empty() ? "root" : remote);
    fs::path p(tmp);
    fs::create_directories(p.parent_path());

    bool createEmpty = (fi->flags & O_TRUNC) || (fi->flags & O_CREAT);
    if (!createEmpty) 
    {
        FJAccess* access = FJAccess::getInstance();
        const struct FileInfo *entry = access->findFile(path);
        if (entry)
        {
            bool ok = access->copyFile(entry->id, tmp);
            // try to download existing file; if fails, create empty
            if (!ok)
            {
                std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
                ofs.close();
            }
            delete entry;
        }
    }
    else {
        std::ofstream ofs(tmp, std::ios::binary | std::ios::trunc);
        ofs.close();
    }

    HandleInfo hi;
    hi.localPath = tmp;
    hi.dirty = false;
    g_handles[handle] = hi;
    fi->fh = handle;
    return 0;
}

static int fj_read(const char* path, char* buf, size_t size, fuse_off_t offset, struct fuse_file_info* fi) 
{
    (void)path;
    if (verbose)
        fprintf(stderr, "read: %s\n", path);
    uint64_t handle = fi->fh;
    std::lock_guard<std::mutex> lk(g_handles_mutex);
    auto it = g_handles.find(handle);
    if (it == g_handles.end()) return -EBADF;
    const std::string& local = it->second.localPath;

    std::ifstream ifs(local, std::ios::binary);
    if (!ifs.is_open()) return -EIO;
    ifs.seekg(0, std::ios::end);
    size_t fsize = (size_t)ifs.tellg();
    if ((size_t)offset >= fsize) return 0;
    ifs.seekg(offset, std::ios::beg);
    ifs.read(buf, size);
    size_t read = ifs.gcount();
    return (int)read;
}

static int fj_write(const char* path, const char* buf, size_t size, fuse_off_t offset, struct fuse_file_info* fi) 
{
    (void)path;
    if (verbose)
        fprintf(stderr, "write: %s\n", path);
    uint64_t handle = fi->fh;
    std::lock_guard<std::mutex> lk(g_handles_mutex);
    auto it = g_handles.find(handle);
    if (it == g_handles.end()) return -EBADF;
    const std::string& local = it->second.localPath;

    std::fstream fsFile(local, std::ios::binary | std::ios::in | std::ios::out);
    if (!fsFile.is_open()) {
        std::ofstream ofs(local, std::ios::binary | std::ios::trunc);
        ofs.close();
        fsFile.open(local, std::ios::binary | std::ios::in | std::ios::out);
        if (!fsFile.is_open()) return -EIO;
    }
    fsFile.seekp(offset, std::ios::beg);
    fsFile.write(buf, size);
    fsFile.flush();
    it->second.dirty = true;
    return (int)size;
}

static int fj_unlink(const char* path) 
{
    if (verbose)
        fprintf(stderr, "unlink: %s\n", path);
    FJAccess* fj = FJAccess::getInstance();
    const struct FileInfo* entry = fj->findFile(path);
    if (!entry)
        return -ENOENT;
    const struct FileInfo* parent = fj->findFile(CUrlTools::getParentPath(path));
    fj->deleteFile(parent->id, entry->id);
    delete entry;
    delete parent;
    return 0;
}

static int fj_mkdir(const char* path, fuse_mode_t mode) 
{
    (void)mode;
    if (verbose)
        fprintf(stderr, "mkdir: %s\n", path);

    std::string parent = CUrlTools::getParentPath(path);
    std::string name = CUrlTools::getName(path);
    FJAccess* fj = FJAccess::getInstance();
    const struct FileInfo* parent_entry = fj->findFile(parent);
    if (!parent_entry && parent != "")
    {
        if (parent_entry)
            delete parent_entry;
        return -ENOENT;
    }
    int parent_id = parent_entry ? parent_entry->id : 0;
    bool success = fj->createDir(parent_id, name);
    delete parent_entry;
    return success? 0: -ENOENT;
}

static int fj_rmdir(const char* path) 
{
    if (verbose)
        fprintf(stderr, "rmdir: %s\n", path);
    FJAccess* access = FJAccess::getInstance();
    // Check if directory exists
    const struct FileInfo* entry = access->findFile(path);
    if (!entry) {
        return -ENOENT;  // Directory not found
    }
    if (!entry->isDir) {
        delete entry;
        return -ENOTDIR;  // Not a directory
    }
    // Check if directory is empty
    auto contents = access->getDirectoryContent(entry->id);
    if (!contents.empty()) {
        delete entry;
        return -ENOTEMPTY;  // Directory not empty
    }

    // Delete the directory
    const struct FileInfo* parent_entry = access->findFile(CUrlTools::getParentPath(path));
    bool success = access->deleteFile(parent_entry->id, entry->id);  // or deleteDirectory()
    delete entry;
    delete parent_entry;
    if (!success)
    {
        return -EIO;  // I/O error
    }
    return 0;
}

static int fj_release(const char* path, struct fuse_file_info* fi) 
{
    uint64_t handle = fi->fh;
    if (verbose)
        fprintf(stderr, "release: %s\n", path);
    HandleInfo hi;
    {
        std::lock_guard<std::mutex> lk(g_handles_mutex);
        auto it = g_handles.find(handle);
        if (it == g_handles.end()) return 0;
        hi = it->second;
        g_handles.erase(it);
    }

    if (hi.dirty) {
        // delete remote first (to prevent duplicates)
        fj_unlink(path);
        std::string parent = CUrlTools::getParentPath(path);
        std::string name = CUrlTools::getName(path);
        FJAccess* fj = FJAccess::getInstance();
        const struct FileInfo* parent_info = fj->findFile(parent);
        if (parent_info)
        {
            std::string localPath = hi.localPath;
            std::replace(localPath.begin(), localPath.end(), '/', '\\');
            bool ok = fj->uploadFile(localPath, parent_info->id, name);
            delete parent_info;
            if (!ok)
            {
                return -EIO;
            }
        }
    }

    try { fs::remove(hi.localPath); }
    catch (...) {}
    return 0;
}

static struct fuse_operations fj_oper = {};

int main(int argc, char* argv[]) 
{
    // read env config
    std::wstring baseUrl, auth;
    std::string user, password;
    char const* baseUrlEnv = std::getenv("FILEJUMP_BASE_URL");
    char const* authEnv = std::getenv("FILEJUMP_AUTH_TOKEN");
    if (baseUrlEnv)
    {
        std::string t = baseUrlEnv;
        baseUrl = std::wstring(t.begin(), t.end()).c_str();
    }

    if (authEnv)
    {
        std::string t = authEnv;
        auth = std::wstring(t.begin(), t.end()).c_str();
    }

    if (argc > 1)
        for (int arg = 1; arg < argc; arg++)
        {
            if (std::string(argv[arg]) == "--verbose")
            {
                verbose = true;
                FJAccess::set_verbose(verbose);
            }
            if (std::string(argv[arg]) == "--server")
                baseUrl = CUrlTools::Utf8ToWide(argv[arg + 1]);
            if (std::string(argv[arg]) == "--token")
                auth = CUrlTools::Utf8ToWide(argv[arg + 1]);
            if (std::string(argv[arg]) == "--user-email")
                user = argv[arg + 1];
            if (std::string(argv[arg]) == "--password")
                password = argv[arg + 1];
        }
    if (baseUrl.empty() || (auth.empty() && password.empty()))
    {
        std::string usage;
        usage =  "FileJumpFS can be used to mount the filejump cloud storage to disk on your windows.\n";
        usage += "Using the mounted drive letter, you can see the cloud storage content, copy files to or from cloud storage\n";
        usage += "The following FileJump servers can be used: https://app.filejump.com/, https://drive.filejump.com/, https://eu.filejump.com/ \n";
        usage += "parameters are:\n't--server: URL of server to use;\n\t--token: security token to access to FileJump media;\n";
        usage += "\t--user-email and --password to authenticate with user name and password (instead of token);\n";
        usage += "It is also possible to authenticate with environment variables FILEJUMP_BASE_URL and FILEJUMP_AUTH_TOKEN - just set variables instead of command line;\n";
        usage += "--verbose to get more information for debugging\n";
        fprintf(stderr, usage.c_str());
        exit(-1);
    }
    if (!user.empty() and !password.empty())
        FJAccess::configure_with_password(baseUrl, user, password);

    FJAccess::configure(baseUrl, auth);

    // prepare temp dir
    char tmpPath[MAX_PATH];
    if (GetTempPathA(MAX_PATH, tmpPath) == 0) {
        g_tempDir = ".";
    }
    else {
        g_tempDir = std::string(tmpPath) + "filejumpfs";
    }
    fs::create_directories(g_tempDir);

    fj_oper.getattr = fj_getattr;
    fj_oper.readdir = fj_readdir;
    fj_oper.open = fj_open;
    fj_oper.create = fj_create;
    fj_oper.read = fj_read;
    fj_oper.write = fj_write;
    fj_oper.unlink = fj_unlink;
    fj_oper.mkdir = fj_mkdir;
    fj_oper.rmdir = fj_rmdir;
    fj_oper.release = fj_release;

    return fuse_main(argc, argv, &fj_oper, NULL);
}
