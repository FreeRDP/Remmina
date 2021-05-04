FROM docker.io/ubuntudesktop/gnome-3-28-1804:latest

LABEL description="Bootstrap image used to build the Remmina snap with" \
                   maintainer="Antenore Gatta <antenore@simbiosi.org>" \
                   vendor="Remmina Project" \
                   name="org.remmina.Remmina.images.gnome-3-28-1804"

# Set noninteractive
ENV DEBIAN_FRONTEND noninteractive

RUN \
    LC_ALL=en_US.UTF-8 apt-get -y update -qq \
    && apt-get autoremove -y \
    && apt-get clean -y
