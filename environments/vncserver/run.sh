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
IMAGE_NAME="remmina/environments/vncserver"

$CRT stop "${CONTAINER_NAME}"
$CRT rm "${CONTAINER_NAME}"

$CRT build . -t ${IMAGE_NAME}

$CRT run -p 5555:5555 -v "${PWD}/vnc-server-tmp:/vnc-server-tmp:rw" --rm --name "${CONTAINER_NAME}" -it ${IMAGE_NAME}
