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
import os
import json
import datetime
from FileJump.FileJumpApi import FileJumpApi
from FileJump.Exceptions import FJError
from FileJump.ProgressCallback import IProgressCallback
from FileJump.LocalFileReader import LocalFileReader

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
        print(f"Uploading file {file_index+1}/{total_files}: {file_name}")

    def file_progress(self, file_name, uploaded, size):
        percent = int(uploaded * 100 / size)
        if percent != self.last_percent:
            self.last_percent = percent
            print(f"\r{file_name}: {percent}%", end='', flush=True)
            if percent == 100:
                print()



def test_fj_write():
    FileJumpApi.set_url("https://eu.filejump.com/api/v1/")
    FileJumpApi.set_token("62|liYE7ZABcnuw0zQugkYqTrv97R41NXWSir2j2MeC0d78094c")
    file_name = __file__
    relative_name = __name__
    api = FileJumpApi()
    try:
        # api.login("user", "password")
        # files = api.read_directory_tree()
        #
        # fr = LocalFileReader(file_name)
        # pc = PercentCallback()
        # res = api.post_file(file_name, fr, relative_name, pc)
        # print(res)

        # entry_id = res["fileEntry"]["id"]
        # parent_id = res["fileEntry"]["parent_id"]
        # file_info = api.get_file_info(parent_id, entry_id)
        # desc = f"Uploaded by test on {datetime.datetime.now().isoformat()}"
        # api.set_description(entry_id, desc)
        # file_info2 = api.get_file_info(parent_id, entry_id)

        entry_id = "976687"
        file = api.get_file(entry_id)
        print(file)

        entry_id = "976687"
        api.delete_files([entry_id])
        files = api.read_directory_tree()
        print(files)
        dup = [f["id"] for f in files if f.get("name") == "duplicates.csv"]
        api.delete_files(dup)

    except FJError as e:
        print(f"Error: {e}")
