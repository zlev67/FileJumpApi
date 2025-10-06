/* ==============================================================================
*
* MIT License
*
* Copyright(c) 2025 Lev Zlotin
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files(the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions :
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* ==============================================================================*/

#include <windows.h>
#include <wininet.h>
#include <string>
#include <iostream>
#include <vector>
#include <fstream>
#include <ctime>
#include "fj_wininet.h"
#include <random>
#include <sstream>
#include <iomanip>
#include <map>
#include "CUrlTools.h"

#pragma comment(lib, "wininet.lib")

/**
 * Performs an HTTP GET request using WinInet API
 *
 * @param url     The complete URL to send the GET request to (wide string)
 * @param headers Optional HTTP headers to include in the request (wide string)
 * @return        Response body as a string, or empty string on failure
 *
 * Note: This is a simplified implementation using InternetOpenUrl
 *       which doesn't provide fine-grained control over the request
 */
std::string HttpGet(const std::wstring& url, const std::wstring& headers) {
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    std::string responseData;

    // Initialize WinINet session with default configuration
    hInternet = InternetOpen(
        L"WinINetGetRequest",           // User agent string
        INTERNET_OPEN_TYPE_PRECONFIG,   // Use registry/system proxy settings
        NULL, NULL, 0);                 // No explicit proxy

    if (!hInternet) {
        std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
        return "";
    }

    // Open the URL directly - combines connect and request creation
    hConnect = InternetOpenUrl(
        hInternet,
        url.c_str(),                           // Target URL
        headers.c_str(), (DWORD)headers.length(), // HTTP headers
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE, // Don't use cache
        0);                                    // Context value for callbacks

    if (!hConnect) {
        std::cerr << "InternetOpenUrl failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return "";
    }

    // Query the HTTP status code from the response
    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    if (HttpQueryInfo(hConnect,
        HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER, // Get status as number
        &statusCode,
        &statusCodeSize,
        NULL)) {
        if (statusCode != 200) {
            std::cerr << "HTTP Status: " << statusCode << std::endl;
        }
    }

    // Read the response body in chunks
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hConnect, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseData.append(buffer, bytesRead);
    }

    // Clean up handles
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return responseData;
}

/**
 * Generic HTTP request function that supports any HTTP method
 *
 * @param method  HTTP method (GET, POST, PUT, DELETE, etc.)
 * @param url     Complete URL for the request
 * @param headers HTTP headers to send (wide string format)
 * @param data    Request body data (for POST, PUT, etc.)
 * @return        Response body as string, or empty string on failure
 *
 * This function provides more control than HttpGet by allowing:
 * - Custom HTTP methods
 * - Request body data
 * - Full header control
 * - Proper URL parsing and connection handling
 */
