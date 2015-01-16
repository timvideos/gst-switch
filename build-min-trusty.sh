#!/bin/bash -ex

sudo apt-get install \
    build-essential \
    libglib2.0-dev \
    libgtk-3-dev \
    libgstreamer1.0-dev \
    libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-tools \
    autoconf \
    libtool



./autogen.sh
make

