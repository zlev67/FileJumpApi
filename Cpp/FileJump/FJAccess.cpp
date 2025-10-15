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
#include "FJAccess.h"
#include <fstream>
#include "CUrlTools.h"
#include "fj_wininet.h"
#define JSON_DIAGNOSTICS 1
#include <nlohmann/json.hpp>
using json = nlohmann::json;

std::wstring FJAccess::m_baseUrl;
std::wstring FJAccess::m_bearerToken;
FJAccess* FJAccess::instance;
std::mutex FJAccess::m_cache_mutex;
bool FJAccess::verbose = false;

FJAccess::FJAccess()
{
    directoryTranslate[0] = "/";
}

FILEJUMP_API FileInfo* FJAccess::json2fileinfo(const json& json_response, const std::string& subtree, FileInfo* buf)
{
    json j;
    if (!subtree.empty() && json_response.contains(subtree))
        j = json_response[subtree];
    else
        j = json_response;
    if (verbose)
    {
        std::string debug_str = j.dump(2).c_str();
        fprintf(stderr, "JSON parse: %s\n", debug_str.c_str());
    }
    try {
        auto ttt = j["name"];
        buf->name = j["name"];
        buf->path = CUrlTools::splitIntPath(j["path"]);
        buf->size = 0;
        buf->isDir = (j["type"] == "folder");
        if(!buf->isDir)
            buf->size = j["file_size"];
        buf->id = j["id"];
        int parent_id = 0;
        if (j.contains("parent_id") && !j["parent_id"].is_null())
            buf->parent_id = j["parent_id"];
        buf->created_at = CUrlTools::StringToFileTime(j["created_at"]);
        buf->updated_at = CUrlTools::StringToFileTime(j["updated_at"]);
    }
    catch (const json::exception& e)
    {
        if(verbose)
            fprintf(stderr, e.what());
    }
    return buf;
}

bool FILEJUMP_API FJAccess::configure_with_password(const std::wstring& baseUrl, const std::string& user, const std::string& password)
{
    class LoginFileTools
    {
    public:
        static std::wstring get_url(std::wstring const& _base_url)
        {
            std::map<std::wstring, std::wstring> params = {};
            return CUrlTools::buildUrlWithParams(_base_url + std::wstring(L"api/v1/auth/login"), params);
        };
        static std::wstring get_header()
        {
            return CUrlTools::createHeaders({
                {L"Accept", L"application/json"},
                {L"Content-Type", L"application/json"},
                {L"User-Agent", L"WindowsHttpClient/1.0"}
                });
        }
        static std::string getData(const std::string& email, const std::string& password)
        {
            json j;
            j["email"] = email;
            j["password"] = password;
            j["token_name"] = std::string("fuse3_token");
            std::string out = j.dump(2); // 2 = indent with 2 spaces
            return out;
        }
    };
    m_baseUrl = baseUrl;
    if (m_baseUrl[m_baseUrl.length() - 1] != L'/')
        m_baseUrl += L"/";

    std::string postResponse = HttpPost(LoginFileTools::get_url(m_baseUrl), LoginFileTools::get_header(), LoginFileTools::getData(user, password));
    if (postResponse.empty())
        return false;
    json j = json::parse(postResponse);
    if (j.contains("user"))
    {
        auto j1 = j["user"];
        auto token = j1["access_token"];
        std::wstring bearerToken = CUrlTools::Utf8ToWide(token);
        configure(m_baseUrl, bearerToken);
        return true;
    }
    return false;
}


