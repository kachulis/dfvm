FROM ubuntu:20.04

ARG DEBIAN_FRONTEND=noninteractive

WORKDIR /docker_build/

# Install required packages
#RUN apt-get update && apt-get install -y build-essential libbz2-dev libcurl4-openssl-dev autoconf libssl-dev wget zlib1g-dev liblzma-dev libdeflate-dev
RUN apt-get update && apt-get install -y build-essential autoconf wget zlib1g-dev libcurl4-openssl-dev
# Download and build boost program_options and iostreams
RUN wget https://archives.boost.io/release/1.78.0/source/boost_1_78_0.tar.gz && \
tar -xf boost_1_78_0.tar.gz && \
rm boost_1_78_0.tar.gz && \
cd boost_1_78_0/ && \
./bootstrap.sh --with-libraries=iostreams,program_options,serialization --prefix=../boost && \
./b2 install && \
cd .. && \
cp boost/lib/libboost_program_options.so.1.78.0 /usr/local/lib/ && \
ln -s /usr/local/lib/libboost_program_options.so.1.78.0 /usr/local/lib/libboost_program_options.so && \
cp -r boost/include/boost/ /usr/include/ && \
rm -r boost_1_78_0 boost

RUN apt-get install -y libbz2-dev liblzma-dev libdeflate-dev
# Download and build htslib
RUN wget https://github.com/samtools/htslib/releases/download/1.21/htslib-1.21.tar.bz2 && \
tar -xf htslib-1.21.tar.bz2 && \
rm htslib-1.21.tar.bz2 && \
cd htslib-1.21 && \
autoheader && \
autoconf && \
./configure --enable-libcurl --with-libdeflate && \
make install && \
cd .. && \
rm -r htslib-1.21


RUN mkdir dfvm
COPY *.cpp dfvm
COPY *.hh dfvm
COPY Makefile dfvm

RUN cd dfvm && \
make

ENV LD_LIBRARY_PATH=/usr/local/lib

#RUN apt-get install -y gdb

WORKDIR /