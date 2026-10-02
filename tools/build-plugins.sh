#!/bin/bash
# Plugins/<isim>/<isim>.kyp uretir: scratch, wasm, c, cpp
# Her .kyp = plugin.json + run + lib.sh. Eksik arac varsa GitHub'dan curl ile indirilir.
mkdir -p Plugins && cd Plugins || exit 1

LIB=$(cat << 'EOF'
# kaya plugin ortak yardimcilari (POSIX sh)
KP_TOOLS="${KAYA_TOOLS:-${XDG_DATA_HOME:-$HOME/.local/share}/kaya/tools}"

kp_die() { echo "$*" >&2; exit 1; }

kp_need() { command -v "$1" >/dev/null 2>&1 || kp_die "eksik arac: $1 ($2)"; }

kp_arch() {
  case "$(uname -m)" in
    x86_64|amd64) echo x86_64 ;;
    aarch64|arm64) echo aarch64 ;;
    *) echo unknown ;;
  esac
}

kp_curl() { curl -fsSL --proto '=https' --proto-redir '=https' -A kaya-plugin "$@"; }

# kp_gh_url <sahip/repo> <regex>: GitHub release dosyalarindan ilk eslesenin adresi.
# Once son kararli surum, olmazsa on surumler dahil son 10 release taranir.
# Adres https://github.com/<sahip/repo>/releases/download/ ile baslamiyorsa reddedilir.
kp_gh_url() {
  kp_need curl "curl kur"
  for _api in "releases/latest" "releases?per_page=10"; do
    _json=$(kp_curl "https://api.github.com/repos/$1/$_api") || continue
    _url=$(printf '%s\n' "$_json" | grep -o '"browser_download_url": *"[^"]*"' \
      | sed 's/^[^:]*: *"//; s/"$//' | grep -iE "$2" | head -n 1)
    case "$_url" in
      "https://github.com/$1/releases/download/"*) printf '%s\n' "$_url"; return 0 ;;
    esac
  done
  return 1
}

# kp_install <ad> <url>: indirir, $KP_TOOLS/<ad> altina acar
kp_install() {
  _name="$1"; _url="$2"
  _tmp=$(mktemp -d) || exit 1
  echo "[$_name] indiriliyor: $_url" >&2
  kp_curl -o "$_tmp/pkg" "$_url" || { rm -rf "$_tmp"; kp_die "[$_name] indirme basarisiz"; }
  mkdir -p "$_tmp/x"
  case "$_url" in
    *.tar.gz|*.tgz) tar -xzf "$_tmp/pkg" -C "$_tmp/x" ;;
    *.tar.xz) tar -xJf "$_tmp/pkg" -C "$_tmp/x" ;;
    *.AppImage) cp "$_tmp/pkg" "$_tmp/x/app.AppImage" && chmod +x "$_tmp/x/app.AppImage" ;;
    *) false ;;
  esac || { rm -rf "$_tmp"; kp_die "[$_name] arsiv acilamadi (tar / xz-utils kurulu mu?)"; }
  mkdir -p "$KP_TOOLS" && rm -rf "$KP_TOOLS/$_name" && mv "$_tmp/x" "$KP_TOOLS/$_name" \
    || { rm -rf "$_tmp"; kp_die "[$_name] kurulamadi: $KP_TOOLS"; }
  rm -rf "$_tmp"
  echo "[$_name] kuruldu: $KP_TOOLS/$_name" >&2
}

# kp_find <ad> <dosya-adi>: indirilmis aracin calistirilabilir dosyasi
kp_find() { find "$KP_TOOLS/$1" -type f -name "$2" -perm -u+x 2>/dev/null | head -n 1; }
EOF
)

build() {  # build <isim> <aciklama> <izinler>   (run icerigini stdin'den okur)
  n="$1"; d="$2"; p="$3"
  mkdir -p "$n/_src"
  printf '{"name":"%s","version":"0.2.0","description":"%s","permissions":[%s]}\n' "$n" "$d" "$p" > "$n/_src/plugin.json"
  cat > "$n/_src/run"
  printf '%s\n' "$LIB" > "$n/_src/lib.sh"
  chmod +x "$n/_src/run"
  (cd "$n/_src" && tar --format=ustar -cf "../$n.kyp" plugin.json run lib.sh)
  rm -rf "$n/_src"
  echo "OK Plugins/$n/$n.kyp"
}

build scratch "Scratch projelerini TurboWarp ile calistirir (yoksa GitHub'dan indirir)" '"run_process","network","filesystem"' << 'EOF'
#!/bin/sh
d=$(cd "$(dirname "$0")" && pwd); . "$d/lib.sh"
sb3="$1"; shift
[ -f "$sb3" ] || kp_die "scratch: dosya bulunamadi: $sb3"

# 1) sistemde kuruluysa onu kullan
for b in turbowarp-desktop turbowarp TurboWarp; do
  command -v "$b" >/dev/null 2>&1 && exec "$b" "$sb3"
done
if command -v flatpak >/dev/null 2>&1 && flatpak info org.turbowarp.TurboWarp >/dev/null 2>&1; then
  exec flatpak run --file-forwarding org.turbowarp.TurboWarp @@ "$sb3" @@
fi