std::string HttpRequest(const std::wstring& method, const std::wstring& url,
    const std::wstring& headers, const std::string& data)
{
    HINTERNET hInternet = NULL;
    HINTERNET hConnect = NULL;
    HINTERNET hRequest = NULL;
    std::string responseData;

    // Parse the URL into its components (scheme, host, path, port)
    URL_COMPONENTS urlComp = {};
    urlComp.dwStructSize = sizeof(URL_COMPONENTS);
    urlComp.dwSchemeLength = -1;    // Calculate length automatically
    urlComp.dwHostNameLength = -1;
    urlComp.dwUrlPathLength = -1;

    if (!InternetCrackUrl(url.c_str(), 0, 0, &urlComp)) {
        std::cerr << "InternetCrackUrl failed: " << GetLastError() << std::endl;
        return "";
    }

    // Initialize WinINet with method-specific agent name
    std::wstring agentName = L"WinINet" + method + L"Request";
    hInternet = InternetOpen(
        agentName.c_str(),
        INTERNET_OPEN_TYPE_PRECONFIG,
        NULL, NULL, 0);

    if (!hInternet) {
        std::cerr << "InternetOpen failed: " << GetLastError() << std::endl;
        return "";
    }

    // Establish connection to the server
    std::wstring hostname(urlComp.lpszHostName, urlComp.dwHostNameLength);
    hConnect = InternetConnect(
        hInternet,
        hostname.c_str(),
        urlComp.nPort,              // Use port from URL (80 for HTTP, 443 for HTTPS)
        NULL, NULL,                 // No username/password
        INTERNET_SERVICE_HTTP,
        0, 0);

    if (!hConnect) {
        std::cerr << "InternetConnect failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hInternet);
        return "";
    }

    // Create the HTTP request
    std::wstring urlPath(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
    hRequest = HttpOpenRequest(
        hConnect,
        method.c_str(),             // HTTP method (GET, POST, etc.)
        urlPath.c_str(),            // URL path (everything after hostname)
        NULL,                       // HTTP version (NULL = HTTP/1.1)
        NULL,                       // Referrer
        NULL,                       // Accept types
        (urlComp.nScheme == INTERNET_SCHEME_HTTPS ? INTERNET_FLAG_SECURE : 0) |
        INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE,
        0);

    if (!hRequest) {
        std::cerr << "HttpOpenRequest failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    // Send the HTTP request with headers and optional body data
    BOOL result = HttpSendRequest(
        hRequest,
        headers.c_str(),
        (DWORD)headers.length(),
        data.empty() ? NULL : (LPVOID)data.c_str(), // Send data only if provided
        (DWORD)data.length());

    if (!result) {
        std::cerr << "HttpSendRequest failed: " << GetLastError() << std::endl;
        InternetCloseHandle(hRequest);
        InternetCloseHandle(hConnect);
        InternetCloseHandle(hInternet);
        return "";
    }

    // Read the response body
    char buffer[4096];
    DWORD bytesRead;
    while (InternetReadFile(hRequest, buffer, sizeof(buffer), &bytesRead) && bytesRead > 0) {
        responseData.append(buffer, bytesRead);
    }

    // Clean up all handles
    InternetCloseHandle(hRequest);
    InternetCloseHandle(hConnect);
    InternetCloseHandle(hInternet);

    return responseData;
}

/**
 * Wrapper function for HTTP PUT requests
 * @param url     Target URL
 * @param headers HTTP headers
 * @param data    Request body
 * @return        Response body
 */
std::string HttpPut(const std::wstring& url, const std::wstring& headers, const std::string& data) {
    return HttpRequest(L"PUT", url, headers, data);
}

/**
 * Wrapper function for HTTP POST requests
 * @param url     Target URL
 * @param headers HTTP headers
 * @param data    Request body
 * @return        Response body
 */
std::string HttpPost(const std::wstring& url, const std::wstring& headers, const std::string& data) {
    return HttpRequest(L"POST", url, headers, data);
}

/**
 * Wrapper function for HTTP DELETE requests
 * @param url     Target URL
 * @param headers HTTP headers
 * @param data    Optional request body
 * @return        Response body
 */
std::string HttpDelete(const std::wstring& url, const std::wstring& headers, const std::string& data)
{
    return HttpRequest(L"DELETE", url, headers, data);
}

/**
 * FileUploader class for uploading large files via HTTP POST with multipart/form-data encoding
 *
 * Features:
 * - Streams file data to avoid loading entire file in memory
 * - Supports Bearer token authentication
 * - Automatic retry with exponential backoff on timeout
 * - Cancellable uploads
 * - Custom form fields support
 * - Automatic MIME type detection
 */
class FileUploader {
private:
    std::wstring baseUrl;              // Base URL for upload endpoint
    std::wstring token;                // Bearer authentication token
    bool cancel;                       // Flag to cancel ongoing upload
    static const size_t CHUNK_SIZE = 65536; // 64KB chunks for streaming

    /**
     * Generates a random boundary string for multipart/form-data encoding
     * Format: ----WebKitFormBoundary[16 hex digits]
     *
     * @return Boundary string for separating multipart sections
     */
    std::string GenerateBoundary() {
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, 15);

        std::stringstream ss;
        ss << "----WebKitFormBoundary";
        for (int i = 0; i < 16; i++) {
            ss << std::hex << dis(gen);
        }
        return ss.str();
    }

    /**
     * Determines MIME type based on file extension
     *
     * @param filePath Full path to the file
     * @return         MIME type string (e.g., "image/jpeg", "application/pdf")
     *                 Returns "application/octet-stream" for unknown types
     */
    std::string GetMimeType(const std::string& filePath) {
        size_t dotPos = filePath.find_last_of('.');
        if (dotPos == std::string::npos)
        {
            return "application/octet-stream";
        }

        // Extract extension and convert to lowercase
        std::string ext = filePath.substr(dotPos + 1);
        std::transform(ext.begin(), ext.end(), ext.begin(), ::towlower);

        // Map of file extensions to MIME types
        static const std::map<std::string, std::string> mimeTypes = {
            {"txt", "text/plain"},
            {"json", "application/json"},
            {"xml", "application/xml"},
            {"html", "text/html"},
            {"css", "text/css"},
            {"js", "application/javascript"},
            {"jpg", "image/jpeg"},
            {"jpeg", "image/jpeg"},
            {"png", "image/png"},
            {"gif", "image/gif"},
            {"bmp", "image/bmp"},
            {"svg", "image/svg+xml"},
            {"pdf", "application/pdf"},
            {"doc", "application/msword"},
            {"docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document"},
            {"xls", "application/vnd.ms-excel"},
            {"xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"},
            {"ppt", "application/vnd.ms-powerpoint"},
            {"pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation"},
            {"zip", "application/zip"},
            {"rar", "application/x-rar-compressed"},
            {"7z", "application/x-7z-compressed"},
            {"mp3", "audio/mpeg"},
            {"wav", "audio/wav"},
            {"mp4", "video/mp4"},
            {"avi", "video/x-msvideo"},
            {"mov", "video/quicktime"}
        };

        auto it = mimeTypes.find(ext);
        return (it != mimeTypes.end()) ? it->second : "application/octet-stream";
    }

    /**
     * Extracts filename from a full file path
     * Handles both forward slash (/) and backslash (\) separators
     *
     * @param filePath Full path to file
     * @return         Filename without path
     */
    std::string GetFileName(const std::string& filePath) {
        size_t pos = filePath.find_last_of("/\\");
        if (pos == std::string::npos) {
            return filePath;
        }
        return filePath.substr(pos + 1);
    }

    /**
     * Gets the size of a file in bytes
     *
     * @param filePath Path to the file
     * @return         File size in bytes, or 0 if file cannot be opened
     */
    size_t GetFileSize(const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            return 0;
        }
        size_t size = file.tellg();
        file.close();
        return size;
    }

    /**
     * Builds the header section of a multipart/form-data request
     *
     * @param filePath Path to the file being uploaded
     * @param fields   Map of form field names to values
     * @param boundary Boundary string separating multipart sections
     * @return         Complete multipart header as string
     *
     * Format:
     * --boundary
     * Content-Disposition: form-data; name="fieldName"
     *
     * fieldValue
     * --boundary
     * Content-Disposition: form-data; name="file"; filename="filename.ext"
     * Content-Type: mime/type
     *
     * [file data follows]
     */
    std::string BuildMultipartHeader(const std::string& filePath,
        const std::map<std::string, std::string>& fields,
        const std::string& boundary)
    {
        std::stringstream ss;
        std::string fileName = GetFileName(filePath);
        std::string mimeType = GetMimeType(filePath);

        // Add all form fields before the file
        for (const auto& field : fields) {
            ss << "--" << boundary << "\r\n";
            ss << "Content-Disposition: form-data; name=\"" << field.first << "\"\r\n\r\n";
            ss << field.second << "\r\n";
        }

        // Add file field header (file data will be streamed separately)
        ss << "--" << boundary << "\r\n";
        ss << "Content-Disposition: form-data; name=\"file\"; filename=\"" << fileName << "\"\r\n";
        ss << "Content-Type: " << mimeType << "\r\n\r\n";

        return ss.str();
    }

    /**
     * Builds the footer section of a multipart/form-data request
     *
     * @param boundary Boundary string
     * @return         Multipart footer string
     *
     * Format:
     * --boundary--
     */
    std::string BuildMultipartFooter(const std::string& boundary) {
        return "\r\n--" + boundary + "--\r\n";
    }

    /**
     * Writes data to an HTTP request handle, handling partial writes
     *
     * @param hRequest WinInet request handle
     * @param data     Pointer to data buffer
     * @param size     Number of bytes to write
     * @return         true on success, false on failure or cancellation
     *
     * Note: InternetWriteFile may not write all data in one call,
     *       so this function loops until all data is written
     */
    bool WriteToRequest(HINTERNET hRequest, const char* data, DWORD size) {
        DWORD totalWritten = 0;
        while (totalWritten < size) {
            if (cancel) {
                return false;
            }

            DWORD written = 0;
            if (!InternetWriteFile(hRequest, data + totalWritten, size - totalWritten, &written)) {
                return false;
            }
            totalWritten += written;
        }
        return true;
    }

    /**
     * Streams file content to an HTTP request in chunks
     * Reads file in 64KB chunks to minimize memory usage
     *
     * @param hRequest WinInet request handle
     * @param filePath Path to file to upload
     * @return         true on success, false on failure or cancellation
     *
     * This allows uploading files larger than available memory
     */
    bool StreamFileToRequest(HINTERNET hRequest, const std::string& filePath) {
        std::ifstream file(filePath, std::ios::binary);
        if (!file.is_open()) {
            return false;
        }

        std::vector<char> buffer(CHUNK_SIZE);

        // Read and write file in chunks
        while (file.read(buffer.data(), CHUNK_SIZE) || file.gcount() > 0) {
            if (cancel) {
                file.close();
                return false;
            }

            DWORD bytesToWrite = static_cast<DWORD>(file.gcount());
            if (!WriteToRequest(hRequest, buffer.data(), bytesToWrite)) {
                file.close();
                return false;
            }
        }

        file.close();
        return true;
    }

public:
    /**
     * Constructor
     * @param baseUrl Base URL for the upload endpoint
     * @param token   Bearer authentication token
     */
    FileUploader(const std::wstring& baseUrl, const std::wstring& token)
        : baseUrl(baseUrl), token(token), cancel(false) {
    }

    /**
     * Cancels an ongoing upload operation
     * Sets the cancel flag which is checked during streaming
     */
    void Cancel() {
        cancel = true;
    }

    /**
     * Uploads a file via HTTP POST with multipart/form-data encoding
     *
     * @param filePath Path to file to upload
     * @param fields   Map of additional form fields (name -> value)
     * @return         Server response body as string
     * @throws         std::runtime_error on failure
     *
     * Features:
     * - Automatic retry with exponential backoff (1s -> 10s -> 100s)
     * - Streams file data to support large files
     * - Validates HTTP 201 status code
     * - Supports cancellation via Cancel() method
     *
     * The upload is performed using HttpSendRequestEx for streaming:
     * 1. Send headers with HttpSendRequestEx
     * 2. Stream multipart header with InternetWriteFile
     * 3. Stream file content in chunks with InternetWriteFile
     * 4. Stream multipart footer with InternetWriteFile
     * 5. Complete request with HttpEndRequest
     * 6. Read and return response
     */
    std::string PostFile(const std::string& filePath, const std::map<std::string, std::string>& fields)
    {
        std::string responseUtf8;
        cancel = false;

        // Generate unique boundary for this request
        std::string boundary = GenerateBoundary();

        // Build multipart header and footer
        std::string header = BuildMultipartHeader(filePath, fields, boundary);
        std::string footer = BuildMultipartFooter(boundary);

        // Get file size to calculate total content length
        size_t fileSize = GetFileSize(filePath);
        if (fileSize == 0) {
            throw std::runtime_error("Failed to read file or file is empty");
        }

        // Calculate total size of request body
        DWORD contentLength = static_cast<DWORD>(header.size() + fileSize + footer.size());

        // Retry loop with exponential backoff for timeout errors
        int timeout = 1000; // Start with 1 second timeout

        while (true) {
            // Initialize WinInet session
            HINTERNET hInternet = InternetOpen(L"FileUploader/1.0",
                INTERNET_OPEN_TYPE_PRECONFIG,
                NULL, NULL, 0);
            if (!hInternet) {
                throw std::runtime_error("Failed to initialize WinInet");
            }

            // Set timeouts for this request
            InternetSetOption(hInternet, INTERNET_OPTION_CONNECT_TIMEOUT, &timeout, sizeof(timeout));
            InternetSetOption(hInternet, INTERNET_OPTION_SEND_TIMEOUT, &timeout, sizeof(timeout));
            InternetSetOption(hInternet, INTERNET_OPTION_RECEIVE_TIMEOUT, &timeout, sizeof(timeout));

            // Parse the target URL
            std::wstring fullUrl = baseUrl;
            URL_COMPONENTSW urlComponents = { 0 };
            urlComponents.dwStructSize = sizeof(urlComponents);
            wchar_t szHostName[256] = { 0 };
            wchar_t szUrlPath[1024] = { 0 };
            urlComponents.lpszHostName = szHostName;
            urlComponents.dwHostNameLength = sizeof(szHostName) / sizeof(wchar_t);
            urlComponents.lpszUrlPath = szUrlPath;
            urlComponents.dwUrlPathLength = sizeof(szUrlPath) / sizeof(wchar_t);

            if (!InternetCrackUrl(fullUrl.c_str(), 0, 0, &urlComponents)) {
                InternetCloseHandle(hInternet);
                throw std::runtime_error("Failed to parse URL");
            }

            // Connect to server
            HINTERNET hConnect = InternetConnect(hInternet,
                urlComponents.lpszHostName,
                urlComponents.nPort,
                NULL, NULL,
                INTERNET_SERVICE_HTTP,
                0, 0);
            if (!hConnect) {
                InternetCloseHandle(hInternet);
                throw std::runtime_error("Failed to connect to server");
            }

            // Set flags for the request
            DWORD flags = INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE;
            if (urlComponents.nScheme == INTERNET_SCHEME_HTTPS) {
                flags |= INTERNET_FLAG_SECURE; // Enable SSL for HTTPS
            }

            // Create HTTP POST request
            HINTERNET hRequest = HttpOpenRequest(hConnect, L"POST",
                urlComponents.lpszUrlPath,
                NULL, NULL, NULL, flags, 0);
            if (!hRequest) {
                InternetCloseHandle(hConnect);
                InternetCloseHandle(hInternet);
                throw std::runtime_error("Failed to create request");
            }

            // Build HTTP headers
            std::wstring authHeader = L"Authorization: Bearer " + token;
            std::wstring contentType = L"Content-Type: multipart/form-data; boundary=" + CUrlTools::Utf8ToWide(boundary);
            std::wstring acceptHeader = L"Accept: application/json";
            std::wstring contentLengthHeader = L"Content-Length: " + std::to_wstring(contentLength);
            std::wstring headers = authHeader + L"\r\n" +
                contentType + L"\r\n" +
                acceptHeader + L"\r\n" +
                contentLengthHeader + L"\r\n";

            // Initialize streaming request with HttpSendRequestEx
            INTERNET_BUFFERSW buffers = { 0 };
            buffers.dwStructSize = sizeof(INTERNET_BUFFERSW);
            buffers.dwBufferTotal = contentLength;  // Total size we'll stream
            buffers.lpcszHeader = headers.c_str();
            buffers.dwHeadersLength = (DWORD)headers.length();

            BOOL result = HttpSendRequestEx(hRequest, &buffers, NULL, 0, 0);

            if (!result) {
                DWORD error = GetLastError();
                InternetCloseHandle(hRequest);
                InternetCloseHandle(hConnect);
                InternetCloseHandle(hInternet);

                // Retry with longer timeout if we got a timeout error
                if (error == ERROR_INTERNET_TIMEOUT && timeout <= 10000) {
                    timeout *= 10;
                    continue;
                }
                throw std::runtime_error("Failed to send request");
            }

            // Stream the multipart body in three parts
            bool streamSuccess = true;

            // 1. Write multipart header (form fields)
            if (!WriteToRequest(hRequest, header.c_str(), (DWORD)header.size())) {
                streamSuccess = false;
            }

            // 2. Stream file content in chunks
            if (streamSuccess && !StreamFileToRequest(hRequest, filePath)) {
                streamSuccess = false;
            }

            // 3. Write multipart footer
            if (streamSuccess && !WriteToRequest(hRequest, footer.c_str(), (DWORD)footer.size())) {
                streamSuccess = false;
            }

            if (!streamSuccess) {
                InternetCloseHandle(hRequest);
                InternetCloseHandle(hConnect);
                InternetCloseHandle(hInternet);

                if (cancel) {
                    return ""; // User cancelled
                }
                throw std::runtime_error("Failed to stream file data");
            }

            // Complete the request (signals end of streaming)
            if (!HttpEndRequest(hRequest, NULL, 0, 0)) {
                DWORD error = GetLastError();
                InternetCloseHandle(hRequest);
                InternetCloseHandle(hConnect);
                InternetCloseHandle(hInternet);

                // Retry with longer timeout
                if (error == ERROR_INTERNET_TIMEOUT && timeout <= 10000) {
                    timeout *= 10;
                    continue;
                }
                throw std::runtime_error("Failed to end request");
            }

            // Check HTTP status code
            DWORD statusCode = 0;
            DWORD statusCodeSize = sizeof(statusCode);
            HttpQueryInfo(hRequest, HTTP_QUERY_STATUS_CODE | HTTP_QUERY_FLAG_NUMBER,
                &statusCode, &statusCodeSize, NULL);

            // Read response body
            const DWORD bufferSize = 4096;
            char buffer[bufferSize];
            DWORD bytesRead;
            while (InternetReadFile(hRequest, buffer, bufferSize, &bytesRead) && bytesRead > 0) {
                responseUtf8.append(buffer, bytesRead);
            }

            // Clean up handles
            InternetCloseHandle(hRequest);
            InternetCloseHandle(hConnect);
            InternetCloseHandle(hInternet);

            // Validate status code (expecting 201 Created)
            if (statusCode != 201) {
                throw std::runtime_error("Upload failed with status " +
                    std::to_string(statusCode) + ": " + responseUtf8);
            }

            // Success - exit retry loop
            break;
        }

        return responseUtf8;
    }
};

/**
 * Convenience function for uploading files with multipart/form-data
 *
 * @param url      Complete URL for upload endpoint
 * @param token    Bearer authentication token
 * @param fields   Map of form fields (name -> value)
 * @param fileName Path to file to upload
 * @return         Server response body
 * @throws         std::runtime_error on failure
 *
 * This is a simple wrapper around FileUploader class for single file uploads
 */
std::string HttpPostMultipart(const std::wstring& url, const std::wstring& token,
    const std::map<std::string, std::string>& fields,
    const std::string& fileName)
{
    FileUploader uploader(url, token);
    std::string response = uploader.PostFile(fileName, fields);
    return response;
}