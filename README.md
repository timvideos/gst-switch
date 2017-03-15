----

# gst-switch has been deprecated in favor of [voctomix](https://github.com/voc/voctomix).
# All development effort is now going into [voctomix](https://github.com/voc/voctomix).


----
----
----
----
----
----


# Build Status
[![Build Status](https://travis-ci.org/timvideos/gst-switch.png?branch=master)](https://travis-ci.org/timvideos/gst-switch)
[![Coverage Status](https://coveralls.io/repos/timvideos/gst-switch/badge.png)](https://coveralls.io/r/timvideos/gst-switch)

# gst-switch

gst-switch is a software based live video mixing and capture platform designed
for fast, reliable and fault tolerant mixing, switching and capture. It is
intended to replace expensive hardware mixing solutions needed when mixing HD
video streams.

gst-switch was developed as a HD be a replacement of
[dvswitch](http://dvswitch.alioth.debian.org/wiki/).


# Quick Start

 * Use Ubuntu 14.04 (trusty)
 * Check out code
 * Run `build-min-trusty.sh`
 * Run `run-demo.sh`

----
----

# Design

gst-switch is split into multiple parts.

The video mixing is done in a server process which is controlled via an API
available on DBUS.

Video and Audio is feed to the server using the gstreamer data protocol (GDP)
over TCP sockets. The resulting mixed output is written to disk and can also be
read out of the server again using GDP over a TCP socket.

The UI talks to the server via the API and displays the results. The UI is a
totally separate process (which could run on a different machine) meaning that
any crashes, stalls or other issues do not effect the mixing or recording.

The Python API exists to allow easy scripting on the server. It has
functionality for controlling the starting and stopping of the server and
video/audio feeds.  It also can control the mixing, doing anything that the GUI
can do. The Python API is used extensively for testing, but should also be
useful for semi and full-automated mixing.

![System design diagram](https://docs.google.com/drawings/d/18sU5uECrPgQzxLWzc7RaYWRs9F3aocx0chf16-yxBEY/pub?w=1871&h=999)

# Installing

gst-switch is only **tested** on the current
[Ubuntu LTS](https://wiki.ubuntu.com/LTS).

It **should** work on other Linux versions which include the dependencies (such
as Debian, RedHat, etc) but is not regularly tested on them.

## Dependencies

gst-switch has minimal dependencies when used just for basic video switching.

The requirements are;
 * A current version of [gstreamer1.0](http://gstreamer.freedesktop.org/).
   (Used to do all the actual hard work.)
 * A current version of dbus. (Used to communicate between the server and UI.)
 * A current version of gtk3.0 (Used for the GUI.)

The Python API is highly recommended but optional part of gst-switch. The
Python API requirements are;
 * python-gi (and thus gir1.2-glib-2.0 and libglib2.0-0).
   (Used to call gstreamer and dbus Python bindings.)
 * python-scipy
   (Used to create and manipulate images.)


The test suite requirements are;
 * pytest (including pytest-pep8 and pytest-cov)
 * pylint
 * ffmpeg or avconv command line tools (ffmpeg and libav-tools).
  * *FIXME: These should be removed and replaced with gst-launch commands.*

There are some optional dependencies for the more advanced features like
speaker tracking. *FIXME: Add the dependency information here.*

## Ubuntu Trusty

Either run the `build-min-trusty.sh` script or follow the instructions in the
file.

## Ubuntu Precise

**We are currently in the process of dropping support for Precise in preference
of Trusty.**

Ubuntu Precise does not ship with gstreamer1.0, so it needs to be build from
source. The `scripts/install2.sh` script will do this for you and then builds
gst-switch too.

----
----

# Manual

gst-switch contains two parts, *gst-switch-srv* and *gst-switch-ui*

## The gst-switch UI
The gst-switch UI is a graphical controller for controlling the input streams.
The UI must be started *after* the server is running.

Example gst-switch-ui command line;
```bash
gst-switch-ui
```

### Controls

 * Left Click - Set video as primary video.
 * Right Click - Set video as secondary video.

| Key                   | Function                                     |
| --------------------- | -------------------------------------------- |
| R                     | Start recording to new file                  |
| Tab                   | Cycle composite modes                        |
| Esc                   | No composite mode                            |
| F1 or P               | Compositing mode - Picture-in-Picture        |
| F2 or D               | Compositing mode - Side-by-side (preview)    |
| F3 or S               | Compositing mode - Side-by-side (equal)      |
| A                     | When compositing, change the primary video   |
| B                     | When compositing, change the secondary video |
| Up/Down               | When compositing, select the video           |
| Ctrl + Arrows         | When compositing PIP, Adjust PIP position    |
| Ctrl + Shift + Arrows | When compositing PIP, Adjust PIP size        |


## The gst-switch Server

The gst-switch server will open at least three ports for video/audio input, and
command controls.

Example gst-switch command line:
```bash
gst-switch-srv
```
*FIXME: Add examples of the recording and resolution options to the above
example.*

Once the gst-switch server is running, you will then need to feed the server
audio and video streams. Helper scripts for doing this can be found in the XXXX
directory.

*FIXME: Add example feed information here.*


### Command Line Options

```
Usage:
  gst-switch-srv [OPTION...]

Help Options:
  -h, --help                        Show help options
  --help-all                        Show all help options
  --help-gst                        Show GStreamer Options

Application Options:
  -v, --verbose                     Prompt more messages
  -t, --test-switch=OUTPUT          Perform switch test
  -r, --record=FILENAME             Enable recorder and record into the specified FILENAME
  -p, --video-input-port=NUM        Specify the video input listen port.
  -a, --audio-input-port=NUM        Specify the audio input listen port.
  -c, --controller-address=ADDRESS     Specify DBus-Address for remote control, defaults to tcp:host=::,port=5000.
```

### Video Input

The default TCP port for video data is *3000*.

Video data should be sent using gstreamer but there are multiple methods for
doing so (see the sections below).

All video sent **must** be in the resolution the exact aspect ratio, resolution
and frame rates the server was configured with when starting.

The color space / pixel format must be in [I420](http://www.fourcc.org/yuv.php#IYUV).
(This limitation comes from the usage of the inter gstreamer elements.)

You can use the following gstreamer plug-ins to convert the video on the
sender's side;
 * [videobox](http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good-plugins/html/gst-plugins-good-plugins-videobox.html),
   to add borders or crop a video (thus changing the resolution or aspect ratio).

 * [videoscale](http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-plugins/html/gst-plugins-base-plugins-videoscale.html),
   to change the resolution of video via scaling.

 * [videoconvert](http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-plugins/html/gst-plugins-base-plugins-videoconvert.html),
   to convert the pixel format and color space.

 * [videorate](http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-base-plugins/html/gst-plugins-base-plugins-videorate.html),
   to adjust the frame rate (fps).

All video **must** be deinterlaced, use the following plug-ins to convert
interlaced video on the sender's side;
 * [deinterlace](http://gstreamer.freedesktop.org/data/doc/gstreamer/head/gst-plugins-good-plugins/html/gst-plugins-good-plugins-deinterlace.html),
   the basic, well coded, stable, deinterlacer included in gstreamer-plugins-good.

 * `avdeinterlace`, the high performance and feature rich deinterlacer from
   libav (a fork of ffmpeg). Previously called `ffdeinterlace`.

#### Using `gst-launch-1.0` for video input

*FIXME: Put some example gst-launch-1.0 examples here.*

*FIXME: Write some example scripts which show gst-launch-1.0 usage for capturing
from DV, v4l, decklink cards, etc.*

#### Using gst-switch Python API

*FIXME: Put some Python API examples here.*

#### Using gst-switch-cap for video input

gst-switch-cap is a custom program needed if you wish to use the speaker
tracking functionality.

Example command line;
```bash
gst-switch-cap --device='/dev/ttyUSB0' --protocol='visca'
```

#### Supported video formats

*FIXME: Actually make sure the tests cover the following formats listed below.*

gst-switch is tested with the following video resolution;
 * 300x200   - A low resolution only used in testing to increase test speed.
 * 1024x768  - A 4:3 resolution almost universally supported by computers and
               projector equipment. Sometimes called XGA.
 * 1280x720  - Lowest resolution 16:9 HD format, often called
               [720p](http://en.wikipedia.org/wiki/720p).
 * 1920x1080 - Most common 16:9 HD format, often called
               [1080p](http://en.wikipedia.org/wiki/1080p).
 * 4096x2160 - Ultra HD format with 19:10 (1.9:1) aspect ratio. Sometimes
	       called [4k](http://en.wikipedia.org/wiki/4K_resolution) video or
               2160p. 
               **WARNING: Even the fastest computers struggle at this resolution!**


gst-switch is tested with the following video frame rates;
 * 24fps - Common "film" format.
 * 25fps - "Full" frame rate of TV in PAL based countries (Europe, Australia, etc).
 * 30fps - "Full" frame rate of TV in NTSC based countries (USA, Japan, etc).
 * 60fps - Most common frame rate of computer monitors.
 * 120fps - Frame rate of "High FPS" computer monitors and TVs.


#### Audio Input Port

The default audio input port is *4000*.

Audio input must be in;
```
      audio/x-raw
                 format: S16LE
                   rate: 48000
               channels: 2
                 layout: interleaved
```

##### Example test audio feed

```
gst-launch-1.0 audiotestsrc is-live=true \
        ! audioconvert \
        ! audio/x-raw,rate=48000,channels=2,format=S16LE,layout=interleaved \
        ! gdppay \
        ! tcpclientsink port=4000
```

##### Example real audio feed



#### Control Port

The default command control port is *5000*.

*FIXME: This is a DBUS port of some type?*


# gstreamer plug-ins usage

This section lists the plug-ins used by gst-switch system.

*FIXME: Generate this bit automagically. The following command line is kind of a
start;*
```shell
ack --type=cc "g_string_new|g_string_append" * | grep '"' | grep ".c" | sed -e's/.c:[^"]*"/ /' -e's/"[^"]*$//' | sort | uniq > output
```

### GUI

#### Audio Display

tools/gstaudiovisual - Renders a video of the audio.
 * *FIXME: Is this GUI or server side?*
 * audioconvert
 * videoconvert
 * faad
 * gdpdepay
 * monoscope
 * queue2
 * tcpclientsrc
 * tee
 * textoverlay
 * xvimagesink

tools/gstvideodisp - Displays the video on the screen.
 * cairooverlay
 * gdpdepay
 * tcpclientsrc
 * videoconvert
 * xvimagesink

### Server

tools/gstswitchserver
 * gdppay
 * intervideosrc
 * tcpserversink

tools/gstcase
 * *FIXME: What is this?*
 * faac
 * gdpdepay
 * gdpdepay
 * gdppay
 * giostreamsrc
 * interaudiosink
 * interaudiosrc
 * intervideosink
 * intervideosrc
 * queue2
 * tcpserversink
 * tee

tools/gstcomposite - The actual mixer?
 * *FIXME: I'm sure there is probably more here...*
 * identity
 * intervideosink
 * queue2
 * tee


#### Recording

The output format is an AVI file with VP8 (webm) video and faac audio.

tools/gstrecorder - Recording to disk.
 * avimux
 * voaacenc
 * filesink
 * gdppay
 * interaudiosrc
 * intervideosrc
 * queue2
 * tcpserversink
 * tee
 * vp8enc

### Speaker Track

tools/gstswitchcapture and tools/gstswitchptz

### Tests

Everything under the tests/ directory

 * audiotestsrc
 * avidemux
 * faad
 * fakesink
 * filesrc
 * gdpdepay
 * gdppay
 * goom2k1
 * monoscope
 * tcpclientsrc
 * textoverlay
 * videoconvert
 * videotestsrc
 * xvimagesink


###The gst-switch PTZ

Implements Pan-Tilt-Zoom Camera controller UI.
```bash
gst-switch-ptz --device='/dev/ttyUSB0' --protocol='visca' --video='/dev/video0'
```

*FIXME: Remove this tool and put it into a gst-plugins-ptz repository with the
associated plug-ins.*

# License

This program is free software: you can redistribute it and/or modify it under
the terms of the **GNU General Public License** as published by the Free
Software Foundation, either version 3 of the License, or (at your option) any
later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

Full text of the GNU General Public License can be found in the COPYING file or
at the following URL <http://www.gnu.org/licenses/gpl-3.0.html>.
