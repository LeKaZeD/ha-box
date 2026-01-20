# https://developers.home-assistant.io/docs/add-ons/configuration#add-on-dockerfile
ARG BUILD_FROM
FROM $BUILD_FROM

# Execute during the build of the image
ARG TEMPIO_VERSION BUILD_ARCH
RUN \
    curl -sSLf -o /usr/bin/tempio \
    "https://github.com/home-assistant/tempio/releases/download/${TEMPIO_VERSION}/tempio_${BUILD_ARCH}"

# Install runtime dependencies
RUN \
    apk add --no-cache \
        python3 \
        py3-pip \
        py3-requests

# Install build dependencies (needed for compiling Python packages)
RUN \
    apk add --no-cache --virtual .build-deps \
        linux-headers \
        gcc \
        python3-dev \
        musl-dev \
        zlib-dev \
        jpeg-dev \
        freetype-dev \
        lcms2-dev \
        openjpeg-dev \
        tiff-dev

# Install architecture-specific packages
# i2c-tools and RPi.GPIO are only available/needed on aarch64 (Raspberry Pi)
RUN \
    if [ "$BUILD_ARCH" = "aarch64" ]; then \
        apk add --no-cache i2c-tools && \
        pip3 install --no-cache-dir RPi.GPIO>=0.7.1; \
    fi

# Install Python packages
COPY requirements.txt /tmp/
RUN \
    pip3 install --no-cache-dir -r /tmp/requirements.txt \
    && rm -rf /tmp/requirements.txt

# Remove build dependencies to reduce image size
RUN \
    apk del --no-cache .build-deps

# Copy root filesystem
COPY rootfs /
