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

import json
import mimetypes
import os
import datetime
import logging

import requests
from requests_toolbelt.multipart.encoder import MultipartEncoder, MultipartEncoderMonitor

#from fj_sync.Globals import Globals
from .Exceptions import FJError
from .HttpRequest import HttpRequest


logger = logging.getLogger(__name__)


class FileJumpApi:
    """
       FileJumpApi provides a Python interface to the FileJump cloud storage API.

       This class supports authentication, file and folder management, file uploads and downloads,
       and metadata operations. It handles HTTP requests, error management, and progress callbacks
       for file transfers.

       Main features:
       - Login and authentication
       - Folder creation and directory tree reading
       - File upload (with progress callback and cancellation support)
       - File download (with progress callback)
       - File metadata management (description, info)
       - File deletion

       Usage:
           api = FileJumpApi()
           api.login(email, password)
           api.create_folder("new_folder", parent_id)
           api.post_file(file_path, file_reader, relative_path, percent_callback)
           api.read(files_list, target_dir, percent_callback)
           api.delete_files([entry_id])

       Exceptions:
           Raises FJError on API or network errors.
       """
    base_url = None
    token = None

    @staticmethod
    def set_token(token):
        FileJumpApi.token = token

    @staticmethod
    def set_url(url):
        FileJumpApi.base_url = url

    def __init__(self):
        self.cancel = False

    def login(self, email, password, token_name="test_token"):
        """
        Login to FileJump API.

        :return: returned json. Token is set to self.token
        """
        headers = {'Content-Type': 'application/json'}
        url = "auth/login"
        data = {"email": email, "password": password, "token_name": token_name,}
        request = HttpRequest(self.base_url + url, headers=headers)
        response = request.post_request(data=json.dumps(data))
        if response.status_code != 200:
            raise FJError(f"Failed to create folder: {response.text}")
        data = response.json()
        if not data:
            raise FJError("Failed to login to FileJump API: empty response")
        self.token = data.get("user", {}).get("access_token", None)
        return data

    def get_data(self, query):
        """

        :return:
        :rtype:
        """
        headers = {'Content-Type': 'application/json', "Authorization": f'Bearer {self.token}'}
        request = HttpRequest( self.base_url+query, headers=headers)
        response = request.get_request()
        if response.status_code != 200:
            raise FJError(f"Failed to get data from FileJump API: {response.text}")
        data = response.json()
        if not data:
            return None
        return data

    def create_folder(self, folder, parent_id):
        """
        Creates a new folder in FileJump API.

        :param folder:
        :param parent_id:
        :return:
        """
        logger.info("Creating folder: %s at parent ID: %s", folder, str(parent_id))
        url = "folders"
        headers = {
            'Content-Type': 'application/json',
            "Authorization": f'Bearer {self.token}'
        }
        data = {
            "name": folder,
            "parentId": parent_id,
        }
        request = HttpRequest(self.base_url + url, headers=headers)
        response = request.post_request(data=json.dumps(data))
        if response.status_code != 200:
            raise FJError(f"Failed to create folder: {response.text}")
        try:
            resp = response.json()
            return resp
        except json.JSONDecodeError as e:
            logger.error("Failed to decode JSON response: %s", response.text)
            raise FJError(f"Failed to create folder, invalid response: {response.text}") from e

    def post_file(self, file_path, file_reader, relative_path, percent_callback):
        """
        :return:
        :rtype:
        """
        self.cancel = False
        url = "uploads"
        time  = datetime.datetime.now()
        logger.info("Uploading file: %s to relative path: %s, file size %d",
                    file_path, relative_path, os.path.getsize(file_path))

        headers = {'accept': 'application/json',
                   "Authorization": f'Bearer {self.token}',
                   }

        mime_type, _ = mimetypes.guess_type(file_path)
        mime_type = mime_type or "application/octet-stream"
        fields = {
            "file": (file_path, file_reader, mime_type),
        }
        data = {
            "parentId": "null",
            "relativePath": os.path.normpath(relative_path).replace("\\", "/"),
        }
        fields.update(data)

        def create_callback(encoder):
            def callback(monitor):
                if self.cancel:
                    logger.info("File upload stopped by user: %s", file_path)
                    raise FJError("File upload stopped by user")
                if percent_callback:
                    percent_callback.file_progress(file_path, monitor.bytes_read, encoder.len)
            return callback

        _encoder = MultipartEncoder(fields=fields)
        _monitor = MultipartEncoderMonitor(_encoder, create_callback(_encoder))
        headers['Content-Type'] = _monitor.content_type

        timeout = 1000
        out_data = None
        retry = True
        while retry:
            request = HttpRequest( self.base_url+url, headers=headers, timeout=timeout)
            try:
                response = request.post_request(data=_monitor)
                if response.status_code != 201:
                    logger.info("bad response on file upload: %s, %d, %s",
                                file_path, response.status_code, response.text)
                    raise FJError(f"Upload to FileJump returned bad status: {response.text}")
                out_data = response.json()

                retry = False
            except requests.exceptions.Timeout as ex:
                if timeout > 10000:
                    logger.error("File upload caused timeout; giving up after %d seconds", timeout)
                    raise FJError(f"Upload to FileJump timed out after {timeout} seconds") from ex
                timeout = timeout *10
                logger.info("File upload caused timeout; timeout for next retry is %d seconds", timeout)
                logger.exception(ex)
                retry = True
            except Exception as ex:
                if self.cancel:
                    logger.info("File upload stopped by user: %s", file_path)
                    return None
                logger.exception(ex)
                raise FJError(f"Upload to FileJump caused exception") from ex
        if not out_data:
            return None

        logger.info("File uploaded: %s, %d, %d seconds",
                    file_path, os.path.getsize(file_path), (datetime.datetime.now() - time).total_seconds())
        return out_data


    def get_files(self, path_id=None):
        files = list()
        if path_id:
            url = "drive/file-entries?perPage=1000&parentIds={path_id}&workspaceId=0&page={{}}".format(path_id=path_id)
        else:
            url = "drive/file-entries?perPage=1000&workspaceId=0&page={}"
        next_page = 0
        while next_page is not None:
            data = self.get_data(url.format(next_page))
            next_page = data.get("next_page", None)
            files.extend(data.get("data", None))
        return files

    def get_file_info(self, parent_id, entry_id):
        """
        Retrieves detailed information about a file entry by its ID.

        :param entry_id: ID of the file entry to retrieve
        :return: Detailed file entry data from the API
        """
        files_info = self.get_files(parent_id)
        if files_info:
            for file in files_info:
                if file.get("id") == entry_id:
                    return file
        return None

    def get_file(self, entry_id):
        """
        Retrieves a file from FileJump API by its ID.

        :param entry_id: ID of the file entry to retrieve
        :return: File entry data from the API
        """
        url = f"file-entries/{entry_id}"
        headers = {
            'Content-Type': 'application/json',
            "Authorization": f'Bearer {self.token}'
        }
        request = HttpRequest(self.base_url + url, headers=headers)
        response = request.get_request()
        if response.status_code != 200:
            raise FJError(f"Failed to get file entry: {response.text}")
        return response

    def set_description(self, entry_id, description):
        """
        Updates the description of a file entry in FileJump API.

        :param entry_id: ID of the file entry to update
        :param description: New description for the file entry
        :return: Response data from the API
        """
        url = f"file-entries/{entry_id}"
        headers = {
            'Content-Type': 'application/json',
            "Authorization": f'Bearer {self.token}'
        }
        data = {
            "description": description
        }
        request = HttpRequest(self.base_url + url, headers=headers)
        response = request.put_request(data=json.dumps(data))
        if response.status_code != 200:
            raise FJError(f"Failed to update description: {response.text}")
        return response.json()

    def delete_files(self, entry_ids, delete_forever=False):
        """
        Deletes file entries in FileJump API.

        :param entry_ids: List of file entry IDs to delete
        :param delete_forever: Boolean indicating whether to delete files permanently
        :return: Response data from the API
        """
        url = "file-entries/delete"
        headers = {
            'Content-Type': 'application/json',
            "Authorization": f'Bearer {self.token}'
        }
        data = {
            "entryIds": entry_ids,
            "deleteForever": delete_forever
        }
        request = HttpRequest(self.base_url + url, headers=headers)
        response = request.post_request(data=json.dumps(data))
        if response.status_code != 200:
            raise FJError(f"Failed to delete files: {response.text}")
        return response.json()

    def read(self, files_list, target_dir, percent_callback=None):
        """
        Download a list of files from FileJump to the local target directory.
        :param files_list: List of file dicts (must contain 'id' and 'name')
        :param percent_callback progress callback function to report download progress
        :param target_dir: Local directory to save files
        """
        chunk_size = 16384  # 16 MB
        self.cancel = False
        for file in files_list:
            entry_id = file.get("id")
            name = file.get("name")
            if not entry_id or not name:
                continue
            response = self.get_file(entry_id)
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

    def read_directory_tree(self, path_id=None):
        """
        Recursively reads all directories and files from FileJump API.

        :param path_id: ID of the parent directory (optional)
        :return: List of all files and directories
        """
        logger.info(f"Reading directory tree for path_id: {path_id}")
        all_entries = []
        folders = []
        if self.cancel:
            return all_entries, folders
        entries = self.get_files(path_id)
        if not entries:
            return all_entries, folders
        for entry in entries:
            entry['empty'] = True  # Mark empty directories
            if entry.get('type') == 'folder':  # Assuming 'type' indicates file or directory
                folders.append(entry)
                sub_entries, sub_folders = self.read_directory_tree(entry.get('id'))
                if sub_entries:
                    all_entries.extend(sub_entries)
                    entry['empty'] = False
                if sub_folders:
                    folders.extend(sub_folders)
                    entry['empty'] = False
            all_entries.append(entry)
        logger.info(f"Reading directory tree for path_id: {path_id} Done. Found {len(all_entries)} entries.")
        return all_entries, folders

