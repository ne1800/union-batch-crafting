#!/usr/bin/env bash
# Provision a disposable Arch CI container from a dated, signed package snapshot.
set -euo pipefail
# Both hosts serve the same dated snapshot: the Arch Linux Archive and its
# official mirror. Pacman moves to the mirror when the archive fails, so the
# packages never mix with a newer repository state.
snapshot=2026/11/20
for host in archive.archlinux.org umea.archive.pkgbuild.com; do
  # Pacman expands its own repository and architecture placeholders.
  # shellcheck disable=SC2016
  printf 'Server = https://%s/repos/%s/$repo/os/$arch\n' "$host" "$snapshot"
done >/etc/pacman.d/mirrorlist
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
# The dated snapshot hosts sometimes stall. Pacman keeps finished downloads
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
