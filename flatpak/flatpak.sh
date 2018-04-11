#!/bin/sh
flatpak-builder --arch=$FLATPAK_ARCH --user --install-deps-from=flathub --sandbox --repo=repo build *.json
