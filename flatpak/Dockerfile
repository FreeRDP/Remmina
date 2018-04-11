FROM fedora:latest
RUN dnf install -yq flatpak flatpak-builder xz
RUN adduser -U -m user
USER user
WORKDIR /home/user
ADD --chown=user:user . .
RUN flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
CMD ["/bin/bash", "-xe", "flatpak.sh"]
