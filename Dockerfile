FROM ubuntu:14.04

MAINTAINER ZeroMQ Project <zeromq@imatix.com>

# Update repositories
RUN apt-get update

# Install dependencies to build ZeroMQ libraries
RUN DEBIAN_FRONTEND=noninteractive apt-get install -y git build-essential libtool autoconf automake pkg-config unzip libkrb5-dev

# Clone, build and install libsodium support for ZeroMQ
RUN cd /tmp && git clone git://github.com/jedisct1/libsodium.git && cd libsodium && git checkout e2a30a && ./autogen.sh && ./configure && make check && make install && ldconfig

# Clone, build and install ZeroMQ
RUN cd /tmp && git clone git://github.com/zeromq/libzmq.git && cd libzmq && ./autogen.sh && ./configure && make && make install && ldconfig

# Clone, build and install CZMQ
RUN cd /tmp && git clone git://github.com/zeromq/czmq.git && cd czmq && ./autogen.sh && ./configure && make && make install && ldconfig

# Clone, build and install Zyre
RUN cd /tmp && git clone git://github.com/zeromq/zyre.git && cd zyre && ./autogen.sh && ./configure && make && make install && ldconfig

# Copy chat C-code to home directory
RUN cp /tmp/zyre/examples/chat/chat.c ~/

# Remove all temporary files
RUN rm /tmp/* -rf 

# Compile chat code
RUN cd && gcc chat.c -lczmq -lzyre -o chat
RUN cd && cp chat /usr/bin/
# Run the chat client
ENTRYPOINT ["chat"]
