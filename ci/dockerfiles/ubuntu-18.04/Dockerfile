FROM ubuntu:18.04

LABEL description="Bootstrap image used to build and test Remmina with Ubuntu 18.04" \
                   maintainer="Antenore Gatta <antenore@simbiosi.org>" \
                   vendor="Remmina Project" \
                   name="org.remmina.Remmina.images.ubuntu-18.04"

# Set noninteractive
ENV DEBIAN_FRONTEND noninteractive

RUN \
    LC_ALL=en_US.UTF-8 apt-get update -qq \
    && apt-get install -y -qq software-properties-common python3-software-properties \
    && apt-add-repository ppa:remmina-ppa-team/remmina-next-daily -y \
    && apt-add-repository ppa:alexlarsson/flatpak -y \
    && apt-get update -qq \
    && apt-get install -y -qq flatpak-builder flatpak build-essential git-core \
    cmake curl freerdp2-dev intltool libappindicator3-dev libasound2-dev \
    libavahi-ui-gtk3-dev libavcodec-dev libavresample-dev libavutil-dev \
    libcups2-dev libgcrypt11-dev libgnome-keyring-dev libgnutls28-dev \
    libgstreamer-plugins-base1.0-dev libgstreamer1.0-dev libgtk-3-dev \
    libjpeg-dev libjson-glib-dev libpcre2-8-0 libpcre2-dev libpulse-dev \
    libsecret-1-dev libsodium-dev libsoup2.4-dev libspice-client-gtk-3.0-dev \
    libspice-protocol-dev libssh-dev libssl-dev libsystemd-dev \
    libtelepathy-glib-dev libvncserver-dev libvte-2.91-dev libwebkit2gtk-4.0-dev \
    libx11-dev libxcursor-dev libxdamage-dev libxext-dev libxi-dev \
    libxinerama-dev libxkbfile-dev libxkbfile-dev libxml2 libxml2-dev \
    libxrandr-dev libxtst-dev libxv-dev python3 python3-dev wget \
    && apt-get autoremove -y \
    && apt-get clean -y
