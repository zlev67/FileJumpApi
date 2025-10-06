#pragma once
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
#include "FileJump.h"

#include <string>
#include <vector>
#include <list>
#include <unordered_map>
#include <mutex>
#include <nlohmann/json.hpp>
#include <Windows.h>
using json = nlohmann::json;

struct FileInfo
{
	FileInfo()
	{
		path = {};
		size = 0;
		isDir = false;
		id = -1;
		parent_id = -1;
		created_at = { 0 };
		updated_at = { 0 };
	}

	std::string name;
	std::vector<int> path;
	uint64_t size;
	bool isDir;
	int id;
	int parent_id;
	FILETIME created_at;
	FILETIME updated_at;
};


class DirectoryLru
{
private:
	std::unordered_map <int, std::list<FileInfo>> filesLRU;
	std::list <int> pathLRU;
public:
	std::list<FileInfo> get(int path)
	{
		if (filesLRU.count(path))
		{
			pathLRU.remove(path);
			pathLRU.push_front(path);
			// Key exists
			return filesLRU[path];
		}
		return {};
	}
	void remove(int path)
	{
		filesLRU.erase(path);
		pathLRU.remove(path);
	}
	void add(int path, std::list<FileInfo> data)
	{
		if (pathLRU.size() > 20)
		{
			int path_to_remove = pathLRU.back();
			remove(path);
		}
		filesLRU[path] = data;
		pathLRU.push_front(path);
	}
};


class FILEJUMP_API FJAccess
{
private:
	static FJAccess* instance;
	static std::wstring m_baseUrl;
	static std::wstring m_bearerToken;
	static bool verbose;
	std::unordered_map <std::string, int> directoryCache;
	std::unordered_map <int, std::string> directoryTranslate;
	DirectoryLru m_lru;
	static std::mutex m_cache_mutex;

	std::string path2string(std::vector<int> path);
	std::list<FileInfo> get_files(int path_id);
	void fillDirectoryCache();
	void read_directory_tree(int id = 0);
	FileInfo *json2fileinfo(const json & response, const std::string & subtree, FileInfo* buf);


public:
	FJAccess();
	static void set_verbose(bool _verbose)
	{
		verbose = _verbose;
	}
	static bool configure_with_password(const std::wstring& baseUrl, const std::string& user, const std::string& password);
	static void configure(const std::wstring& base_url, const std::wstring& bearer_token)
	{
		m_baseUrl = base_url;
		m_bearerToken = bearer_token;
	}
	virtual ~FJAccess() = default;

	std::list <FileInfo> getDirectoryContent(int directoryID);
	int getDirectoryID(std::string directoryPath);
	const struct FileInfo* findFile(const std::string& path);
	bool copyFile(int id, const std::string& dest);
	bool deleteFile(int parent_id, int id);
	bool createDir(int id, const std::string& name);
	bool uploadFile(const std::string& source, int remotePathId, const std::string& remoteName);

	static FJAccess* getInstance()
	{
		if (!instance)
			instance = new FJAccess();
		return instance;
	}
	static void destroy()
	{
		delete instance;
	}
};

