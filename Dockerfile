FROM alpine:3.9 as n64
# Install build dependencies
RUN apk --no-cache \
    add --virtual build-dependencies \
    wget \
    tar \
    make \
    diffutils \
    texinfo \
    gcc \
    g++ \
    lua5.3-dev \
    jansson-dev \
    libusb-dev
# Prepare workspace
WORKDIR /usr/local/src/n64
RUN wget \
    --quiet \
    --output-document - \
    https://github.com/glankk/n64/tarball/master \
    | tar xz --strip-components=1
RUN LDFLAGS="-L/usr/lib/lua5.3" ./configure \
    --prefix=/opt/n64 \
    --enable-vc
RUN make install-toolchain -j`cat /proc/cpuinfo | grep processor | wc -l`
# Compile and install
RUN make \
    && make install \
    && make install-sys

FROM alpine:3.9 as gzinject
# Install build dependencies
RUN apk --no-cache \
    add --virtual build-dependencies \
    gcc \
    musl-dev \
    make
# Prepare workspace
WORKDIR /usr/local/src/gzinject
RUN wget \
    --quiet \
    --output-document - \
    https://github.com/krimtonz/gzinject/tarball/master \
    | tar xz --strip-components=1
# Fix unsupported argument
RUN sed -i "s/--target-directory=/-t /" Makefile.in
# Compile and install
RUN ./configure --prefix=/opt/gzinject
RUN make \
    && make install

# Final image
FROM alpine:3.9
RUN apk --no-cache add \
    make \
    jansson \
    lua5.3
COPY --from=n64 /opt/n64 /usr/local
COPY --from=gzinject /opt/gzinject /usr/local
WORKDIR /usr/local/src/gz
COPY . .
RUN make
VOLUME ["/srv/OoT", "/var/gz-output"]
CMD ["/bin/sh"]
