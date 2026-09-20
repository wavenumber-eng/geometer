#!/usr/bin/env bash
set -euo pipefail

if [[ $(id -u) -ne 0 ]]; then
  echo "Run this provisioning script as root." >&2
  exit 2
fi

source /etc/os-release
if [[ ${ID:-} != ubuntu || ${VERSION_ID:-} != 22.04 || $(uname -m) != x86_64 ]]; then
  echo "Expected Ubuntu 22.04 x86_64, found ${PRETTY_NAME:-unknown} $(uname -m)." >&2
  exit 2
fi

uv_version=0.9.15
uv_sha256=2053df0089327569cddd6afea920c2285b482d9b123f5db9f658273e96ab792c
cmake_version=4.1.2
cmake_sha256=773cc679c3a7395413bd096523f8e5d6c39f8718af4e12eb4e4195f72f35e4ab
chrome_version=153.0.8010.52
chrome_sha256=e66f66d4802a46d4a022667e668aa950e277cadbfbed4b3777915b47413a0ef9
node_version=24.12.0
node_sha256=bdebee276e58d0ef5448f3d5ac12c67daa963dd5e0a9bb621a53d1cefbc852fd
rust_version=1.95.0
rustup_sha256=dda7234360b7f578ca8b0ddcb80145646fa61a67c1720a5abc7051b35c9fcb71
builder_user=geometer-builder

export DEBIAN_FRONTEND=noninteractive
apt-get update
apt-get install -y --no-install-recommends \
  build-essential ca-certificates cmake curl fonts-liberation git libasound2 \
  libdbus-1-dev libgbm1 libgl1-mesa-dev libgtk-3-0 libnspr4 libnss3 \
  libudev-dev libwayland-dev libx11-dev libxcursor-dev libxext-dev libxi-dev \
  libxinerama-dev libxkbcommon-dev libxcomposite1 libxdamage1 libxrandr-dev \
  libxss1 libxtst6 ninja-build pkg-config \
  python3 python3-venv unzip wayland-protocols xz-utils zip

if ! id "$builder_user" >/dev/null 2>&1; then
  useradd --create-home --shell /bin/bash "$builder_user"
fi
install -d -o "$builder_user" -g "$builder_user" /work/geometer
install -m 0644 "$(dirname "$0")/local_builder/wsl.conf" /etc/wsl.conf

tool_tmp=$(mktemp -d)
cleanup() {
  rm -rf "$tool_tmp"
  rm -f /usr/local/bin/rustup-init
}
trap cleanup EXIT

curl -fsSL -o "$tool_tmp/uv.tar.gz" \
  "https://github.com/astral-sh/uv/releases/download/${uv_version}/uv-x86_64-unknown-linux-gnu.tar.gz"
echo "$uv_sha256  $tool_tmp/uv.tar.gz" | sha256sum --check --status
tar -xzf "$tool_tmp/uv.tar.gz" -C "$tool_tmp"
install -m 0755 "$tool_tmp/uv-x86_64-unknown-linux-gnu/uv" /usr/local/bin/uv
install -m 0755 "$tool_tmp/uv-x86_64-unknown-linux-gnu/uvx" /usr/local/bin/uvx

curl -fsSL -o "$tool_tmp/cmake.tar.gz" \
  "https://github.com/Kitware/CMake/releases/download/v${cmake_version}/cmake-${cmake_version}-linux-x86_64.tar.gz"
echo "$cmake_sha256  $tool_tmp/cmake.tar.gz" | sha256sum --check --status
rm -rf "/opt/cmake-${cmake_version}"
mkdir -p "/opt/cmake-${cmake_version}"
tar -xzf "$tool_tmp/cmake.tar.gz" -C "/opt/cmake-${cmake_version}" --strip-components=1
for tool in cmake cpack ctest; do
  ln -sfn "/opt/cmake-${cmake_version}/bin/$tool" "/usr/local/bin/$tool"
done

curl -fsSL -o "$tool_tmp/chrome.zip" \
  "https://storage.googleapis.com/chrome-for-testing-public/${chrome_version}/linux64/chrome-linux64.zip"
echo "$chrome_sha256  $tool_tmp/chrome.zip" | sha256sum --check --status
rm -rf "/opt/chrome-for-testing-${chrome_version}"
mkdir -p "/opt/chrome-for-testing-${chrome_version}"
unzip -q "$tool_tmp/chrome.zip" -d "/opt/chrome-for-testing-${chrome_version}"
ln -sfn "/opt/chrome-for-testing-${chrome_version}/chrome-linux64/chrome" \
  /usr/local/bin/google-chrome

curl -fsSL -o "$tool_tmp/node.tar.xz" \
  "https://nodejs.org/dist/v${node_version}/node-v${node_version}-linux-x64.tar.xz"
echo "$node_sha256  $tool_tmp/node.tar.xz" | sha256sum --check --status
rm -rf "/opt/node-v${node_version}"
mkdir -p "/opt/node-v${node_version}"
tar -xJf "$tool_tmp/node.tar.xz" -C "/opt/node-v${node_version}" --strip-components=1
ln -sfn "/opt/node-v${node_version}/bin/node" /usr/local/bin/node
ln -sfn "/opt/node-v${node_version}/bin/npm" /usr/local/bin/npm
ln -sfn "/opt/node-v${node_version}/bin/npx" /usr/local/bin/npx
PATH="/opt/node-v${node_version}/bin:$PATH" npm install --global npm@11.16.0

curl -fsSL -o "$tool_tmp/rustup-init" \
  https://static.rust-lang.org/rustup/dist/x86_64-unknown-linux-gnu/rustup-init
echo "$rustup_sha256  $tool_tmp/rustup-init" | sha256sum --check --status
chmod 0755 "$tool_tmp/rustup-init"
install -m 0755 "$tool_tmp/rustup-init" /usr/local/bin/rustup-init
runuser -u "$builder_user" -- env HOME="/home/$builder_user" \
  /usr/local/bin/rustup-init -y --profile minimal --default-toolchain "$rust_version"
rm -f /usr/local/bin/rustup-init
runuser -u "$builder_user" -- env HOME="/home/$builder_user" \
  "/home/$builder_user/.cargo/bin/rustup" component add --toolchain "$rust_version" clippy rustfmt

cat >/etc/geometer-builder-provisioned <<EOF
schema=wn.geometer.local_builder_provision.a0
ubuntu=22.04
architecture=x86_64
uv=$uv_version
cmake=$cmake_version
chrome=$chrome_version
node=$node_version
npm=11.16.0
rust=$rust_version
rust_components=clippy,rustfmt
EOF

echo "Provisioned Geometer Ubuntu 22.04 builder. Terminate and restart the WSL distribution to apply isolation."
