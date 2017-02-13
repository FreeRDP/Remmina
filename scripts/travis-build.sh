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
elif [ "$BUILD_TYPE" == "snap" ]; then
    if [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
        if [ "$SNAP_PRIME_ON_PULL_REQUEST" != "true" ]; then
            echo '$SNAP_PRIME_ON_PULL_REQUEST is not set to true, thus we skip this now'
            exit 0
        fi
    fi

    if [ "$TRAVIS_BUILD_STEP" == "before_install" ]; then
        if [ -n "$ARCH" ]; then DOCKER_IMAGE="$ARCH/$DOCKER_IMAGE"; fi
        docker run --name $DOCKER_BUILDER_NAME -v $PWD:$PWD -w $PWD -td $DOCKER_IMAGE
    elif [ "$TRAVIS_BUILD_STEP" == "install" ]; then
        docker_exec apt-get update -q
        docker_exec apt-get install cmake git-core snapcraft -y
    elif [ "$TRAVIS_BUILD_STEP" == "script" ]; then
        git clean -f
        mkdir $BUILD_FOLDER
        docker_exec cmake -B$BUILD_FOLDER -H. -DSNAP_BUILD_ONLY=ON

        make_target='snap'
        if [ -z "$TRAVIS_TAG" ] || [ "$TRAVIS_PULL_REQUEST" != "false" ]; then
            make_target='snap-prime'
        fi

        docker_exec make $make_target -C $BUILD_FOLDER
    elif [ "$TRAVIS_BUILD_STEP" == "after_success" ]; then
        sudo mkdir -p $BUILD_FOLDER/snap/.snapcraft -m 777
        set +x
        openssl aes-256-cbc -K $SNAPCRAFT_CONFIG_KEY \
            -iv $SNAPCRAFT_CONFIG_IV \
            -in snap/.snapcraft/travis_snapcraft.cfg \
            -out $BUILD_FOLDER/snap/.snapcraft/snapcraft.cfg -d
    elif [ "$TRAVIS_BUILD_STEP" == "deploy-unstable" ]; then
        docker_exec make snap-push-$SNAP_UNSTABLE_CHANNEL -C $BUILD_FOLDER
    elif [ "$TRAVIS_BUILD_STEP" == "deploy-release" ]; then
        docker_exec make snap-push-$SNAP_RELEASE_CHANNEL -C $BUILD_FOLDER
    fi
else
    echo 'No $BUILD_TYPE defined'
    exit 1
fi
