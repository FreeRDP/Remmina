#!/bin/bash
# Remmina - The GTK+ Remote Desktop Client
# Copyright (C) 2017-2018 Marco Trevisan
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA  02110-1301, USA.
#
#   In addition, as a special exception, the copyright holders give
#   permission to link the code of portions of this program with the
#   OpenSSL library under certain conditions as described in each
#   individual source file, and distribute linked combinations
#   including the two.
#   You must obey the GNU General Public License in all respects
#   for all of the code used other than OpenSSL.#   If you modify
#   file(s) with this exception, you may extend this exception to your
#   version of the file(s), but you are not obligated to do so.#   If you
#   do not wish to do so, delete this exception statement from your
#   version.#   If you delete this exception statement from all source
#   files in the program, then also delete it here.
#
#
set -xe

TRAVIS_BUILD_STEP="$1"
DOCKER_BUILDER_NAME='builder'

if [ -z "$TRAVIS_BUILD_STEP" ]; then
    echo "No travis build step defined"
    exit 0
fi

function docker_exec() {
    docker exec -i $DOCKER_BUILDER_NAME $*
}

if [ "$BUILD_TYPE" == "deb" ]; then
    if [ "$TRAVIS_BUILD_STEP" == "before_install" ]; then
        sudo apt-add-repository $DEB_PPA -y
        sudo apt-get update -q
        sudo apt-get install -y devscripts equivs
    elif [ "$TRAVIS_BUILD_STEP" == "install" ]; then
        sudo mk-build-deps -ir remmina
        if [ -n "$DEB_EXTRA_DEPS" ]; then
            sudo apt-get install -y $DEB_EXTRA_DEPS
        fi
    elif [ "$TRAVIS_BUILD_STEP" == "script" ]; then
        git clean -f
        mkdir $BUILD_FOLDER
        cmake -B$BUILD_FOLDER -H. $DEB_BUILD_OPTIONS
        make VERBOSE=1 -C $BUILD_FOLDER
    fi
elif [ "$BUILD_TYPE" == "cmake" ]; then
	echo "TRAVIS_EVENT_TYPE=" $TRAVIS_EVENT_TYPE
    if [ "$TRAVIS_BUILD_STEP" == "before_install" ]; then
		# We use our freerdp-daily PPA to get freerdp precompiled packages
		# travis builds are for ubuntu trusty 14.04
		sudo apt-add-repository $FREERDP_DAILY_PPA -y
        sudo apt-get update -qq
        sudo apt-get install -y build-essential git-core cmake \
			libssl-dev libx11-dev libxext-dev libxinerama-dev \
			libxcursor-dev libxdamage-dev libxv-dev libxkbfile-dev libasound2-dev libcups2-dev libxml2 libxml2-dev \
			libxrandr-dev libgstreamer0.10-dev libgstreamer-plugins-base0.10-dev \
			libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev libxi-dev libavutil-dev \
			libavcodec-dev libxtst-dev libgtk-3-dev libgcrypt11-dev libssh-dev libpulse-dev \
			libvte-2.90-dev libxkbfile-dev libfreerdp-dev libtelepathy-glib-dev libjpeg-dev \
			libgnutls-dev libgnome-keyring-dev libavahi-ui-gtk3-dev libvncserver-dev \
			libappindicator3-dev intltool libsecret-1-dev libwebkit2gtk-3.0-dev \
			libsoup2.4-dev libjson-glib-dev \
			libfreerdp-dev
    elif [ "$TRAVIS_BUILD_STEP" == "script" ]; then
        git clean -f
        mkdir $BUILD_FOLDER
        cmake -B$BUILD_FOLDER -H. $CMAKE_BUILD_OPTIONS
        make VERBOSE=1 -C $BUILD_FOLDER
    fi
else
    echo 'No $BUILD_TYPE defined'
    exit 1
fi
