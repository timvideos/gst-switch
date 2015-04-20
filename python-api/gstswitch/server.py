"""
The server deals with all operations controlling gst-switch-srv
These include all OS related tasks
"""

from __future__ import absolute_import, print_function, unicode_literals

from six import string_types
import os
import sys
import time
import select
import signal
import subprocess
from distutils import spawn
import logging

from errno import ENOENT
from .exception import PathError, ServerProcessError
from .exception import MatchTimeoutError, MatchEofError, SelectError


__all__ = ["Server", ]


TOOLS_DIR = '/'.join(os.getcwd().split('/')[:-1]) + '/tools/'


class ProcessMonitor(subprocess.Popen):
    """Runs a Command in a Background-Thread and monitors it's output

    Can block until the Command prints a certain string and log the full
    output into a file
    """

    def __init__(self, cmd, logtarget=sys.stderr):
        self.log = logging.getLogger('server-output-monitor')

        # Logfile to write to
        self._logtarget = logtarget

        # Internal Buffer to search for matched when wait_for_output is called
        self._buffer = ""

        self.log.debug("starting subprocess")

        try:
            super(ProcessMonitor, self).__init__(
                cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                bufsize=0,
                shell=False)
        except Exception as err:
            raise ServerProcessError(err)

        self.log.debug("subprocess successfully started")

    def terminate(self):
        """Kills the process and waits for the thread to exit"""

        self.log.info("reading remaining data from subprocess")
        while True:
            (read, _, _) = select.select([self.stdout], [], [], 0)
            if self.stdout not in read:
                break

            self.log.debug("reading data from subprocess")
            chunk = os.read(self.stdout.fileno(), 2000).decode('utf-8')

            if len(chunk) == 0:
                break

            self.log.debug("read %d bytes, appending to logtarget", len(chunk))
            self._logtarget.write(chunk)

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
            (read, _, _) = select.select([self.stdout], [], [], timeout)
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

            self.log.debug("reading data from subprocess")
            chunk = os.read(self.stdout.fileno(), 2000).decode('utf-8')

            if len(chunk) == 0:
                raise MatchEofError("Subprocess died while waiting for match "
                                    "'%s' %dx in the subprocess output."
                                    % (match, count,))

            self.log.debug("read %d bytes, appending to buffer", len(chunk))
            self._buffer += chunk
            self._logtarget.write(chunk)

            self.log.debug("testing again for %dx '%s' in buffer",
                           count, match)
            if self._buffer.count(match) >= count:
                self.log.debug("match found, returning")
                return


