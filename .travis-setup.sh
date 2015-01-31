#! /bin/bash -ex

# Update / upgrade everything
sudo apt-get -y update && sudo apt-get -y upgrade

# gst-switch compile stuff
sudo apt-get -y install build-essential dh-autoreconf
sudo apt-get -y install libgtk-3-dev
sudo apt-get -y install gstreamer1.0.* libgstreamer.*1.0.*

# Python stuff for gst-switch API - Python 2.7
sudo apt-get -y install python-software-properties python-pip
sudo apt-get -y install libglib2.0-dev gir1.2-glib-2.0 libgirepository1.0-dev libglib2.0-0 python-gi
sudo apt-get -y install python-scipy
sudo pip install --upgrade -r requirements.txt

# Python stuff for gst-switch API - Python 3.4
sudo apt-get -y install python3-software-properties python3-pip
sudo apt-get -y install libglib2.0-dev gir1.2-glib-2.0 libgirepository1.0-dev libglib2.0-0 python3-gi
sudo apt-get -y install python3-scipy
sudo pip3 install --upgrade -r requirements.txt

# Needed for tests
sudo apt-get -y install wget libav-tools

# Needed for style checks
sudo apt-get -y install indent

# Needed to get coverage output
sudo apt-get -y install libyaml-dev
if [[ $TEST == "server" ]]; then
	sudo pip install cpp-coveralls --upgrade
else
	sudo pip install python-coveralls --upgrade
fi

# Start an X server so the GUI works
sudo apt-get -y install xvfb xclip
export DISPLAY=:99.0
/sbin/start-stop-daemon --start --quiet --pidfile /tmp/custom_xvfb_99.pid --make-pidfile --background --exec /usr/bin/Xvfb -- :99 -ac -screen 0 1024x768x16
