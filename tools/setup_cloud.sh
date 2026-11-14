#!/usr/bin/env bash
# Debian 13 user-owned toolchain. Does not alter repository source or system packages.
set -euo pipefail
prefix=/workspace/mf-toolchain
export XDG_CACHE_HOME="$prefix/cache"
mkdir -p "$XDG_CACHE_HOME"
python -m venv /workspace/.venv-mf
/workspace/.venv-mf/bin/python -m pip install --disable-pip-version-check cmake==3.31.6 ninja==1.11.1.4 pillow==11.3.0
mkdir -p "$prefix/apt/lists/partial" "$prefix/apt/cache/archives/partial" "$prefix/sysroot"
cat > "$prefix/apt/sources.list" <<'EOF'
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie main
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://security.debian.org/debian-security trixie-security main
deb [signed-by=/usr/share/keyrings/debian-archive-keyring.gpg] https://deb.debian.org/debian trixie-updates main
EOF
mkdir -p "$prefix/apt/empty"
cat > "$prefix/apt/apt.conf" <<'EOF'
Dir::Etc::sourcelist "/workspace/mf-toolchain/apt/sources.list";
Dir::Etc::sourceparts "/workspace/mf-toolchain/apt/empty";
Dir::Etc::parts "/workspace/mf-toolchain/apt/empty";
Dir::State::lists "/workspace/mf-toolchain/apt/lists";
Dir::Cache "/workspace/mf-toolchain/apt/cache";
APT::Get::List-Cleanup "0";
EOF
export APT_CONFIG="$prefix/apt/apt.conf"
/usr/bin/apt-get update
# Download only; signatures and repository checksums remain enforced by APT.
/usr/bin/apt-get -o Debug::NoLocking=1 --download-only -y --no-install-recommends install libasound2-dev libfreetype6-dev libfontconfig1-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxext-dev libxrender-dev libgl1-mesa-dev zlib1g-dev libbrotli-dev xvfb xauth
(
 cd "$prefix/apt/cache/archives"
 /usr/bin/apt-get download libfreetype6 libfontconfig1 libasound2t64 libx11-6 libxext6 libxrandr2 libxinerama1 libxcursor1 libxrender1 libxfixes3 libxau6 libxdmcp6 libxcb1 libgl1 libglx0 libegl1 libopengl0 libbz2-1.0 libpng16-16t64
)
for archive in "$prefix"/apt/cache/archives/*.deb; do
 dpkg-deb -x "$archive" "$prefix/sysroot"
done
cat > "$prefix/activate.sh" <<'EOF'
export XDG_CACHE_HOME=/workspace/mf-toolchain/cache
export PATH=/workspace/.venv-mf/bin:/workspace/mf-toolchain/sysroot/usr/bin:$PATH
export PKG_CONFIG_SYSROOT_DIR=/workspace/mf-toolchain/sysroot
export PKG_CONFIG_LIBDIR=/workspace/mf-toolchain/sysroot/usr/lib/x86_64-linux-gnu/pkgconfig:/workspace/mf-toolchain/sysroot/usr/share/pkgconfig:/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig
export CPLUS_INCLUDE_PATH=/workspace/mf-toolchain/sysroot/usr/include
export C_INCLUDE_PATH=/workspace/mf-toolchain/sysroot/usr/include
export LIBRARY_PATH=/workspace/mf-toolchain/sysroot/usr/lib/x86_64-linux-gnu
EOF
. "$prefix/activate.sh"
cd /workspace/JerzyVirtualBreadboard
python tools/import_gui_assets.py --verify
cmake -S . -B build/baseline -G Ninja -DCMAKE_BUILD_TYPE=Release -DMF_GUI_ASSETS=ON -DBUILD_TESTING=ON
cmake --build build/baseline --parallel 3