std::list<FileInfo> FILEJUMP_API FJAccess::get_files(int path_id)
{
    class GetFileTools
    {
    public:
        static std::wstring get_url(std::wstring const& base_url, int path_id, int page)
        {
            std::map<std::wstring, std::wstring> params = { {L"perPage", L"1000"}, {L"workspaceId", L"0"},
                {L"parentIds", std::to_wstring(path_id)}, {L"page", std::to_wstring(page) } };
            return CUrlTools::buildUrlWithParams(base_url + std::wstring(L"api/v1/drive/file-entries"), params);
        };
        static std::wstring get_header(const std::wstring& token)
        {
            return CUrlTools::createHeaders({
                {L"Content-Type", L"application/json"},
                {L"Authorization", L"Bearer " + token},
                {L"User-Agent", L"WindowsHttpClient/1.0"} });
        }
    };
    int next_page = 0;
    std::list<FileInfo> res;
    while (true)
    {
        auto response = HttpGet(GetFileTools::get_url(m_baseUrl, path_id, next_page), 
                                GetFileTools::get_header(m_bearerToken));
        if (response.empty())
        {
            int error = GetLastError();
            return res;
        }
        json j = json::parse(response);

        // Access next_page (could be null)
        next_page = -1;
        if (!j["next_page"].is_null()) {
            next_page = j["next_page"].get<int>();
        }

        // Access data array
        if (j.contains("data") && j["data"].is_array()) {
            // Iterate through each item in data array
            for (const auto& item : j["data"])
            {
                FileInfo fi;
                json2fileinfo(item, "", &fi);
                res.push_back(fi);
            }
        }
        if (next_page == -1)
            break;
    }
    return res;
}
std::string FILEJUMP_API FJAccess::path2string(std::vector<int> path)
{
    std::string out;
    std::string last = "root";
    for (auto s : path)
    {
        out += directoryTranslate[s];
    }
    return out;
}

void FILEJUMP_API FJAccess::read_directory_tree(int id)
{
    // guard for external stop flag: you may map Globals.should_stop to an external atomic flag if needed
    // For now we do not check for cancellation
    auto entries = get_files(id);
    for (auto& entry : entries) 
    {
        if (entry.isDir) 
        {
            directoryTranslate[entry.id] = entry.name;
            std::string path = path2string(entry.path);
            directoryCache[path] = entry.id;
            read_directory_tree(entry.id);
        }
    }
}

FILEJUMP_API const struct FileInfo* FJAccess::findFile(const std::string& path)
{
    std::string parentPath = CUrlTools::getParentPath(path);
    std::string name = CUrlTools::getName(path);
    int parent_id = getDirectoryID(parentPath);
    auto entries = getDirectoryContent(parent_id);
    for (auto& e : entries)
    {
        if (e.name == name)
        {
            return new struct FileInfo(e);
        }
    }
    return nullptr;
}

bool FILEJUMP_API FJAccess::copyFile(int id, const std::string& dest)
{
    class CopyFileTools
    {
    public:
        static std::wstring get_url(std::wstring const& base_url, int id)
        {
            std::map<std::wstring, std::wstring> params = {};
            return CUrlTools::buildUrlWithParams(base_url + std::wstring(L"api/v1/file-entries/")+std::to_wstring(id), params);
        };
        static std::wstring get_header(const std::wstring& token)
        {
            return CUrlTools::createHeaders({
                {L"Content-Type", L"application/json"},
                {L"Authorization", L"Bearer " + token},
                {L"User-Agent", L"WindowsHttpClient/1.0"} });
        }
    };
    std::wstring url = CopyFileTools::get_url(m_baseUrl, id);
    std::wstring headers = CopyFileTools::get_header(m_bearerToken);
    auto response = HttpGet(url, headers);
    if (response.length() == 0)
        return false;
    std::ofstream outFile(dest, std::ios::binary);
    outFile.write(response.c_str(), response.length());
    if (outFile.bad()|| outFile.fail()) {
        return false;
    }

    return true;
}
bool FILEJUMP_API FJAccess::deleteFile(int parent_id, int id)
{
    class DeleteFileTools
    {
    public:
        static std::wstring get_url(std::wstring const& base_url)
        {
            std::map<std::wstring, std::wstring> params = {};
            return CUrlTools::buildUrlWithParams(base_url + std::wstring(L"api/v1/file-entries/delete"), params);
        };
        static std::wstring get_header(const std::wstring& token)
        {
            return CUrlTools::createHeaders({
                {L"Accept", L"application/json"},
                {L"Content-Type", L"application/json"},
                {L"Authorization", L"Bearer " + token},
                {L"User-Agent", L"WindowsHttpClient/1.0"} 
                });
        }
        static std::string getData(int id)
        {
            json j;
            j["entryIds"] = { std::to_string(id) };
            j["deleteForever"] = true;
            std::string out = j.dump(2); // 2 = indent with 2 spaces
            return out;
        }
    };

    std::string deleteResponse = HttpPost(DeleteFileTools::get_url(m_baseUrl), DeleteFileTools::get_header(m_bearerToken), DeleteFileTools::getData(id));
    m_lru.remove(parent_id);
    if (!deleteResponse.empty())
        return false;
    return true;
}

