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
import datetime
import json
import logging
import os
from time import sleep

from PySide6.QtWidgets import QInputDialog

from fj_sync.filejump.Exceptions import FJError
from fj_sync.filejump.FileJumpApi import FileJumpApi
from fj_sync.synchronize.FileOperations import FileOperations
from fj_sync.tools.Tools import Tools

logger = logging.getLogger(__name__)

class FjOperations(FileOperations):
    _all_fj_entries = None
    _all_fj_folders = None

    @property
    def all_fj_entries(self):
        if FjOperations._all_fj_entries is None:
            FjOperations._all_fj_entries, FjOperations._all_fj_folders = self.api.read_directory_tree()
        return self._all_fj_entries

    @property
    def all_fj_folders(self):
        if FjOperations._all_fj_folders is None:
            FjOperations._all_fj_entries, FjOperations._all_fj_folders = self.api.read_directory_tree()
        return self._all_fj_folders

    def __init__(self):
        """
        Initializes the FjOperations class.
        This class provides methods to interact with FileJump API for file operations.
        """
        super().__init__()
        self.api = FileJumpApi()

    def rename(self, files_list):
        pass


    def get_file_info(self, parent_id, entry_id):
        """
        Retrieves detailed information about a file entry by its ID.

        :param entry_id: ID of the file entry to retrieve
        :return: Detailed file entry data from the API
        """
        files_info = self.api.get_files(parent_id)
        if files_info:
            for file in files_info:
                if file.get("id") == entry_id:
                    return file
        return None

    def delete(self, file_list):
        """
        Deletes all files in the given list of FileJump entry IDs.
        If a file is the last in its directory, removes the directory too (recursively up).
        :param file_list: List of file entry IDs to delete.
        """
        entry_ids = [file["id"] for file in file_list]
        logger.info(f"Deleting files: {len(entry_ids)} entries")
        if not entry_ids:
            return
        # Delete files

        self.api.delete_files(entry_ids, delete_forever=True)
        # Check for empty directories and delete them
        logger.info(f"Deleting: search empty directories.")
        dir_tree, folders = self.api.read_directory_tree()
        while True:
            empty_dirs = list()
            for entry in folders:
                if entry.get('empty'):
                    empty_dirs.append(entry["id"])
            logger.info(f"Deleting empty directories. {len(empty_dirs)} empty directories found.")
            if empty_dirs:
                self.api.delete_files(list(empty_dirs), delete_forever=True)
            else:
                break
        logger.info(f"Deleting files Done")

    def read(self, files_list, target_dir, percent_callback=None):
        """
        Download a list of files from FileJump to the local target directory.
        :param files_list: List of file dicts (must contain 'id' and 'name')
        :param percent_callback progress callback function to report download progress
        :param target_dir: Local directory to save files
        """
        chunk_size = 16384  # 16 MB
        for file in files_list:
            entry_id = file.get("id")
            name = file.get("name")
            if not entry_id or not name:
                continue
            response = self.api.get_file(entry_id)
            downloaded = 0
            total = int(response.headers.get('content-length', 0))
            file_path = os.path.join(target_dir, name)
            with open(file_path, "wb") as f:
                for chunk in response.iter_content(chunk_size=chunk_size):
                    if chunk:
                        f.write(chunk)
                        downloaded += len(chunk)
                        if percent_callback and total:
                            percent = int(downloaded * 100 / total)
                            percent_callback(percent)

    def create_folders(self, files_list, parent_id=None):
        """
        Create folders in FileJump based on the provided list of file dicts.
        :param files_list: List of folder dicts (must contain 'name' and 'ppath')
        :param parent_id: Optional parent folder id in FileJump
        """
        if not files_list:
            return
        folders_list = {item["name"]:item["id"] for item in self.all_fj_folders}
        for file_info in files_list:
            path = file_info.get("ppath", "")
            path_list = path.split("\\")
            for i in range(len(path_list)):
                folder = path_list[i]
                if len(folder) < 3:
                    logger.warning(f"Folder name will be mangled to get 3 letters: '{folder}' in path '{path}'.")
                    folder = folder.ljust(3, "_")

                if folder not in folders_list:
                    parent_folder = path_list[i-1] if i > 0 else parent_id
                    parent_folder = parent_folder.ljust(3, "_")
                    num_tries = 0
                    while True:
                        if num_tries > 5:
                            raise FJError(f"Failed to create folder {folder} under {parent_folder} after 5 attempts.")
                        res = None
                        try:
                            num_tries+=1
                            res = self.api.create_folder(folder, folders_list[parent_folder])
                            break
                        except FJError as e:
                            logger.info(f"Failed to create folder {folder} under {parent_folder}. Sleep 20 sec")
                            sleep(20)
                            continue
                    if res is not None:
                        folder_data = res.get("folder")
                        if folder_data:
                            folder_data["ppath"] = "\\".join(path_list[:i+1])
                            folders_list[folder_data["name"]] = folder_data.get("id")
                            self.all_fj_folders.append(folder_data)
                        else:
                            raise FJError(f"Failed to create folder {folder}: No folder data in response.")
                    else:
                        raise FJError(f"Failed to create folder: No response from API for {folder} under {parent_folder}.")

    def write(self, file_operations, files_list, path, percent_callback):
        """
        Upload a list of files to FileJump.

        :param files_list: List of local file dicts (must contain 'path' and 'name')
        :param parent_id: Optional parent folder id in FileJump
        :param percent_callback: Callback function to report upload progress
        """
        def write_file(file_info):
            """
            Helper function to upload a single file.
            :param file_path: Local path of the file to upload
            :param parent_id: Parent folder ID in FileJump
            :param relative_path: Relative path in FileJump
            """
            file_path = file_info.get("path")
            name = file_info.get("name")
            if not file_path or not name:
                return
            full_name = os.path.join(file_path, name)
            relative_path = file_info.get("ppath", name)
            dj_all_folders = {int(item["id"]): item["name"] for item in self.all_fj_folders}
            try:
                file_reader = file_operations.read(file_info)
                if not file_reader:
                    logger.error(f"Failed to read source file {full_name}")
                    return
                res = self.api.post_file(full_name, file_reader, relative_path, percent_callback)
                if res is None:
                    logger.error(f"Failed to upload {full_name}: No response from API.")
                    return
                file_entry = res.get("fileEntry")
                if not file_entry:
                    logger.error(f"Failed to upload {full_name}: No file entry in response.")
                    return
                entry_id = file_entry["id"]
                uploaded_path = file_entry["path"]
                uploaded_path_list = uploaded_path.split("/")
                for uploaded_folder in uploaded_path_list:
                    if uploaded_folder not in dj_all_folders:
                        break
                uploaded_name = file_entry["name"]
                if uploaded_name != name:
                    logger.warning(f"Uploaded file name mismatch: {uploaded_name} != {name}. "
                                   f"FileJump API may have renamed the file.")
                desc = json.dumps(
                    {
                        "SHA256": Tools.calculate_sha256(full_name),
                        "ctime": str(datetime.datetime.fromtimestamp(os.path.getctime(full_name))),
                        "utime": str(datetime.datetime.fromtimestamp(os.path.getmtime(full_name))),
                    })
                self.api.set_description(entry_id, desc)
            except FJError as e:
                print(f"Failed to upload {full_name}: {e}")

        self.create_folders(files_list)
        self.read_directory_tree(None)
        folders_list = {item["ppath"]: item["id"] for item in self.all_fj_folders if item["type"] == "folder"}
        for index, file_info in enumerate(files_list):
            percent_callback(None, 0, 0, index, len(files_list))
            write_file(file_info)
            if Globals.should_stop:
                logger.info("File upload stopped by user.")
                break

    def read_directory_tree(self, local_path):
        """
        Returns the list of fj_files.

        :return: List of fj_files or an empty list if not set.
        :rtype: list
        """
        fj_entries = []
        fj_folders = []
        dj_all_folders = {int(item["id"]):item["name"] for item in self.all_fj_folders}
        for entry in self.all_fj_entries:
            if local_path == "0" or local_path in entry.get('path').split("/"):
                path = entry.get('path', '')
                _p = path.split("/")
                _pp = list()
                for _e in _p:
                    if _e != str(entry["id"]):
                        if int(_e) in dj_all_folders:
                            _pp.append(dj_all_folders[int(_e)])
                        else:
                            _pp.append(_e)
                _pp.append(entry["name"])
                ppath = "\\".join(_pp)
                name = entry.get('name', '')
                ctime = entry.get('created_at', '')
                mtime = entry.get('updated_at', '')
                size = entry.get('file_size', '')
                sha256 = ""
                desc = entry.get('description', '')
                if desc:
                    try:
                        desc_json = json.loads(desc)
                        sha256 = desc_json.get("SHA256", "")
                        ctime = desc_json.get("ctime", "")
                        mtime = desc_json.get("utime", "")
                    except Exception:
                        pass
                entry_to_add = {
                    "path": path,
                    "ppath":ppath,
                    "name": name,
                    "ctime": ctime,
                    "mtime": mtime,
                    "size": size,
                    "sha256": sha256,
                    "id": entry.get("id"),
                }
                if entry.get('type') != 'folder':\
                    fj_entries.append(entry_to_add)
                else:
                    fj_folders.append(entry_to_add)
        return fj_entries, fj_folders

    def get_folder_dialog(self, parent):
        """
        Opens a folder selection dialog and returns the selected folder path.

        :param parent: Parent widget for the dialog
        :return: Selected folder path or None if canceled
        """
        folder_name = None
        files, folders = self.read_directory_tree("0")
        f_names = ["root (id: 0)"] + sorted([f"{item['ppath']} (id: {item['id']})" for item in folders])
        selected, ok = QInputDialog.getItem(
            parent,
            "Select FileJump Directory",
            "Choose a directory from FileJump:",
            f_names,
            0,
            False
        )
        if ok and selected:
            idx = f_names.index(selected)
            folder_name = f_names[idx]
        return folder_name

    def init(self):
        """
        Initializes the FileJumpApi instance by reading the directory tree.
        This method should be called before using other methods to ensure the
        file entries are loaded.
        """
        entries = self.all_fj_entries
        logger.info("No entries found in FileJump.")
