#!/usr/bin/env bash
# Export the host's trusted root CAs into .devcontainer/certs/ so the dev
# container build can verify HTTPS behind a TLS-intercepting corporate proxy
# (e.g. Zscaler). macOS only. The exported *.crt files are gitignored — they're
# local, network-specific trust material.
#
# Usage:  .devcontainer/import-host-certs.sh   then rebuild the dev container.
set -euo pipefail
dir="$(cd "$(dirname "$0")/certs" && pwd)"
rm -f "$dir"/corp-*.crt
security find-certificate -a -p /Library/Keychains/System.keychain > /tmp/host-roots.pem
# Split the concatenated PEM into one certificate per .crt (update-ca-certificates
# registers each). Lines before the first cert are ignored.
awk -v d="$dir" '
  f              { print > file }
  /-----BEGIN CERTIFICATE-----/ { n++; file=sprintf("%s/corp-%03d.crt", d, n); f=1; print > file }
  /-----END CERTIFICATE-----/   { f=0 }
' /tmp/host-roots.pem
rm -f /tmp/host-roots.pem
echo "Wrote $(ls "$dir"/corp-*.crt 2>/dev/null | wc -l | tr -d ' ') cert(s) to $dir"
echo "Now: Dev Containers: Rebuild Container"
