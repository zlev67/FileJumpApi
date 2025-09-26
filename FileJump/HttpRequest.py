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
import requests                                      # pylint: disable=E0401




class HttpRequest:
    # pylint: disable=R0913
    def __init__(self, url, headers=None, timeout=1000):
        """
        Create HTTP request sender

        :param url:
        :param headers:
        :param verify:
        :param timeout:
        """
        self.url = url
        verify = os.path.join(os.path.dirname(__file__), 'filejump.pem')
        self.verify = verify
        self.timeout = timeout
        self.headers = headers

    def put_request(self, data):
        """

        :param data:
        :return:
        """
        response = requests.put(
            self.url,
            headers=self.headers,
            data=data,
            verify=self.verify,
            timeout=self.timeout,
        )
        return response

    def delete_request(self, data=None):
        """

        :return:
        """
        response = requests.delete(
            self.url,
            data=data,
            headers=self.headers,
            verify=self.verify,
            timeout=self.timeout
        )
        return response

    def get_request(self, data=None, params=None):
        """

        :param data:
        :param params:
        :return:
        """
        response = requests.get(
            self.url,
            headers=self.headers,
            data=data,
            params=params,
            verify=self.verify,
            allow_redirects=True,
            timeout=self.timeout
        )
        return response

    def post_request(self, data=None, *, files=None):
        """
        wrapper to requests.post

        :param data: data to post
        :param compress: indicates if data should be zipped on post
        :param files: possible stream posting (files)
        :return:
        """
        response = requests.post(
            self.url,
            headers=self.headers,
            data=data,
            files=files,
            verify=self.verify,
            timeout=self.timeout
        )
        return response

