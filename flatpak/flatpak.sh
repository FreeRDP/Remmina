#!/bin/sh
flatpak-builder --arch=$FLATPAK_ARCH --user --disable-rofiles-fuse --install-deps-from=flathub --sandbox --repo=repo build *.json
