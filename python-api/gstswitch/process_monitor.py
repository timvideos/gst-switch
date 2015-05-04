"""
The ProcessMonitor runs a command in a background thread and monitors
its output- It can block until the command prints a certain string and
log the full output into a file
"""

from __future__ import absolute_import, print_function, unicode_literals

import subprocess
import logging
import sys
import os
import time
import select

from .exception import ServerProcessError
from .exception import MatchTimeoutError, MatchEofError, SelectError


class ProcessMonitor(subprocess.Popen):
    """Runs a command in a background-thread and monitors its output

    Can block until the command prints a certain string and log the full
    output into a file
    """

    def __init__(self, cmd, cmd_output_target=sys.stderr):
        self.log = logging.getLogger('server-output-monitor')

        # Logfile to write to
        self._cmd_output_target = cmd_output_target

        # Internal Buffer to search for matched when wait_for_output is called
        self._buffer = ""

        self.log.debug("starting subprocess")

        try:
            super(ProcessMonitor, self).__init__(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=4096,
                shell=False)
        except Exception as err:
            raise ServerProcessError(err)

        self.log.debug("subprocess successfully started")

    def terminate(self):
        """Kills the process and waits for the thread to exit"""

        self.log.info("reading remaining data from subprocess")
        while True:
            # select takes three lists of file-descriptors to be monitored:
            #   readable, writeable and exceptional
            # The last argument is the timeout after which select should give
            # up. 0 means, that select should return immediately without
            # blocking (called a "poll")
            (read, _, _) = select.select([self.stdout], [], [], 0)

            # if the processes' stdout is not readable (ie there is nothing
            # to read), we're done and can exit the loop
            if self.stdout not in read:
                break

            # otherwise read as many bytes as possible, up to 2000,
            # from the process. os.read does not wait until exactly 2000
            # bytes have been read but returns as many bytes as possible
            # the only time os.read block is, when there's nothing to read,
            # which we ruled out by the poll-call to select before
            #
            # os.read -in contrast to self.stdout.read- is non-blocking,
            # if at least 1 character is readable.
            self.log.debug("reading data from subprocess")
            chunk = os.read(self.stdout.fileno(), 2000).decode('utf-8')

            if len(chunk) == 0:
                break

            self.log.debug(
                "read %d bytes, appending to cmd_output_target",
                len(chunk))
            self._cmd_output_target.write(chunk)

        self.log.debug("terminating the subprocess")
        super(ProcessMonitor, self).terminate()
        self.log.debug("waiting for the subprocess to die")
        super(ProcessMonitor, self).communicate()

    def wait_for_output(self, match, timeout=5, count=1):
        """Searches the output already captured from the running process for
        match and returns immediatly if match has already been captured.

        Sets up a match-request for the stdout/stderr-reader and blocks until
        match emerges in the processes stdout/stderr but not longer then
        timeout. If no match is found until timeout is passed, a RuntimeError
        is raised.
        """
        if self._buffer.count(match) >= count:
            self.log.debug("match found, returning without reading more data")
            return

        endtime = time.time() + timeout
        while True:
            timeout = endtime - time.time()
            self.log.debug("waiting for data output by subprocess"
                           "(remaining time to timeout = %fs)", timeout)

            # select takes three lists of file-descriptors to be monitored:
            #   readable, writeable and exceptional
            # The last argument is the timeout after which select should give
            # up. If one of the supplied descriptors gets readable within the
            # supplied timeout, the function returns.
            (read, _, _) = select.select([self.stdout], [], [], timeout)

            # if the processed' stdout is not readable (ie there's
            # to read) and select did return nevertheless, so there must have
            # been an exception or a timeout.
            if self.stdout not in read:
                remaining = endtime - time.time()
                if remaining < 0.001:
                    raise MatchTimeoutError(
                        "Timeout while waiting for match "
                        "'%s' %dx in the subprocess output.\n"
                        "re-run tests with -x and look at "
                        "server.log to investigate further"
                        % (match, count,))

                raise SelectError("select returned without stdout being"
                                  " readable, assuming an exception")

            # read as many bytes as possible, up to 2000,
            # from the process. os.read does not wait until exactly 2000
            # bytes have been read but returns as many bytes as possible
            # the only time os.read block is, when there's nothing to read,
            # which we ruled out by the call to select before
            #
            # os.read -in contrast to self.stdout.read- is non-blocking,
            # if at least 1 character is readable.
            self.log.debug("reading data from subprocess")
            chunk = os.read(self.stdout.fileno(), 2000).decode('utf-8')

            if len(chunk) == 0:
                raise MatchEofError("Subprocess died while waiting for match "
                                    "'%s' %dx in the subprocess output."
                                    % (match, count,))

            self.log.debug("read %d bytes, appending to buffer", len(chunk))
            self._buffer += chunk
            self._cmd_output_target.write(chunk)

            self.log.debug("testing again for %dx '%s' in buffer",
                           count, match)
            if self._buffer.count(match) >= count:
                self.log.debug("match found, returning")
                return
