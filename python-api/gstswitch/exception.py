"""
All custom exceptions come here
"""

from __future__ import absolute_import, print_function, unicode_literals

import six

__all__ = [
    'BaseError', 'PathError', 'ServerProcessError', 'ConnectionError',
    'ConnectionReturnError', 'RangeError', 'InvalidIndexError',
]


class BaseError(Exception):

    """docstring for BaseError"""
    pass


class PathError(BaseError):

    """docstring for PathError"""
    pass


class ServerProcessError(BaseError):

    """docstring for ServerProcessError"""
    pass

if six.PY2:
    class ConnectionError(BaseError):

        """
        This class is only defined here in Python 2 as there is a builtin with
        the same name in Python 3
        """
        pass
else:
    # Python 3; so just use the builtin
    from builtins import ConnectionError

class ConnectionReturnError(BaseError):

    """docstring for ConnectionReturnError"""
    pass


class RangeError(BaseError):

    """docstring for RangeError"""
    pass


class InvalidIndexError(BaseError):

    """docstring for InvalidIndexError"""
    pass
