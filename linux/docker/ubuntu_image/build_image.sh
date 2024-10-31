#!/bin/bash
echo "###########################################################"
echo "docker info:"
docker info


echo "###########################################################"
echo "docker build:"
dockerFile_dir=$(cd `dirname $0`; pwd)
# echo ${dockerFile_dir}
docker build -t ubuntu2404:1.0.0 ${dockerFile_dir}