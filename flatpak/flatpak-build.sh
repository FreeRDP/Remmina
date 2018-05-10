#!/bin/sh -e

flatpak-builder \
    --user \
    --arch="${FLATPAK_ARCH}" \
    --install-deps-from=flathub \
    --repo=repo \
    --sandbox \
    app/ org.remmina.Remmina.json
