FROM gcc:latest

# dependencies
RUN apt-get -y update && apt-get install build-essential libreadline-dev autoconf-archive libgmp-dev expect flex bison automake m4 libtool pkg-config

# build
COPY . /usr/src/bic
WORKDIR /usr/src/bic
RUN autoreconf -i && ./configure && make && make check && make install

# run
CMD ["bic"]
