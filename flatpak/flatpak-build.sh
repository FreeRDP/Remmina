#!/bin/sh -e

flatpak_builder()
{
    flatpak-builder \
        --user \
        --arch="${FLATPAK_ARCH}" \
        --repo=repo \
        --sandbox \
        "$@"
}

# Build everything but Remmina module
flatpak_builder \
    --install-deps-from=flathub \
    --stop-at=remmina \
    app/ flatpak/org.remmina.Remmina.json

# Build Remmina module from local checkout
flatpak build app/ \
    cmake -G Ninja \
          -DCMAKE_INSTALL_PREFIX:PATH=/app \
          -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo \
          -DWITH_MANPAGES:BOOL=OFF \
          -DWITH_TELEPATHY:BOOL=OFF \
          .

flatpak build app/ \
    ninja -v install

# Finish Flatpak app
flatpak_builder \
    --disable-download \
    --disable-updates \
    --finish-only \
    app/ flatpak/org.remmina.Remmina.json
