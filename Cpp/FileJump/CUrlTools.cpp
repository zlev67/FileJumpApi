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

#include "CUrlTools.h"
#include <sstream>
#include <wtypes.h>
#include <vector>



FILEJUMP_API std::wstring CUrlTools::buildUrlWithParams(
    const std::wstring& basePath,
    const std::map<std::wstring, std::wstring>& params)
{
    if (params.empty())
    {
        return basePath;
    }

    std::wostringstream url;
    url << basePath << "?";

    bool first = true;
    for (const auto& param : params) {
        if (!first) {
            url << "&";
        }
        url << urlEncode(param.first) << "=" << urlEncode(param.second);
        first = false;
    }

    return url.str();
}

FILEJUMP_API std::wstring CUrlTools::urlEncode(const std::wstring& value)
{
    std::wostringstream encoded;
    for (WCHAR c : value) {
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            encoded << c;
        }
        else {
            encoded << '%' << std::hex << std::uppercase << (unsigned char)c;
        }
    }
    return encoded.str();
}

// Convert wide string to UTF-8
FILEJUMP_API std::string CUrlTools::WideToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string result(size - 1, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), -1, &result[0], size, nullptr, nullptr);
    return result;
}

// Convert UTF-8 to wide string
FILEJUMP_API std::wstring CUrlTools::Utf8ToWide(const std::string& str) {
    if (str.empty()) return std::wstring();
    int size = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, nullptr, 0);
    std::wstring result(size - 1, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), -1, &result[0], size);
    return result;
}


FILEJUMP_API std::wstring CUrlTools::str2wide(const std::string& input)
{
    return Utf8ToWide(input);
}

FILEJUMP_API std::wstring CUrlTools::createHeaders(const std::map <std::wstring, std::wstring> &headersMap)
{
    std::wostringstream headerString;
    for (const auto& header : headersMap) {
        headerString << header.first << ": " << header.second << "\r\n";
    }
    std::wstring headerStr = headerString.str();
    return headerStr;
}

FILEJUMP_API std::vector<std::string> CUrlTools::splitPath(const std::string& path, char delimiter)
{
    std::vector<std::string> parts;
    std::stringstream ss(path);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) {  // skip empty parts (e.g. leading '/')
            parts.push_back(item);
        }
    }
    return parts;
}
FILEJUMP_API std::vector<int> CUrlTools::splitIntPath(const std::string& s, char delimiter)
{
    std::vector<int> tokens = { 0 };
    std::stringstream ss(s);
    std::string item;

    while (std::getline(ss, item, delimiter)) {
        if (!item.empty()) { // skip empty parts (like leading "/")
            tokens.push_back(std::stoi(item));
        }
    }
    return tokens;
}

FILEJUMP_API std::string CUrlTools::getParentPath(const std::string& path)
{
    std::string p = path;
    // attempt to find in parent directory
    std::string parent = "/";
    size_t pos = p.rfind('/');
    std::string parentPath = (pos == std::string::npos) ? "/" : p.substr(0, pos);
    return parentPath;
}

FILEJUMP_API std::string CUrlTools::getName(const std::string& path)
{
    std::string p = path;
    // attempt to find in parent directory
    size_t pos = p.rfind('/');
    std::string name = (pos == std::string::npos) ? p : p.substr(pos + 1);
    return name;
}

FILEJUMP_API FILETIME CUrlTools::StringToFileTime(const std::string& isoString) {
    // Parse ISO 8601: "2025-10-03T13:07:48.000000Z"
    SYSTEMTIME st = { 0 };
    int year, month, day, hour, minute, second, microseconds;

    // Parse the string
    sscanf_s(isoString.c_str(), "%d-%d-%dT%d:%d:%d.%dZ",
        &year, &month, &day, &hour, &minute, &second, &microseconds);

    st.wYear = year;
    st.wMonth = month;
    st.wDay = day;
    st.wHour = hour;
    st.wMinute = minute;
    st.wSecond = second;
    st.wMilliseconds = microseconds / 1000;

    FILETIME ft;
    SystemTimeToFileTime(&st, &ft);

    return ft;
}