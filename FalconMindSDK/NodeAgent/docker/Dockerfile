# Multi-stage build for FalconMind NodeAgent
# Supports: x86_64, ARM64, ARMv7

# Stage 1: Build stage
FROM ubuntu:22.04 AS builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    pkg-config \
    libsqlite3-dev \
    libssl-dev \
    zlib1g-dev \
    libbrotli-dev \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Copy source code
COPY . /src
WORKDIR /src/build

# Build NodeAgent
RUN cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DNODEAGENT_USE_MQTT=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    && make -j$(nproc) \
    && make install

# Stage 2: Runtime stage
FROM ubuntu:22.04 AS runtime

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libssl3 \
    zlib1g \
    libbrotli1 \
    curl \
    jq \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create user and directories
RUN groupadd -r nodeagent && \
    useradd -r -g nodeagent -s /bin/false nodeagent && \
    mkdir -p /etc/nodeagent /var/lib/nodeagent /var/log/nodeagent && \
    chown -R nodeagent:nodeagent /var/lib/nodeagent /var/log/nodeagent

# Copy built binaries from builder
COPY --from=builder /usr/local/lib/libnodeagent.so* /usr/local/lib/
COPY --from=builder /usr/local/bin/nodeagent* /usr/local/bin/
COPY --from=builder /src/build/nodeagent_unit_tests /usr/local/bin/

# Copy demo executables
COPY --from=builder /src/build/nodeagent_demo /usr/local/bin/
COPY --from=builder /src/build/cluster_center_mock /usr/local/bin/

# Update library cache
RUN ldconfig

# Copy entrypoint and utility scripts
COPY docker/entrypoint.sh /usr/local/bin/
COPY docker/healthcheck.sh /usr/local/bin/nodeagent-health
COPY docker/status.sh /usr/local/bin/nodeagent-status
RUN chmod +x /usr/local/bin/entrypoint.sh /usr/local/bin/nodeagent-health /usr/local/bin/nodeagent-status

# Copy default configuration
COPY docker/config.yaml /etc/nodeagent/config.yaml

# Expose Prometheus metrics port
EXPOSE 9090

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD nodeagent-health || exit 1

# Set entrypoint
ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
CMD ["nodeagent"]

# Labels
LABEL maintainer="FalconMind Team"
LABEL description="FalconMind NodeAgent - UAV Edge Computing Platform"
LABEL version="1.0.0"
