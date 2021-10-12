# Remmina docker images

To update/push images you need permissions for this repository:

https://gitlab.com/Remmina/Remmina/container_registry/

To build and push:

```shell
# cd where is the Dockerfile
# login with your Gitlab account and a personal access token
docker build -t registry.gitlab.com/remmina/remmina/ubuntu:20.04 --no-cache --force-rm --compress .
docker push registry.gitlab.com/remmina/remmina/ubuntu:20.04
```


