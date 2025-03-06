FROM mcr.microsoft.com/devcontainers/cpp:1.2.7-debian12 AS dev

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

FROM debian:12.9-slim AS ci

ENV DEBIAN_FRONTEND=noninteractive

COPY --from=dev /usr/local/include/ /usr/local/include/
COPY --from=dev /usr/local/lib/ /usr/local/lib/

RUN set -x; \
	apt-get update && apt-get install -y --no-install-recommends \
  git \
  ca-certificates \
  curl \
  gpg \
	build-essential \
	lcov \
	cmake ; \
	rm -rf /var/lib/apt/lists/*
