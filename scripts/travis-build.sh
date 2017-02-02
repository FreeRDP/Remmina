#!/bin/bash
# Remmina - The GTK+ Remote Desktop Client
# Copyright (C) 2014-2017 Marco Trevisan
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
        apt-add-repository $DEB_PPA -y
        apt-get update -q
        apt-get install -y devscripts equivs
    elif [ "$TRAVIS_BUILD_STEP" == "install" ]; then
        mk-build-deps -ir remmina
        apt-get install -y libspice-protocol-dev libspice-client-gtk-3.0-dev
    elif [ "$TRAVIS_BUILD_STEP" == "script" ]; then
        git clean -f
        mkdir build
        cd build
        cmake -DCMAKE_BUILD_TYPE=Debug -DWITH_SSE2=ON -DWITH_APPINDICATOR=OFF --build=build ..
        make VERBOSE=1
    fi
elif [ "$BUILD_TYPE" == "snap" ]; then
    if [ "$TRAVIS_BUILD_STEP" == "before_install" ]; then
        if [ -n "$ARCH" ]; then DOCKER_IMAGE="$ARCH/$DOCKER_IMAGE"; fi
        docker run --name $DOCKER_BUILDER_NAME -v $PWD:$PWD -w $PWD -td $DOCKER_IMAGE
    elif [ "$TRAVIS_BUILD_STEP" == "install" ]; then
        docker_exec apt-get update -q
        docker_exec apt-get install cmake git-core snapcraft -y
    elif [ "$TRAVIS_BUILD_STEP" == "script" ]; then
        git clean -f
        mkdir build
        docker_exec cmake -Bbuild -H. -DSNAP_BUILD_ONLY=ON

        # Sometimes the arch isn't properly recognized by snapcraft
        if [ -n "$ARCH" ]; then
            echo -e "architectures:\n  - $ARCH" >> build/snap/snapcraft.yaml
        fi

        make_target='snap'
        if [ -z "$TRAVIS_TAG" ] || [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
            make_target='snap-prime'
        fi

        docker_exec make $make_target -C build
    fi
else
    echo 'No $BUILD_TYPE defined'
    exit 1
fi
