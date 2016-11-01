FROM cern/slc6-base
MAINTAINER Branislav Ristic <bristic@cern.ch>
ARG version="1.0"
LABEL description="CERN SLC6 base with root dependencies"
LABEL version="${version}"

RUN touch /var/lib/rpm/*
RUN yum -y install bzip2 git vim make glibc-devel which man \
	cmake3 bzip2 tar git vim libpng libjpeg libSM libX11 \
        libXpm libXft libXext || true 
RUN yum -y clean all

ENV DISPLAY          ""

