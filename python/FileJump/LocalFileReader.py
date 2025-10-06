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

from FileJump.FileOperations import IFileReader


class LocalFileReader(IFileReader):
    """
    A file reader implementation that provides a file-like interface for reading local files.

    This class implements the IFileReader interface and provides binary file reading capabilities
    with support for seeking, size calculation, and context management. It's designed to work
    with multipart uploads and other scenarios requiring file-like objects.

    The class maintains the current file position and provides the remaining bytes count,
    making it suitable for upload progress tracking and multipart encoding.

    Attributes:
        fp (file object): The underlying file pointer opened in binary read mode
        _size (int): Total size of the file in bytes

    Example:
        Basic usage:
        >>> reader = LocalFileReader("/path/to/file.txt")
        >>> data = reader.read(1024)  # Read first 1024 bytes
        >>> reader.close()

        Context manager usage (recommended):
        >>> with LocalFileReader("/path/to/file.txt") as reader:
        ...     data = reader.read()
    """
    def __init__(self, full_name):
        """
        Initialize the LocalFileReader with a file path.

        Opens the specified file in binary read mode and calculates its total size.
        The file pointer is positioned at the beginning of the file.

        Args:
          full_name (str): Full path to the file to be read

        Raises:
          FileNotFoundError: If the specified file doesn't exist
          PermissionError: If the file cannot be opened for reading
          OSError: If there's an I/O error accessing the file

        Note:
          The file is opened immediately upon initialization. Remember to call
          close() or use the class as a context manager to ensure proper cleanup.
        """
        self.fp = open(full_name , "rb")
        self.fp.seek(0, os.SEEK_SET)  # Move to the end of the file to get size
        self._size = os.path.getsize(full_name)

    def read(self, size=-1):
        """
        Read data from the file.

        Reads up to 'size' bytes from the file. If size is -1 or not specified,
        reads the entire remaining content of the file.

        Args:
            size (int, optional): Number of bytes to read. If -1 (default),
                                reads all remaining content.

        Returns:
            bytes: The data read from the file. May be shorter than requested
                  if end of file is reached.

        Example:
            >>> reader = LocalFileReader("example.txt")
            >>> chunk = reader.read(1024)  # Read up to 1024 bytes
            >>> remaining = reader.read()  # Read rest of file
            >>> reader.close()
        """
        if size == -1:
            return self.fp.read()

        read_res =  self.fp.read(size)

        return read_res

    def seek(self, pos, whence=0):
        """
        Move the file pointer to a specific position.

        Changes the current position of the file pointer. The position is calculated
        relative to the reference point specified by 'whence'.

        Args:
            pos (int): Offset in bytes to move to
            whence (int, optional): Reference point for the position:
                - 0 (os.SEEK_SET): Beginning of file (default)
                - 1 (os.SEEK_CUR): Current position
                - 2 (os.SEEK_END): End of file

        Example:
            >>> reader = LocalFileReader("example.txt")
            >>> reader.seek(100)        # Move to byte 100 from start
            >>> reader.seek(50, 1)      # Move 50 bytes forward from current position
            >>> reader.seek(-10, 2)     # Move to 10 bytes before end of file
        """
        self.fp.seek(pos, whence)

    def tell(self):
        """
        Get the current file position.

        Returns the current position of the file pointer in bytes from the
        beginning of the file.

        Returns:
            int: Current position in bytes (0-based)

        Example:
            >>> reader = LocalFileReader("example.txt")
            >>> reader.read(100)
            >>> position = reader.tell()  # Returns 100
        """
        return self.fp.tell()

    def close(self):
        """
        Close the file and release system resources.

        Closes the underlying file pointer and sets it to None to prevent
        further operations. It's safe to call this method multiple times.

        Note:
            After calling close(), any further read operations will fail.
            Consider using the class as a context manager to ensure automatic cleanup.

        Example:
            >>> reader = LocalFileReader("example.txt")
            >>> data = reader.read()
            >>> reader.close()  # Always close when done
        """
        if self.fp:
            self.fp.close()
        self.fp = None

    def __len__(self):
        """
        Return the number of bytes remaining to be read.

        Calculates the remaining bytes from the current file position to the end
        of the file. This is particularly useful for multipart encoders that need
        to know the content length for HTTP headers.

        Returns:
            int: Number of bytes remaining from current position to end of file

        Note:
            This returns remaining bytes, not total file size. For total file size,
            the file size is calculated during initialization.

        Example:
            >>> reader = LocalFileReader("example.txt")  # File is 1000 bytes
            >>> len(reader)  # Returns 1000
            >>> reader.read(300)
            >>> len(reader)  # Returns 700 (1000 - 300)
        """
        return self._size - self.fp.tell()

    def __enter__(self):
        """
        Context manager entry point.

        Returns the instance itself, allowing the class to be used with
        'with' statements for automatic resource management.

        Returns:
            LocalFileReader: Self reference for method chaining

        Example:
            >>> with LocalFileReader("example.txt") as reader:
            ...     data = reader.read()
            # File is automatically closed when exiting the 'with' block
        """
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        """
        Context manager exit point.

        Ensures the file is properly closed when exiting a 'with' statement,
        even if an exception occurs within the context.

        Args:
            exc_type: Exception type (if an exception occurred)
            exc_val: Exception value (if an exception occurred)
            exc_tb: Exception traceback (if an exception occurred)

        Note:
            This method is called automatically when exiting a 'with' statement.
            It ensures proper cleanup regardless of whether the context exits
            normally or due to an exception.
        """
        """Context manager exit - ensures file is closed"""
        self.close()


