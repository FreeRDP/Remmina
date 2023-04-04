FROM ubuntu:latest

RUN apt-get update && apt-get install -y tigervnc-standalone-server openssh-server xvfb xterm
RUN mkdir -p /run/sshd /var/run/sshd /root/.vnc /root/.ssh ; \
    echo "root:root" | chpasswd && \
    echo 'Port 2222' >> /etc/ssh/sshd_config && \
    echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config && \
    echo 'PasswordAuthentication yes' >> /etc/ssh/sshd_config && \
    echo 'PubkeyAuthentication yes' >> /etc/ssh/sshd_config && \
    echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/* /tmp/* /var/tmp/*

EXPOSE 2222

COPY passwd /root/.vnc/passwd

COPY ssh_key /root/.ssh/authorized_keys

COPY entrypoint.sh /entrypoint.sh

CMD [ "/entrypoint.sh" ]
