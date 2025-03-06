FROM mcr.microsoft.com/devcontainers/cpp:1.2.7-debian12

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
clang-format \
clang-tidy \
netcat-openbsd \
iputils-ping \
lcov \
libcap-dev \
meson \
xsltproc

# install google test
RUN set -x; \
	git clone https://github.com/google/googletest.git; \
	cd googletest; \
	mkdir build; \
	cd build; \
	cmake ..; \
	make; \
	make install