bool FILEJUMP_API FJAccess::createDir(int id, const std::string& name)
{
    class CreateFolderTools
    {
    public:
        static std::wstring get_url(std::wstring const& base_url)
        {
            std::map<std::wstring, std::wstring> params = {};
            return CUrlTools::buildUrlWithParams(base_url + std::wstring(L"api/v1/folders"), params);
        };
        static std::wstring get_header(const std::wstring& token)
        {
            return CUrlTools::createHeaders({
                {L"Accept", L"application/json"},
                {L"Content-Type", L"application/json"},
                {L"Authorization", L"Bearer " + token},
                {L"User-Agent", L"WindowsHttpClient/1.0"}
                });
        }
        static std::string getData(int parent_id, std::string folder_name)
        {
            json j;
            j["name"] = folder_name;
            if(parent_id)
                j["parentId"] = parent_id ;
            std::string out = j.dump(2); // 2 = indent with 2 spaces
            return out;
        }
    };
    std::string createResponse = HttpPost(CreateFolderTools::get_url(m_baseUrl), CreateFolderTools::get_header(m_bearerToken), CreateFolderTools::getData(id, name));
    if (createResponse.empty())
        return false;
    json json_response = json::parse(createResponse);
    auto j = json_response["folder"];
    FileInfo fi;
    json2fileinfo(json_response, "folder", &fi);
    m_lru.remove(fi.parent_id);

    std::lock_guard<std::mutex> guard(m_cache_mutex);
    directoryTranslate[fi.id] = fi.name;
    std::string path = path2string(fi.path);
    directoryCache[path] = fi.id;

    // Set headers
    return true;
}

bool FILEJUMP_API FJAccess::uploadFile(const std::string& source, int remotePath, const std::string& remoteName)
{
    class UploadFileTools
    {
    public:
        static std::wstring get_url(std::wstring const& base_url)
        {
            std::map<std::wstring, std::wstring> params = {};
            return CUrlTools::buildUrlWithParams(base_url + std::wstring(L"api/v1/uploads"), params);
        };
        static std::wstring get_header(const std::wstring& token)
        {
            return CUrlTools::createHeaders({
                {L"Accept", L"*/*"},
                {L"Content-Type", L"application/json"},
                {L"Authorization", L"Bearer " + token},
                {L"User-Agent", L"WindowsHttpClient/1.0"}
                });
        }
    };

    // Form fields
    std::map<std::string, std::string> fields = 
    {
        {"parentId", std::to_string(remotePath)},
        {"relativePath", remoteName},
        {"description", "Uploaded via API"}
    };
    
    // Files to upload
    std::vector<FileField> files = 
    {
        {"file", source.c_str()}  // field name, file path
    };
    
    std::string multipartResponse = HttpPostMultipart(UploadFileTools::get_url(m_baseUrl), m_bearerToken, fields, source.c_str());
    if (multipartResponse.empty()) 
    {
        return false;
    }
    json json_response = json::parse(multipartResponse);
    if (json_response.contains("fileEntry"))
    {
        auto file = json_response["fileEntry"];
        if (file.contains("parent_id"))
        {
            auto parent_id = file["parent_id"];
            m_lru.remove(parent_id);
        }
    }
    return true;
}


void FILEJUMP_API FJAccess::fillDirectoryCache()
{
    directoryCache["/"] = 0;
    read_directory_tree(0);
}

std::list<FileInfo> FILEJUMP_API FJAccess::getDirectoryContent(int directoryID)
{
    std::lock_guard<std::mutex> guard(m_cache_mutex);
    std::list<FileInfo> out = m_lru.get(directoryID);
    if (out.empty())
    {
        out = get_files(directoryID);
        m_lru.add(directoryID, out);
    }
    return out;
}

int FILEJUMP_API FJAccess::getDirectoryID(std::string directoryPath)
{
    std::lock_guard<std::mutex> guard(m_cache_mutex);
    if (directoryCache.empty())
        fillDirectoryCache();
    return directoryCache[directoryPath];
}

