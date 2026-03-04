#!/usr/bin/env bash
set -euo pipefail
DIR=$(cd "$(dirname "$0")/../nginx/certs/dev" && pwd)
mkdir -p "$DIR"
openssl req -x509 -newkey rsa:4096 -days 365 -nodes \
  -subj "/CN=localhost" \
  -keyout "$DIR/server.key" -out "$DIR/server.crt"
chmod 600 "$DIR/server.key" "$DIR/server.crt"
echo "Dev TLS certificates generated at $DIR"
