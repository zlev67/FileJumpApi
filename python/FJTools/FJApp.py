# MIT License
#
# Copyright (c) 2025 Lev Zlotin
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in all
# copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.
import shutil
import getpass
import os
import argparse
import tempfile

from FileJump.FileJumpApi import FileJumpApi
from FileJump.LocalFileReader import LocalFileReader
from FileJump.ProgressCallback import IProgressCallback


class FJApp:
    class PercentCallback(IProgressCallback):
        def __init__(self):
            self.file_name = None
            self.total_files = 0
            self.file_index = 0
            self.last_percent = -1

        def start_file(self, file_index, file_name, total_files):
            self.file_name = file_name
            self.total_files = total_files
            self.file_index = file_index
            self.last_percent = -1
            print(f"Uploading file {file_index + 1}/{total_files}: {file_name}")

        def file_progress(self, file_name, uploaded, size):
            percent = int(uploaded * 100 / size)
            if percent != self.last_percent:
                self.last_percent = percent
                print(f"\r{file_name}: {percent}%", end='', flush=True)
                if percent == 100:
                    print()

    def __init__(self, arguments):
        self.arguments = arguments
        if self.arguments is None:
            raise ValueError("Arguments cannot be None")
        if self.arguments.server is None or self.arguments.server.strip() == "":
            raise ValueError("Server URL must be provided")
        FileJumpApi.set_url(self.arguments.server)
        self.fj_api = FileJumpApi()
        if self.arguments.token is None or self.arguments.token.strip() == "":
            has_token = input("No token provided. Do you have token to access the server? [Yes/No] (default No): ")
            if has_token.lower() in ['y', 'yes']:
                token = getpass.getpass("Enter your token: ")
                self.fj_api.set_token(token)
            else:
                user_name = input("Enter your user name: ")
                password = getpass.getpass("Enter your password: ")
                self.fj_api.login(user_name, password)

    def _upload(self):
        if os.path.isdir(self.arguments.upload):
            fd, temp_path = tempfile.mkstemp(suffix='.zip', prefix=self.arguments.upload)
            os.close(fd)  # Close file descriptor
            file_path = shutil.make_archive(temp_path.replace('.zip', ''), 'zip', self.arguments.upload)
        else:
            file_path = self.arguments.upload
        file_reader = LocalFileReader(file_path)
        if self.arguments.remote_path:
            remote_path = self.arguments.remote_path
        else:
            remote_path = None
        self.fj_api.post_file(file_path, file_reader, remote_path, FJApp.PercentCallback())

    def _download(self):
        pass

    def _read_dir(self):
        try:
            all_entries, folders = self.fj_api.read_directory_tree()
            print("Directory listing:")
            for item in all_entries:
                item_type = "DIR" if item['is_dir'] else "FILE"
                size = item['size']
                mod_time = item['modified']
                print(f"{item_type:4} {size:10} {mod_time} {item['name']}")
        except Exception as e:
            print(f"Error reading directory: {e}")

    def run(self):
        if self.arguments.read_dir:
            self._read_dir()
            return True

        elif self.arguments.upload:
            if not os.path.isfile(self.arguments.upload) and not os.path.isdir(self.arguments.upload):
                print(f"Error: '{self.arguments.upload}' path does not exist.")
            else:
                self._upload()
            return True
        elif self.arguments.download:
            print(f"Downloading '{args.download}' from '{args.server}' with token '{args.token}'")
            # Implement download logic here
            return True
        return False

def main(args=None):
    parser = argparse.ArgumentParser(
        description='FileJump command line tool',
        epilog='Example: fjtool --help',
    )
    parser.add_argument('--version', action='version', version='FileJump 0.1.1')
    parser.add_argument('--read_dir', action='store_true', help='Download recursive directory list from FileJump server')
    parser.add_argument('--upload', type=str, help='Upload file or directory to FileJump server')
    parser.add_argument('--download', type=str, help='Download file from FileJump server')
    parser.add_argument('--server', type=str, help='FileJump server URL')
    parser.add_argument('--token', type=str, help='Authentication token for FileJump server')
    parser.add_argument('--verbose', action='store_true', help='Enable verbose output')
    parser.add_argument('--remote_path', type=str, help='Remote path on the server to download files from or upload to (default is a root directory)')
    parser.add_argument('--local_path', type=str, help='Local path on your computer to upload files from or download to (default is a current directory)')

    args = parser.parse_args(args)
    fj_app = FJApp(args)
    res = fj_app.run()
    if not res:
        parser.print_help()


if __name__ == '__main__':
    main()
