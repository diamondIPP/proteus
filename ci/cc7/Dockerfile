FROM cern/cc7-base
LABEL maintainer="Moritz Kiehn <msmk@cern.ch>"
LABEL version="1.0"
LABEL description="CERN CC7 with Proteus dependencies"

RUN yum -y update \
    && yum -y install \
        clang \
        cmake \
        doxygen \
        gcc-c++ \
        make \
        ninja-build \
        root \
        root-genvector \
        root-smatrix \
        python34-pip \
    && yum -y clean all
# the packages provided by cc7 seem to be inconsistent with respect to
# the required python versions so we install them manually.
RUN pip3.4 install Sphinx breathe
