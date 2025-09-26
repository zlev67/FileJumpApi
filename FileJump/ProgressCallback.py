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

class IProgressCallback(metaclass=ABCMeta):
    """
    Interface for reporting progress during file upload or download operations.

    Implement this interface to receive callbacks about the start of a file transfer
    and ongoing progress updates. Useful for displaying progress bars or logging.

    Methods:
        start_file(file_index, file_name, total_files):
            Called when a new file transfer starts.

        file_progress(file_name, uploaded, size):
            Called to report progress of the current file transfer.

    Example:
        class MyProgress(IProgressCallback):
            def start_file(self, file_index, file_name, total_files):
                print(f"Starting {file_name} ({file_index+1}/{total_files})")

            def file_progress(self, file_name, uploaded, size):
                print(f"{file_name}: {uploaded}/{size} bytes uploaded")
    """
    @abstractmethod
    def start_file(self, file_index, file_name, total_files):
        """
        Start file upload

        :param file_index:
        :param file_name:
        :param total_files:
        :return:
        """

    def file_progress(self, file_name, uploaded, size):
        """
        Upload buffer progress

        :param file_name:
        :param uploaded:
        :param size:
        :return:
        """

