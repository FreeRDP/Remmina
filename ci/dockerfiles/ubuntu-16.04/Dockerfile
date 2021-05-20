FROM ubuntu:16.04

LABEL description="Bootstrap image used to build and test Remmina with Ubuntu 16.04" \
                   maintainer="Antenore Gatta <antenore@simbiosi.org>" \
                   vendor="Remmina Project" \
                   name="org.remmina.Remmina.images.ubuntu-16.04"

# Set noninteractive
ENV DEBIAN_FRONTEND noninteractive

RUN \
    LC_ALL=en_US.UTF-8 apt-get update -qq \
    && apt-get install -y -qq software-properties-common python3-software-properties \
    && apt-add-repository ppa:remmina-ppa-team/remmina-next-daily -y \
    && apt-get update -qq \
    && apt-get install -y -qq build-essential git-core cmake libssl-dev \
    libx11-dev libxext-dev libxinerama-dev libxcursor-dev libxdamage-dev \
    libxv-dev libxkbfile-dev libasound2-dev libcups2-dev libxml2 libxml2-dev \
    libxrandr-dev libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    libxi-dev libavutil-dev libavcodec-dev libxtst-dev libgtk-3-dev \
    libgcrypt11-dev libssh-dev libpulse-dev libvte-2.91-dev libxkbfile-dev \
    libtelepathy-glib-dev libjpeg-dev libgnutls28-dev libgnome-keyring-dev \
    libavahi-ui-gtk3-dev libvncserver-dev libappindicator3-dev intltool \
    libsecret-1-dev libsodium-dev libwebkit2gtk-4.0-dev libsystemd-dev \
    libsoup2.4-dev libjson-glib-dev libavresample-dev freerdp2-dev \
    libspice-protocol-dev libspice-client-gtk-3.0-dev libfuse-dev wget \
    && apt-get autoremove -y \
    && apt-get clean -y