# 2) daha once indirdiysek onu kullan, yoksa GitHub'dan indir
find_tw() {
  for n in turbowarp-desktop TurboWarp turbowarp; do
    bin=$(kp_find turbowarp "$n"); [ -n "$bin" ] && return 0
  done
  bin=$(kp_find turbowarp app.AppImage); [ -n "$bin" ] && appimage=1
  [ -n "$bin" ]
}
appimage=""
if ! find_tw; then
  echo "scratch: TurboWarp bulunamadi, GitHub'dan (TurboWarp/desktop) indiriliyor..." >&2
  case "$(kp_arch)" in
    x86_64)  re='linux.*(x64|x86_64|amd64).*\.tar\.gz$';  re2='(x64|x86_64|amd64).*\.AppImage$' ;;
    aarch64) re='linux.*(arm64|aarch64).*\.tar\.gz$';     re2='(arm64|aarch64).*\.AppImage$' ;;
    *) kp_die "scratch: desteklenmeyen islemci mimarisi: $(uname -m)" ;;
  esac
  url=$(kp_gh_url TurboWarp/desktop "$re") || url=$(kp_gh_url TurboWarp/desktop "$re2") \
    || kp_die "scratch: TurboWarp'un uygun Linux paketi GitHub'da bulunamadi"
  kp_install turbowarp "$url"
  find_tw || kp_die "scratch: indirilen pakette TurboWarp calistirilabilir dosyasi yok"
fi

# WSL'de GPU/WebGL genelde yok: yazilim render (swiftshader) ile ac.
# Ek bayrak vermek icin: KAYA_TW_FLAGS="--bayrak1 --bayrak2" kaya run ...
wsl=""
grep -qi microsoft /proc/version 2>/dev/null && wsl="--enable-unsafe-swiftshader"
if [ -n "$appimage" ]; then
  exec "$bin" --appimage-extract-and-run --no-sandbox $wsl ${KAYA_TW_FLAGS:-} "$sb3"
fi
dir=$(dirname "$bin"); flags=""
if [ -f "$dir/chrome-sandbox" ] && [ ! -u "$dir/chrome-sandbox" ]; then
  echo "scratch: UYARI: chrome-sandbox yetkili degil, --no-sandbox ile aciliyor" >&2
  flags="--no-sandbox"
fi
exec "$bin" $flags $wsl ${KAYA_TW_FLAGS:-} "$sb3"
EOF

build wasm "WebAssembly calistirir (wasmtime yoksa GitHub'dan indirir)" '"run_process","network","filesystem"' << 'EOF'
#!/bin/sh
d=$(cd "$(dirname "$0")" && pwd); . "$d/lib.sh"
wasm="$1"; shift
[ -f "$wasm" ] || kp_die "wasm: dosya bulunamadi: $wasm"

for b in wasmtime wasmer; do
  command -v "$b" >/dev/null 2>&1 && exec "$b" run "$wasm" -- "$@"
done
command -v wasm3 >/dev/null 2>&1 && exec wasm3 "$wasm" "$@"

bin=$(kp_find wasmtime wasmtime)
if [ -z "$bin" ]; then
  echo "wasm: calisma ortami bulunamadi, GitHub'dan wasmtime indiriliyor..." >&2
  arch=$(kp_arch)
  [ "$arch" != unknown ] || kp_die "wasm: desteklenmeyen islemci mimarisi: $(uname -m)"
  url=$(kp_gh_url bytecodealliance/wasmtime "wasmtime-v[0-9.]+-${arch}-linux\.tar\.xz\$") \
    || kp_die "wasm: wasmtime'in uygun Linux paketi bulunamadi"
  kp_install wasmtime "$url"
  bin=$(kp_find wasmtime wasmtime)
  [ -n "$bin" ] || kp_die "wasm: indirilen pakette wasmtime yok"
fi
exec "$bin" run "$wasm" -- "$@"
EOF

build c "C kaynak dosyasini derleyip calistirir" '"run_process"' << 'EOF'
#!/bin/sh
d=$(cd "$(dirname "$0")" && pwd); . "$d/lib.sh"
src="$1"; shift
[ -f "$src" ] || kp_die "c: dosya bulunamadi: $src"
cc_bin=""
for b in cc gcc clang; do command -v "$b" >/dev/null 2>&1 && { cc_bin="$b"; break; }; done
[ -n "$cc_bin" ] || kp_die "c: derleyici yok. Kur: sudo apt install build-essential (Debian/Ubuntu/WSL)"
tmp=$(mktemp -d) || exit 1
trap 'rm -rf "$tmp"' EXIT INT TERM
"$cc_bin" -O2 -o "$tmp/app" "$src" -lm || kp_die "c: derleme basarisiz"
"$tmp/app" "$@"
EOF

build cpp "C++ kaynak dosyasini derleyip calistirir" '"run_process"' << 'EOF'
#!/bin/sh
d=$(cd "$(dirname "$0")" && pwd); . "$d/lib.sh"
src="$1"; shift
[ -f "$src" ] || kp_die "cpp: dosya bulunamadi: $src"
cxx_bin=""
for b in c++ g++ clang++; do command -v "$b" >/dev/null 2>&1 && { cxx_bin="$b"; break; }; done
[ -n "$cxx_bin" ] || kp_die "cpp: derleyici yok. Kur: sudo apt install build-essential (Debian/Ubuntu/WSL)"
tmp=$(mktemp -d) || exit 1
trap 'rm -rf "$tmp"' EXIT INT TERM
"$cxx_bin" -O2 -o "$tmp/app" "$src" || kp_die "cpp: derleme basarisiz"
"$tmp/app" "$@"
EOF

cd ..
echo "Bitti. Plugins/ klasorunu depoya push et."
