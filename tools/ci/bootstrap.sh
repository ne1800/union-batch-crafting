#!/usr/bin/env bash
# Provision a disposable Arch CI container from a dated, signed package snapshot.
set -euo pipefail
# Pacman expands its own repository and architecture placeholders.
# shellcheck disable=SC2016
printf '%s\n' 'Server = https://archive.archlinux.org/repos/2026/11/20/$repo/os/$arch' >/etc/pacman.d/mirrorlist
packages=(git mise python)
case "${1:?expected job name}" in
  lint) packages+=(clang gcc go nodejs npm uv) ;;
  portable | sanitizers) packages+=(clang cmake ninja gcc compiler-rt) ;;
  windows) packages+=(clang cmake ninja lld llvm) ;;
  history | export | commits) ;;
  *) exit 2 ;;
esac
# The runner supplies the outer Docker sandbox. Its kernel rejects nested
# seccomp filters during signed package downloads.
# The dated archive mirror sometimes stalls. Pacman keeps finished downloads
# in its cache, so a retry only fetches what is still missing.
for attempt in 1 2 3; do
  pacman -Syu --disable-sandbox --needed --noconfirm "${packages[@]}" && break
  if ((attempt == 3)); then
    exit 1
  fi
  sleep $((attempt * 20))
done
git config --global --add safe.directory "$PWD"
mise trust
