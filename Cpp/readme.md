\# FileJumpFS



FileJumpFS is a filesystem mapper that mounts FileJump cloud storage as a Windows drive letter, allowing you to access your cloud files as if they were local.



\## Features



\- 🔌 Mount FileJump cloud storage to any available Windows drive letter

\- 📁 Browse cloud storage content through Windows File Explorer

\- 📤 Copy files to and from cloud storage seamlessly

\- 🔐 Multiple authentication methods (token or username/password)

\- 🌍 Support for multiple FileJump servers

\- 🐛 Verbose mode for debugging



\## Supported FileJump Servers



\- `https://app.filejump.com/`

\- `https://drive.filejump.com/`

\- `https://eu.filejump.com/`



\## Requirements



\- Windows 10/11

\- \[WinFsp](https://github.com/winfsp/winfsp/releases) (Windows File System Proxy) installed



\## Installation



1\. Download and install \[WinFsp](https://github.com/winfsp/winfsp/releases)

2\. Download FileJumpFS executable

3\. Run from command line with appropriate parameters



\## Usage



\### Basic Syntax



```bash

FileJumpFS \[options] <mount\_point>

```



\### Authentication Methods



\#### Option 1: Using Access Token



```bash

FileJumpFS --server https://app.filejump.com/ --token YOUR\_TOKEN Z:

```



\#### Option 2: Using Username and Password



```bash

FileJumpFS --server https://app.filejump.com/ --user-email your@email.com --password yourpassword Z:

```



\#### Option 3: Using Environment Variables



Set environment variables:

```bash

set FILEJUMP\_BASE\_URL=https://app.filejump.com/

set FILEJUMP\_AUTH\_TOKEN=YOUR\_TOKEN

```



Then run:

```bash

FileJumpFS Z:

```



\### Command Line Parameters



| Parameter | Description |

|-----------|-------------|

| `--server <URL>` | URL of the FileJump server to use |

| `--token <TOKEN>` | Security token for authentication |

| `--user-email <EMAIL>` | User email for authentication (alternative to token) |

| `--password <PASSWORD>` | User password for authentication (use with --user-email) |

| `--verbose` | Enable verbose output for debugging |



Plus all standard FUSE parameters supported by WinFsp.



\### Examples



\*\*Mount with token to drive Z:\*\*

```bash

FileJumpFS --server https://app.filejump.com/ --token abc123xyz Z:

```



\*\*Mount with credentials to drive Y:\*\*

```bash

FileJumpFS --server https://eu.filejump.com/ --user-email user@example.com --password mypass Y:

```



\*\*Mount with environment variables and verbose mode:\*\*

```bash

set FILEJUMP\_BASE\_URL=https://drive.filejump.com/

set FILEJUMP\_AUTH\_TOKEN=your\_token\_here

FileJumpFS --verbose Z:

```



\## Unmounting



To unmount the filesystem:



1\. \*\*Press Ctrl+C\*\* in the terminal where FileJumpFS is running

2\. Or close the terminal window

3\. Or end the FileJumpFS process from Task Manager



\## Environment Variables



| Variable | Description |

|----------|-------------|

| `FILEJUMP\_BASE\_URL` | Base URL of FileJump server (alternative to --server) |

| `FILEJUMP\_AUTH\_TOKEN` | Authentication token (alternative to --token) |



\## Troubleshooting



\### Drive letter not showing up

\- Ensure WinFsp is properly installed

\- Try a different drive letter

\- Check if the drive letter is already in use



\### Authentication errors

\- Verify your token or credentials are correct

\- Check that the server URL is correct and accessible

\- Ensure your FileJump account has proper permissions



\### Connection issues

\- Use `--verbose` flag to see detailed error messages

\- Check your internet connection

\- Verify the FileJump server is accessible



\### Cannot unmount

\- Close all programs accessing the mounted drive

\- Press Ctrl+C in the FileJumpFS terminal

\- As last resort, end the FileJumpFS.exe process in Task Manager



\## Advanced Usage



\### Running in Background



To run FileJumpFS in the background, you can use the standard FUSE `-f` (foreground) flag or create a Windows service using WinFsp's launcher:



```bash

launchctl-x64 start FileJumpFS FileJumpFS.exe --server https://app.filejump.com/ --token YOUR\_TOKEN Z:

```



\### Additional FUSE Options



FileJumpFS supports standard FUSE/WinFsp options such as:

\- `-d` or `--debug` - Debug mode

\- `-f` - Run in foreground

\- `-s` - Single-threaded operation

\- `-o <option>` - Mount options



Example:

```bash

FileJumpFS --server https://app.filejump.com/ --token YOUR\_TOKEN -o uid=1000 Z:

```



\## License



MIT License



Copyright (c) 2025 Lev Zlotin



Permission is hereby granted, free of charge, to any person obtaining a copy

of this software and associated documentation files (the "Software"), to deal

in the Software without restriction, including without limitation the rights

to use, copy, modify, merge, publish, distribute, sublicense, and/or sell

copies of the Software, and to permit persons to whom the Software is

furnished to do so, subject to the following conditions:



The above copyright notice and this permission notice shall be included in all

copies or substantial portions of the Software.



THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR

IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,

FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE

AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER

LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,

OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE

SOFTWARE.



\## Support



For issues and questions:

\- FileJumpFS issues: \[https://github.com/zlev67/FileJumpApi/issues](https://github.com/zlev67/FileJumpApi/issues)

\- FileJump service: \[https://filejump.com/support](https://filejump.com/support)

\- WinFsp issues: \[https://github.com/winfsp/winfsp/issues](https://github.com/winfsp/winfsp/issues)



\## Version



Current version: 1.0.0



---



\*\*Note\*\*: Keep your authentication tokens secure. Do not share them or commit them to version control systems.

