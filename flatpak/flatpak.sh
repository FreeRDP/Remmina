#!/bin/sh
flatpak-builder --arch=x86_64 --user --install-deps-from=flathub --sandbox --repo=repo build *.json
flatpak-builder --arch=i386 --user --install-deps-from=flathub --sandbox --repo=repo build *.json
