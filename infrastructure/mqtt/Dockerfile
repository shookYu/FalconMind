# FalconMind NanoMQ - Production Deployment
# Multi-stage build for security and optimization

# Build stage
FROM emqx/nanomq:0.21.8 AS production

LABEL maintainer="FalconMind Engineering Team <engineering@falconmind.ai>"
LABEL version="1.0.0"
LABEL description="Production-grade MQTT broker for UAV swarm communication"

# Install security updates
USER root
RUN apt-get update && apt-get upgrade -y \
    && apt-get install -y --no-install-recommends \
        ca-certificates \
        curl \
        tzdata \
    && rm -rf /var/lib/apt/lists/* \
    && apt-get clean

# Create non-root user for security
RUN groupadd -r nanomq && useradd -r -g nanomq nanomq

# Create necessary directories
RUN mkdir -p /etc/nanomq/certs \
    /var/log/nanomq \
    /var/lib/nanomq \
    && chown -R nanomq:nanomq /etc/nanomq \
    /var/log/nanomq \
    /var/lib/nanomq

# Copy production configuration
COPY --chown=nanomq:nanomq nanomq.conf /etc/nanomq/nanomq.conf
COPY --chown=nanomq:nanomq acl.conf /etc/nanomq/acl.conf

# Copy TLS certificates (in production, mount as secrets)
COPY --chown=nanomq:nanomq certs/ca.crt /etc/nanomq/certs/
COPY --chown=nanomq:nanomq certs/server.crt /etc/nanomq/certs/
COPY --chown=nanomq:nanomq certs/server.key /etc/nanomq/certs/
RUN chmod 600 /etc/nanomq/certs/server.key

# Security: Set proper permissions
RUN chmod 644 /etc/nanomq/*.conf \
    && chmod 644 /etc/nanomq/certs/*.crt

# Expose ports
# 1883: MQTT (TCP)
# 8883: MQTTS (TLS)
# 8083: WebSocket (WS)
# 8084: WebSocket Secure (WSS)
# 8081: Management API (HTTP)
EXPOSE 1883 8883 8083 8084 8081

# Health check with proper timeout and retry logic
HEALTHCHECK --interval=10s --timeout=5s --start-period=30s --retries=3 \
    CMD curl -sf http://localhost:8081/api/v1/health || exit 1

# Switch to non-root user
USER nanomq

# Set resource limits
ENV MALLOC_ARENA_MAX=2
ENV MALLOC_MMAP_THRESHOLD_=131072
ENV MALLOC_TRIM_THRESHOLD_=131072
ENV MALLOC_TOP_PAD_=131072

# Start NanoMQ with proper signal handling
STOPSIGNAL SIGTERM

ENTRYPOINT ["nanomq"]
CMD ["start", "--conf", "/etc/nanomq/nanomq.conf"]