class Server(object):

    """Control all server related operations

    :param path: Path where the executable gst-switch-srv
    is located. Provide the full path.
    By default looks in the current $PATH.
    :param video_port: The video port number - default = 3000
    :param audio_port: The audio port number - default = 4000
    :param controller_address: The DBus-Address for remote control -
        default = tcp:host=0.0.0.0,port=5000
    :param record_file: The record file format
    :param video_format: The video format to use on the server.
    :returns: nothing
    """

    def __init__(
            self,
            path=None,
            video_port=3000,
            audio_port=4000,
            controller_address='tcp:host=0.0.0.0,port=5000',
            record_file=False,
            video_format=None,
            log_to_file=True):

        super(Server, self).__init__()

        self.log = logging.getLogger('server')

        self._path = None
        self._video_port = None
        self._audio_port = None
        self._controller_address = None
        self._record_file = None
        self.gst_option_string = ''

        self.path = path
        self.video_port = video_port
        self.audio_port = audio_port
        self.controller_address = controller_address
        self.record_file = record_file
        self.video_format = video_format

        self.log_to_file = log_to_file

        self.proc = None
        self.pid = None

    @property
    def path(self):
        """Get the path"""
        return self._path

    @path.setter
    def path(self, path):
        """Set path
        :raises ValueError: Path cannot be left blank
        """
        self._path = path

    @property
    def video_port(self):
        """Get the video port"""
        return self._video_port

    @video_port.setter
    def video_port(self, video_port):
        """Set Video Port
        :raises ValueError: Video Port cannot be left blank
        :raises ValueError: Video Port must be in range 1 to 65535
        :raises TypeError: Video Port must be a string or a number
        """
        if not video_port:
            raise ValueError("Video Port '{0}' cannot be blank"
                             .format(video_port))
        else:
            try:
                i = int(video_port)
                if i < 1 or i > 65535:
                    raise ValueError('Video Port must be in range 1 to 65535')
                else:
                    self._video_port = video_port
            except TypeError:
                raise TypeError("Video Port must be a string or a number, "
                                "not '{0}'".format(type(video_port)))

    @property
    def audio_port(self):
        """Get the audio port"""
        return self._audio_port

    @audio_port.setter
    def audio_port(self, audio_port):
        """Set Audio Port
        :raises ValueError: Audio Port cannot be left blank
        :raises ValueError: Audio Port must be in range 1 to 65535
        :raises TypeError: Audio Port must be a string or a number
        """
        if not audio_port:
            raise ValueError("Audio Port '{0}' cannot be blank"
                             .format(audio_port))
        else:
            try:
                i = int(audio_port)
                if i < 1 or i > 65535:
                    raise ValueError('Audio Port must be in range 1 to 65535')
                else:
                    self._audio_port = audio_port
            except TypeError:
                raise TypeError("Audio Port must be a string or a number,"
                                " not '{0}'".format(type(audio_port)))

    @property
    def controller_address(self):
        """Get the control port"""
        return self._controller_address

    @controller_address.setter
    def controller_address(self, controller_address):
        """Set Control Address
        :raises ValueError: Control Address cannot be left blank
        :raises ValueError: Control Address must contain at least one Colon
        :raises TypeError: Control Address must be a string
        """
        if not controller_address:
            raise ValueError("Control Address '{0}' cannot be blank"
                             .format(controller_address))
        else:
            if not isinstance(controller_address, string_types):
                raise TypeError("Control Address must be a string,"
                                " not '{0}'".format(type(controller_address)))

            try:
                controller_address.index(':')
                self._controller_address = controller_address
            except ValueError:
                raise ValueError("Control Address must contain at least "
                                 " one Colon. It is '{0}'"
                                 .format(controller_address))

    @property
    def record_file(self):
        """Get the record file"""
        return self._record_file

    @record_file.setter
    def record_file(self, record_file):
        """Set Record File
        :raises ValueError: Non-string file format
        :raises ValueError: Record File cannot have forward slashes
        """
        if record_file is False:
            self._record_file = False
        elif record_file is True:
            self._record_file = True
        else:
            if not record_file:
                raise ValueError("Record File: '{0}' "
                                 "Non-string file format".format(record_file))
            else:
                rec = str(record_file)
                if rec.find('/') < 0:
                    self._record_file = rec
                else:
                    raise ValueError("Record File: '{0}' "
                                     "cannot have forward slashes".format(rec))

    def run(self, gst_option=''):
        """Launch the server process

        :param: None
        :gst-option: Any gstreamer option.
        Refer to http://www.linuxmanpages.com/man1/gst-launch-0.8.1.php#lbAF.
        Multiple can be added separated by spaces
        :returns: nothing
        :raises IOError: Fail to open /dev/null (os.devnull)
        :raises PathError: Unable to find gst-switch-srv at path specified
        :raises ServerProcessError: Running gst-switch-srv
        gives a OS based error.
        """
        self.gst_option_string = gst_option
        self.log.debug("Starting server")
        self.proc = self._run_process()
        if self.proc:
            self.pid = self.proc.pid

    def is_alive(self):
        """Returns True if the Process did not yet return"""
        return self.proc.poll() is None

    def wait_for_output(self, match, timeout=5, count=1):
        """Calls wait_for_output with the given parameters on the underlying
        ProcessMonitor"""
        self.proc.wait_for_output(match, timeout, count)

    def _run_process(self):
        """Non-public method: Runs the gst-switch-srv process
        """
        cmd = ['']
        if not self.path:
            srv_location = spawn.find_executable('gst-switch-srv')
            if srv_location:
                cmd[0] = srv_location
            else:
                raise PathError("Cannot find gst-switch-srv in $PATH.\
                    Please specify the path.")
        else:
            cmd[0] += os.path.join(self.path, 'gst-switch-srv')
        if self.gst_option_string:
            cmd += [self.gst_option_string]
        cmd.append("--video-input-port={0}".format(self.video_port))
        cmd.append("--audio-input-port={0}".format(self.audio_port))
        cmd.append("--controller-address={0}".format(self.controller_address))
        if self.record_file is False:
            pass
        elif self.record_file is True:
            cmd.append("-r")
        else:
            if self.record_file is not False:
                cmd.append("--record={0}".format(self.record_file))

        if self.video_format is not None:
            cmd.append("--video-format={0}".format(self.video_format))

        proc = self._start_process(cmd)
        return proc

    def _start_process(self, cmd):
        """Non-public method: Start a process

        :param cmd: The command which needs to be executed
        :returns: process created
        """
        self.log.info('Starting process %s', cmd)
        try:
            if self.log_to_file:
                process = ProcessMonitor(cmd, open('server.log', 'w'))
            else:
                process = ProcessMonitor(cmd)

            return process

        except OSError as error:
            if error.errno == ENOENT:
                raise PathError("Cannot find gst-switch-srv at path:"
                                " '{0}'".format(self.path))
            else:
                raise ServerProcessError("Internal error "
                                         "while launching process")

    @classmethod
    def make_coverage(cls):
        """Generate coverage
        Calls 'make coverage' to generate coverage in .gcov files
        """
        cmd = 'make -C {0} coverage'.format(TOOLS_DIR)
        print('Starting process %s' % (cmd))
        with open(os.devnull, 'w'):
            proc = subprocess.Popen(
                cmd.split(),
                bufsize=-1,
                shell=False)
            out, _ = proc.communicate()
            print(out)

    def terminate(self, cov=False):
        """Terminate the server.
        self.proc is made None on success

        :param: None
        :returns: True when success
        :raises ServerProcessError: Process does not exist
        :raises ServerProcessError: Cannot terminate process. Try killing it
        """
        self.log.debug('Killing server')
        proc = self.proc
        if proc is None:
            raise ServerProcessError('Server Process does not exist')
        else:
            try:
                if cov:
                    self.gcov_flush()
                    self.make_coverage()
                self.log.info('Killing server')
                proc.terminate()
                self.proc = None
                return True
            except OSError:
                raise ServerProcessError("Cannot terminate server process. "
                                         "Try killing it")

    def kill(self, cov=False):
        """Kill the server process by sending signal.SIGKILL
        self.proc is made None on success

        :param: None
        :returns: True when success
        :raises ServerProcessError: Process does not exist
        :raises ServerProcessError: Cannot kill process
        """
        if self.proc is None:
            raise ServerProcessError('Server Process does not exist')
        else:
            try:
                if cov:
                    self.gcov_flush()
                    self.make_coverage()
                os.kill(self.pid, signal.SIGKILL)
                self.proc = None
                return True
            except OSError:
                raise ServerProcessError('Cannot kill process')

    def gcov_flush(self):
        """Generate gcov coverage by sending the signal SIGUSR1
        The generated gcda files are dumped in tools directory.
        Does not kill the process

        :param: None
        :returns: True when success
        :raises ServerProcessError: If Server is not running
        :raises ServerProcessError: Unable to send signal
        """

        if self.proc is None:
            raise ServerProcessError('Server process does not exist')
        else:
            try:
                self.log.debug("Signaling GCOV Flush to %s", self.pid)
                self.proc.send_signal(signal.SIGUSR1)
                return True
            except OSError:
                raise ServerProcessError('Unable to send signal')
