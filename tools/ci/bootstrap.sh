#!/usr/bin/env bash
# Provision a disposable Arch CI container from a dated, signed package snapshot.
set -euo pipefail
# Both hosts serve the dated databases: the Arch Linux Archive and its official
# mirror. Pacman moves to the mirror when the archive fails, so the packages
# never mix with a newer repository state.
snapshot=2026/11/20
mirror=https://umea.archive.pkgbuild.com
for host in https://archive.archlinux.org "$mirror"; do
  # Pacman expands its own repository and architecture placeholders.
  # shellcheck disable=SC2016
  printf 'Server = %s/repos/%s/$repo/os/$arch\n' "$host" "$snapshot"
done >/etc/pacman.d/mirrorlist
packages=(git mise python)
case "${1:?expected job name}" in
  lint) packages+=(clang gcc go nodejs npm uv) ;;
  portable | sanitizers) packages+=(clang cmake ninja gcc compiler-rt) ;;
  windows) packages+=(clang cmake ninja lld llvm) ;;
  history | export | commits) ;;
  *) exit 2 ;;
esac
# GitHub Actions restores .cache/pacman while this script stays unchanged. Its
# databases and packages belong to the snapshot above, so a restored cache
# installs without a refresh and downloads only what it lacks.
cache=$PWD/.cache/pacman
mkdir -p "$cache/pkg" "$cache/sync"
refresh=(-y)
if [[ -s "$cache/sync/core.db" && -f "$cache/snapshot" && "$(<"$cache/snapshot")" == "$snapshot" ]]; then
  cp "$cache"/sync/*.db /var/lib/pacman/sync/
  refresh=()
fi
# The runner supplies the outer Docker sandbox. Its kernel rejects nested
# seccomp filters during signed package downloads.
# The mirror keeps its packages only in a per-package pool, which pacman cannot
# address. Fetch the missing ones from there into pacman's cache. Pacman still
# verifies every signature before it installs a package.
fetch_from_pool() {
  local name file
  pacman -Sup --needed --print-format '%n %f' "${packages[@]}" |
    while read -r name file; do
      [[ -s "$cache/pkg/$file" ]] && continue
      for part in "$file.sig" "$file"; do
        curl -fsS --retry 3 --remove-on-error -o "$cache/pkg/$part" \
          "$mirror/packages/${name:0:1}/$name/$part"
      done
    done
}
# The dated snapshot hosts sometimes stall. Pacman keeps finished downloads
# in its cache, so a retry only fetches what is still missing.
for attempt in 1 2 3; do
  pacman -Su "${refresh[@]}" --cachedir "$cache/pkg" --disable-sandbox --needed --noconfirm \
    "${packages[@]}" && break
  if ((attempt == 3)); then
    exit 1
  fi
  fetch_from_pool || echo "Fetching from the mirror pool failed" >&2
  sleep $((attempt * 20))
done
cp /var/lib/pacman/sync/*.db "$cache/sync/"
echo "$snapshot" >"$cache/snapshot"
git config --global --add safe.directory "$PWD"
mise trust
