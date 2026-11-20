#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != Linux || ! -d /run/systemd/system ]]; then
    printf 'Run this installer on a Linux VM with systemd.\n' >&2
    exit 1
fi

repo_root="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
build_directory="${repo_root}/build-relay"
install_directory=/usr/local/libexec/elemental-coop
unit=elemental-coop-relay.service

# Build as the logged-in user. Only installation and service management need sudo.
cmake -S "$repo_root" -B "$build_directory" \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF -DGROUP2_BUILD_SDL_RENDERER=OFF
cmake --build "$build_directory" --target room_relay --parallel 2

sudo install -d -m 0755 "$install_directory"
sudo install -m 0755 "$build_directory/room_relay" "$install_directory/room_relay.new"
sudo mv -fT "$install_directory/room_relay.new" "$install_directory/room_relay"
sudo install -m 0644 "$repo_root/deploy/$unit" "/etc/systemd/system/$unit"
sudo systemctl daemon-reload
sudo systemctl enable "$unit"
sudo systemctl restart "$unit"
sudo systemctl --no-pager status "$unit"
