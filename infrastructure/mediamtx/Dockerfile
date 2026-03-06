# MediaTx - Production RTSP Media Server
# Multi-stage build with security hardening

# Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    git \
    wget \
    ca-certificates \
    pkg-config \
    cmake \
    && rm -rf /var/lib/apt/lists/*

# Build GStreamer with hardware acceleration support for RK3588
WORKDIR /build
RUN git clone --depth 1 --branch 1.22.0 https://gitlab.freedesktop.org/gstreamer/gstreamer.git \
    && cd gstreamer \
    && meson setup build \
        --prefix=/usr \
        --buildtype=release \
        -Dgpl=enabled \
        -Dgst-plugins-ugly:x264=enabled \
        -Dgst-plugins-bad:va=enabled \
        -Dgst-plugins-bad:videoparsers=enabled \
        -Dgst-plugins-good:rtsp=enabled \
        -Dgst-plugins-good:rtp=enabled \
        -Dgst-plugins-base:app=enabled \
        -Dgst-plugins-base:video=enabled \
        -Dintrospection=disabled \
        -Ddoc=disabled \
        -Dtests=disabled \
        -Dexamples=disabled \
    && ninja -C build \
    && ninja -C build install

# Production stage
FROM bluenviron/mediamtx:1.6.0 AS production

LABEL maintainer="FalconMind Engineering Team <engineering@falconmind.ai>"
LABEL version="1.0.0"
LABEL description="Production RTSP media server with GStreamer hardware acceleration"

# Install runtime dependencies
RUN apt-get update && apt-get install -y --no-install-recommends \
    libgstreamer1.0-0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-plugins-ugly \
    gstreamer1.0-libav \
    gstreamer1.0-tools \
    gstreamer1.0-rtsp \
    libgstrtspserver-1.0-0 \
    libva2 \
    libva-drm2 \
    ca-certificates \
    curl \
    tzdata \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Copy built GStreamer from builder
COPY --from=builder /usr/lib/x86_64-linux-gnu/gstreamer-1.0 /usr/lib/x86_64-linux-gnu/gstreamer-1.0

# Create non-root user
RUN groupadd -r mediamtx && useradd -r -g mediamtx mediamtx

# Create necessary directories
RUN mkdir -p /etc/mediamtx/certs \
    /var/log/falconmind \
    /var/lib/falconmind/recordings \
    /var/lib/falconmind/hls \
    /tmp/mediamtx \
    && chown -R mediamtx:mediamtx /etc/mediamtx \
    /var/log/falconmind \
    /var/lib/falconmind \
    /tmp/mediamtx

# Copy configuration
COPY --chown=mediamtx:mediamtx mediamtx.yml /etc/mediamtx/mediamtx.yml
COPY --chown=mediamtx:mediamtx auth.json /etc/mediamtx/auth.json

# Copy TLS certificates
COPY --chown=mediamtx:mediamtx certs/ca.crt /etc/mediamtx/certs/
COPY --chown=mediamtx:mediamtx certs/server.crt /etc/mediamtx/certs/
COPY --chown=mediamtx:mediamtx certs/server.key /etc/mediamtx/certs/
RUN chmod 600 /etc/mediamtx/certs/server.key

# Set permissions
RUN chmod 644 /etc/mediamtx/*.yml /etc/mediamtx/*.json \
    && chmod 644 /etc/mediamtx/certs/*.crt

# Install health check script
COPY --chown=mediamtx:mediamtx healthcheck.sh /usr/local/bin/
RUN chmod +x /usr/local/bin/healthcheck.sh

# Expose ports
# 8554: RTSP
# 8322: RTSPS (RTSP over TLS)
# 1935: RTMP
# 8888: HLS
# 8889: WebRTC
# 9997: API
# 9998: Metrics
EXPOSE 8554 8322 1935 8888 8889 9997 9998

# Health check
HEALTHCHECK --interval=15s --timeout=5s --start-period=30s --retries=3 \
    CMD /usr/local/bin/healthcheck.sh || exit 1

# Switch to non-root user
USER mediamtx

# Set environment variables
ENV GST_DEBUG=2
ENV GST_PLUGIN_PATH=/usr/lib/x86_64-linux-gnu/gstreamer-1.0
ENV MALLOC_ARENA_MAX=2

# Start MediaTx
STOPSIGNAL SIGTERM

ENTRYPOINT ["/mediamtx"]
CMD ["/etc/mediamtx/mediamtx.yml"]
