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

#include <string>
#include <map>
#include <vector>
#include <windows.h>
#include "FileJump.h"

/**
 * @brief Utility class for URL and path manipulation
 *
 * CUrlTools provides static helper methods for working with URLs, encoding,
 * string conversions, and path operations. All methods are static and the
 * class is not meant to be instantiated.
 *
 * @note This class contains only static methods and should not be instantiated
 *
 * Example usage:
 * @code
 * std::wstring url = CUrlTools::buildUrlWithParams(
 *     L"https://api.example.com/data",
 *     {{L"key", L"value"}, {L"id", L"123"}}
 * );
 * // Result: "https://api.example.com/data?key=value&id=123"
 *
 * std::string parent = CUrlTools::getParentPath("/folder/subfolder/file.txt");
 * // Result: "/folder/subfolder"
 * @endcode
 */
class FILEJUMP_API CUrlTools
{
public:
    /**
     * @brief Build a URL with query parameters
     *
     * Constructs a complete URL by appending encoded query parameters to the base path.
     * Parameters are automatically URL-encoded and joined with '&'.
     *
     * @param basePath The base URL path (e.g., L"https://example.com/api")
     * @param params Map of parameter names to values
     * @return Complete URL with query string (e.g., L"https://example.com/api?key1=value1&key2=value2")
     *
     * @code
     * std::map<std::wstring, std::wstring> params = {
     *     {L"search", L"hello world"},
     *     {L"page", L"1"}
     * };
     * std::wstring url = CUrlTools::buildUrlWithParams(L"https://api.example.com", params);
     * // Result: "https://api.example.com?search=hello%20world&page=1"
     * @endcode
     */
    static std::wstring buildUrlWithParams(
        const std::wstring& basePath,
        const std::map<std::wstring, std::wstring>& params);
    /**
     * @brief URL-encode a wide string
     *
     * Encodes special characters in a string for safe use in URLs.
     * Spaces become %20, special characters are percent-encoded.
     *
     * @param value The string to encode
     * @return URL-encoded string
     *
     * @code
     * std::wstring encoded = CUrlTools::urlEncode(L"hello world!");
     * // Result: "hello%20world%21"
     * @endcode
     */
    static std::wstring urlEncode(const std::wstring& value);

    /**
     * @brief Convert narrow string (std::string) to wide string (std::wstring)
     *
     * Converts a standard string to a wide string for use with Windows APIs
     * and wide-character functions.
     *
     * @param input The narrow string to convert
     * @return Wide string equivalent
     *
     * @code
     * std::string narrow = "Hello";
     * std::wstring wide = CUrlTools::str2wide(narrow);
     * @endcode
     */
    static std::wstring str2wide(const std::string& input);
    static std::wstring Utf8ToWide(const std::string& str);
    /**
     * @brief Convert wide string (std::wstring) to narrow string (std::string) 
     *
     * Converts a wide string to a standard string for use with Windows APIs
     * and string-based functions.
     *
     * @param input The wide string to convert
     * @return standard string equivalent
     *
     * @code
     * std::string wide = L"Hello";
     * std::wstring narrow = CUrlTools::WideToUtf8(wide);
     * @endcode
     */
    static std::string WideToUtf8(const std::wstring& wstr);


    /**
     * @brief Create HTTP headers string from a map
     *
     * Converts a map of header names and values into a properly formatted
     * HTTP headers string with CRLF line endings.
     *
     * @param headersMap Map of header names to values
     * @return Formatted headers string (e.g., L"Content-Type: application/json\r\nAuthorization: Bearer token\r\n")
     *
     * @code
     * std::map<std::wstring, std::wstring> headers = {
     *     {L"Content-Type", L"application/json"},
     *     {L"Authorization", L"Bearer abc123"}
     * };
     * std::wstring headerStr = CUrlTools::createHeaders(headers);
     * @endcode
     */
    static std::wstring createHeaders(const std::map <std::wstring, std::wstring>& headersMap);
    /**
     * @brief Split a path string into components
     *
     * Splits a path into individual segments using the specified delimiter.
     * Empty segments are typically filtered out.
     *
     * @param path The path to split (e.g., "/folder/subfolder/file.txt")
     * @param delimiter The delimiter character (default: '/')
     * @return Vector of path components
     *
     * @code
     * auto parts = CUrlTools::splitPath("/folder/subfolder/file.txt");
     * // Result: {"folder", "subfolder", "file.txt"}
     * @endcode
     */
    static std::vector<std::string> splitPath(const std::string& path, char delimiter = '/');
    /**
     * @brief Split a INTEGER path string (like in FileJump) into integer components. Convert every component to integer ID
     *
     * @param s The string to split
     * @param delimiter The delimiter character (default: '/')
     * @return Vector of integer values
     *
     * @code
     * auto ids = CUrlTools::splitIntPath("123/456/789");
     * // Result: {123, 456, 789}
     * @endcode
     */
    static std::vector<int> splitIntPath(const std::string& s, char delimiter = '/');

    /**
     * @brief Get the parent directory path
     *
     * Extracts the parent directory from a file or directory path by
     * removing the last component.
     *
     * @param path The full path (e.g., "/folder/subfolder/file.txt")
     * @return Parent directory path (e.g., "/folder/subfolder")
     *
     * @code
     * std::string parent = CUrlTools::getParentPath("/home/user/documents/file.txt");
     * // Result: "/home/user/documents"
     *
     * std::string root = CUrlTools::getParentPath("/file.txt");
     * // Result: "/"
     * @endcode
     */
    static std::string getParentPath(const std::string& path);
    /**
     * @brief Get the name (last component) from a path
     *
     * Extracts the file or directory name from a full path by
     * returning the last component after the final delimiter.
     *
     * @param path The full path (e.g., "/folder/subfolder/file.txt")
     * @return Name component (e.g., "file.txt")
     *
     * @code
     * std::string name = CUrlTools::getName("/home/user/documents/file.txt");
     * // Result: "file.txt"
     *
     * std::string dirName = CUrlTools::getName("/home/user/documents/");
     * // Result: "documents"
     * @endcode
     */
    static std::string getName(const std::string& path);
    static FILETIME StringToFileTime(const std::string& isoString);

};

