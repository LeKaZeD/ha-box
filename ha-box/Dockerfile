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

# Note: RPi.GPIO and i2c-tools not needed for UART communication only
# Note: Build dependencies (gcc, python3-dev, image libraries) not needed for current minimal setup
# They will be added back when GPIO/I2C/SPI/display features are implemented

# Install Python packages
# Using --break-system-packages is safe in a container environment
COPY requirements.txt /tmp/
RUN \
    pip3 install --no-cache-dir --break-system-packages -r /tmp/requirements.txt \
    && rm -rf /tmp/requirements.txt

# Copy translations (for Python i18n module)
COPY translations /addon/translations

# Copy root filesystem
COPY rootfs /

# Make s6-overlay scripts executable
# This is critical for s6-overlay v3 to work properly
RUN \
    chmod +x /etc/s6-overlay/s6-rc.d/ha-box/run \
    && chmod +x /etc/s6-overlay/s6-rc.d/ha-box/finish \
    && chmod 644 /etc/s6-overlay/s6-rc.d/ha-box/type \
    && find /etc/cont-init.d -type f -exec chmod +x {} \; 2>/dev/null || true \
    && find /etc/cont-finish.d -type f -exec chmod +x {} \; 2>/dev/null || true
