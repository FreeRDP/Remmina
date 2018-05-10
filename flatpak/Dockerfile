FROM fedora:latest
RUN dnf install -yq flatpak flatpak-builder fuse git
RUN mkdir -m1777 -p /tmp
RUN useradd -U -m remmina
USER remmina
WORKDIR /home/remmina
COPY --chown=remmina:remmina . .
RUN flatpak --user remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
CMD ["/bin/sh", "-xe", "./flatpak/flatpak-build.sh"]
