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
from .FileOperations import IFileReader


class FjFileReader(IFileReader):
    """
    A class to read files from the synchronization queue.
    This class should be implemented to provide file reading functionality.
    """

    def __init__(self, fj_api, file_info):
        self.file_info = file_info
        self.position = 0
        entry_id = self.file_info.get("id")
        name = self.file_info.get("name")
        if not entry_id or not name:
            return
        self.response = fj_api.get_file(entry_id)
        self.length = int(self.response.headers.get('content-length', 0))

    def read(self, size=-1):
        """
        Read the file and return its content.
        :return: File content as bytes or string.
        """
        if size == -1:
            self.position = self.length
            return self.response.read()
        self.position += size
        return self.response.read(size)

    def seek(self, pos, whence=0):
        """
        Move the file pointer to a specific position.
        :param pos: Position to move to.
        :param whence: Reference point for the position (0=beginning, 1=current, 2=end).
        """
        if whence == 0:
            self.position = pos
        elif whence == 1:
            self.position += pos
        elif whence == 2:
            self.position = self.length - pos

    def tell(self):
        """
        Return current position
        :return: Current position in bytes
        """
        return self.position

    def close(self):
        """
        Close the file (optional cleanup)
        """
        pass