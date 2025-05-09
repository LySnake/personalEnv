FROM ubuntu:20.04

RUN echo "COPY 文件到相关目录"
COPY .vimrc /root/ 
COPY sources.list /etc/apt/

RUN echo "更新源并升级"
RUN apt-get update
RUN apt-get upgrade -y

RUN echo "安装相关软件"
RUN apt-get install iputils-ping net-tools \
                    curl wget man \
                    manpages-de  manpages-de-dev  manpages-dev glibc-doc manpages-posix-dev manpages-posix manpages-zh \
                    aptitude \
					-y

# RUN apt-get install build-essential gdb cmake git 
# RUN apt-get install python3-full python3-pip


CMD ["/bin/bash"]
