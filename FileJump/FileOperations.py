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
from abc import ABCMeta, abstractmethod

class IFileReader(metaclass=ABCMeta):
    """
    A class to read files from the synchronization queue.
    This class should be implemented to provide file reading functionality.
    """
    @abstractmethod
    def read(self, size=-1):
        """
        Read the file and return its content.
        :return: File content as bytes or string.
        """

    @abstractmethod
    def seek(self, pos, whence=0):
        """
        Move the file pointer to a specific position.
        :param pos: Position to move to.
        :param whence: Reference point for the position (0=beginning, 1=current, 2=end).
        """

    @abstractmethod
    def tell(self):
        """
        Return current position

        :return: Current position in bytes
        """

    @abstractmethod
    def close(self):
        """
        Close the file (optional cleanup)
        """


class IFileOperations (metaclass=ABCMeta):
    """
    Interface for file operations required for synchronization with FileJump.

    This interface defines the contract for classes that handle file uploads, downloads,
    deletions, renaming, and directory tree reading. Implementations should provide
    concrete logic for interacting with local or remote file systems as needed by the
    synchronization process.

    Methods:
        write(file_operations, files_list, path, percent_callback):
            Upload a list of files to FileJump, reporting progress via callback.

        delete(file_list):
            Delete files from the synchronization queue.

        read(files_list, target_dir, percent_callback=None):
            Download files from the synchronization queue to a target directory.

        rename(files_list):
            Rename files in the synchronization queue.

        read_directory_tree(local_path):
            Read and return the directory tree from the specified local path.

    Usage:
        class MyFileOperations(IFileOperations):
            def write(self, file_operations, files_list, path, percent_callback):
                # Implementation here

            def delete(self, file_list):
                # Implementation here

            def read(self, files_list, target_dir, percent_callback=None):
                # Implementation here

            def rename(self, files_list):
                # Implementation here

            def read_directory_tree(self, local_path):
                # Implementation here
    """
    @abstractmethod
    def write(self, file_operations, files_list, path, percent_callback):
        """
        Upload a list of files to FileJump.

        :param file_operations: File operations instance to retrieve the file data
        :param files_list: List of local file dicts (must contain 'path' and 'name')
        :param path: Optional parent folder
        :param percent_callback: Callback function to report upload progress
        """

    @abstractmethod
    def delete(self, file_list):
        """
        Delete files from the synchronization queue.
        :return:
        """

    @abstractmethod
    def read(self, files_list, target_dir, percent_callback=None):
        """
        Read a file from the synchronization queue.
        :return: FileReader instance or similar object that can read the file.
        """

    @abstractmethod
    def rename(self, files_list):
        """
        Read a file from the synchronization queue.
        :return:
        """

    @abstractmethod
    def read_directory_tree(self, local_path):
        """
        Read the directory tree from the given path.
        :param local_path: Path to the directory.
        :return: List of files in the directory.
        """
