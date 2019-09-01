To build:
make docker

To run a cluster within docker instances running on this node:
docker run -d -P --name testbasic1 -e "SAFPLUS_NODE_ID=1" -e "ASP_NODENAME=node0" -e "SAFPLUS_CLOUD_NODES=172.17.0.2,172.17.0.3" evalbasic
docker run -d -P --name testbasic2 -e "SAFPLUS_NODE_ID=2" -e "ASP_NODENAME=node1" -e "SAFPLUS_CLOUD_NODES=172.17.0.2,172.17.0.3" evalbasic

If docker supports UDP broadcasts you might be able to do this:
docker run -d -P --name testbasic1 -e "SAFPLUS_NODE_ID=1" -e "ASP_NODENAME=node0" evalbasic
docker run -d -P --name testbasic2 -e "SAFPLUS_NODE_ID=2" -e "ASP_NODENAME=node1" evalbasic

To stop:
docker stop testbasic1 testbasic2
docker rm testbasic1 testbasic2

to access:
docker port testbasic1 22
ssh -p <port> root@localhost
