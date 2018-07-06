#!/bin/sh -e

: ${FLATPAK_ARCH:=$(flatpak --default-arch)}
FLATHUB_REPO="https://flathub.org/repo/flathub.flatpakrepo"
DBUS_ID="org.remmina.Remmina"


flatpak_builder()
{
    flatpak-builder \
        --user \
        --arch="${FLATPAK_ARCH}" \
        --repo=repo \
        --sandbox \
        "$@"
}

FLATPAK_MANIFEST_DIR=$(dirname "$(readlink -f "$0")")

cd "${FLATPAK_MANIFEST_DIR}"

# Build everything but Remmina module
flatpak_builder \
    --force-clean \
    --install-deps-from=flathub \
    --stop-at=remmina \
    app/ org.remmina.Remmina.json --ccache

# Build Remmina module from local checkout
mkdir -p _flatpak_build

flatpak build --build-dir="${PWD}/_flatpak_build" app/ \
    cmake -G Ninja \
          -DCMAKE_INSTALL_PREFIX:PATH=/app \
          -DCMAKE_BUILD_TYPE:STRING=RelWithDebInfo \
          -DWITH_MANPAGES:BOOL=OFF \
          -DWITH_TELEPATHY:BOOL=OFF \
          ../..

flatpak build app/ \
    ninja -v -C _flatpak_build install

# Finish Flatpak app
flatpak_builder \
    --disable-download \
    --disable-updates \
    --finish-only \
    app/ org.remmina.Remmina.json

flatpak build-bundle repo remmina-dev.flatpak --runtime-repo=${FLATHUB_REPO} ${DBUS_ID}
