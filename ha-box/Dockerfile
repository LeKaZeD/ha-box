# https://developers.home-assistant.io/docs/add-ons/configuration#add-on-dockerfile
ARG BUILD_FROM
FROM $BUILD_FROM

# Execute during the build of the image
ARG TEMPIO_VERSION BUILD_ARCH
RUN \
    curl -sSLf -o /usr/bin/tempio \
    "https://github.com/home-assistant/tempio/releases/download/${TEMPIO_VERSION}/tempio_${BUILD_ARCH}"

# Install Python dependencies
RUN \
    apk add --no-cache \
        python3 \
        py3-pip \
        py3-pillow \
        py3-requests \
        i2c-tools \
        linux-headers \
        gcc \
        python3-dev \
        musl-dev

# Install Python packages
COPY requirements.txt /tmp/
RUN \
    pip3 install --no-cache-dir -r /tmp/requirements.txt \
    && rm -rf /tmp/requirements.txt

# Copy root filesystem
COPY rootfs /
