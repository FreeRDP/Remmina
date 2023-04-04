if ! type podman > /dev/null; then
        if ! type docker > /dev/null; then
                echo "Install podman or docker to use the remmina environments!"
                exit 1
        fi
        CRT=docker
else    
        CRT=podman
fi

CONTAINER_NAME="vncserver-ssh-socket-forward"

$CRT stop "${CONTAINER_NAME}"
$CRT rm "${CONTAINER_NAME}"

$CRT build . -t remmina/test-server:latest

rm -f "${PWD}/vnc-server-tmp/vnc.socket"
rm -f "${PWD}/vnc-server-tmp/forwarded.socket"

$CRT run -d -p 2222:2222 -v "${PWD}/ssh_key.pub:/root/.ssh/authorized_keys:rw" -v "${PWD}/vnc-server-tmp:/vnc-server-tmp:rw" --rm --name "${CONTAINER_NAME}" -t remmina/test-server:latest

sleep 5

ssh -i "${PWD}/ssh_key" -o "LocalForward=${PWD}/vnc-server-tmp/forwarded.socket /vnc-server-tmp/vnc.socket" -o StreamLocalBindUnlink=yes -o ExitOnForwardFailure=yes  root@localhost -p 2222
