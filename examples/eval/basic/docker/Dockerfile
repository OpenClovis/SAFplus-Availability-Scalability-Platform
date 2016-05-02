
FROM ubuntu:14.04
MAINTAINER Andrew Stone <stone@openclovis.com>

RUN apt-get update && apt-get install -y openssh-server
RUN mkdir /var/run/sshd
RUN echo 'root:clovis' | chpasswd
RUN sed -i 's/PermitRootLogin without-password/PermitRootLogin yes/' /etc/ssh/sshd_config

# SSH login fix. Otherwise user is kicked off after login
RUN sed 's@session\s*required\s*pam_loginuid.so@session optional pam_loginuid.so@g' -i /etc/pam.d/sshd
ENV NOTVISIBLE "in users profile"
RUN echo "export VISIBLE=now" >> /etc/profile

RUN apt-get install -y libprotobuf8 libbz2-1.0 libxml2 libdb5.3 libgdbm3 libsqlite3-0
RUN apt-get install python python-protobuf 
RUN apt-get install -y libboost-program-options1.54.0  libboost-thread1.54.0 libboost-system1.54.0 libboost-filesystem1.54.0 libboost-chrono1.54.0 

RUN locale-gen en_US en_US.UTF-8
RUN dpkg-reconfigure locales

# debugging
RUN apt-get install -y gdb

# SAFPLUS stuff
ENV LD_LIBRARY_PATH "/lib:/usr/lib:/root/evalBasic/lib:/root/evalBasic/plugin"
ENV PATH "/bin:/usr/bin:/sbin:/usr/sbin:/usr/local/sbin:/root/evalBasic/bin"
ENV ASP_NODENAME "node0"
ENV SAFPLUS_BACKPLANE_INTERFACE "eth0"
ENV SAFPLUS_CLOUD_NODES "1"
# ENV ASP_NODEADDR "1"
ENV ASP_BINDIR "/root/evalBasic/bin"
ENV PYTHONPATH "/root/evalBasic/lib"
COPY  evalBasic.tgz /root
RUN (cd /root; tar xvfz evalBasic.tgz)
COPY model.xml /root/evalBasic/bin
COPY go    /root
COPY setup /root/evalBasic/bin
RUN (cd /root/evalBasic/bin; ./safplus_db -x model.xml safplusAmf)
EXPOSE 22 80
# SAFplus UDP ports
EXPOSE 21500-21508/udp
#CMD ["/usr/sbin/sshd", "-D"]
CMD ["/root/go"]
#CMD ["/root/evalBasic/bin/safplus_cloud", "--add", "127.0.0.1", "--id", "1"]
#CMD ["/root/evalBasic/bin/safplus_amf",""]
