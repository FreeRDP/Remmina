# Remmina docker images

To update/push images you need permissions for this repository:

https://cloud.docker.com/u/remmina/repository/docker/remmina/ubuntu

To build and push:

```shell
# cd where is the Dockerfile
# login with your docker account
docker build -t remmina/ubuntu:18.04 --no-cache --force-rm --compress .
docker push remmina/ubuntu:18.04
```


