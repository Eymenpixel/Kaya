#!/bin/sh
set -eu
bin_dir="${HOME:?HOME is not set}/.local/bin"
mkdir -p "$bin_dir"
link="$bin_dir/apt"

usage() {
    printf '%s\n' 'Usage: small-creatures --add APT | --remove APT | --list'
}

resolve() {
    if command -v readlink >/dev/null 2>&1; then readlink -f -- "$1" 2>/dev/null || printf '%s\n' "$1"
    else printf '%s\n' "$1"; fi
}

case "${1:-}" in
    --add)
        [ "${2:-}" = APT ] || { usage >&2; exit 2; }
        apt_path=$(command -v apt 2>/dev/null || true)
        [ -n "$apt_path" ] || { printf '%s\n' 'APT is not installed on this Ubuntu/WSL host.' >&2; exit 1; }
        apt_real=$(resolve "$apt_path")
        if [ -e "$link" ] || [ -L "$link" ]; then
            current=$(resolve "$link")
            [ "$current" = "$apt_real" ] && { printf 'APT is already linked: %s\n' "$link"; exit 0; }
            printf 'Refusing to overwrite existing path: %s\n' "$link" >&2
            exit 1
        fi
        ln -s "$apt_real" "$link"
        printf 'Linked host APT: %s -> %s\n' "$link" "$apt_real"
        printf '%s\n' 'Kaya remains installed. No system files were changed.'
        ;;
    --remove)
        [ "${2:-}" = APT ] || { usage >&2; exit 2; }
        if [ -L "$link" ]; then
            target=$(resolve "$link")
            apt_path=$(command -v apt 2>/dev/null || true)
            apt_real=$( [ -n "$apt_path" ] && resolve "$apt_path" || true )
            [ "$target" = "$apt_real" ] || { printf '%s\n' 'Refusing to remove a link that does not point to host APT.' >&2; exit 1; }
            rm -- "$link"
            printf '%s\n' 'Removed the APT link. Kaya and Ubuntu APT are unchanged.'
        elif [ -e "$link" ]; then
            printf 'Refusing to remove a non-symlink path: %s\n' "$link" >&2
            exit 1
        else
            printf '%s\n' 'APT link is not installed.'
        fi
        ;;
    --list)
        if [ -L "$link" ]; then printf 'APT link: %s -> %s\n' "$link" "$(resolve "$link")"
        else printf '%s\n' 'APT link is not installed.'; fi
        ;;
    *) usage; exit 2 ;;
esac
