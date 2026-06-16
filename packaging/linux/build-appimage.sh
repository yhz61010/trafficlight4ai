#!/usr/bin/env bash
set -euo pipefail

version="${1:?usage: build-appimage.sh <version> [output-dir]}"
output_dir="${2:-dist/release}"
package="trafficlight4ai"

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${repo_root}/build/package-appimage"
app_dir="${repo_root}/build/AppDir"
tool_dir="${repo_root}/build/appimage-tools"

rm -rf "${build_dir}" "${app_dir}" "${tool_dir}"
mkdir -p "${output_dir}" "${tool_dir}"

cmake -S "${repo_root}" -B "${build_dir}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTING=OFF \
    -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "${build_dir}" -j"$(nproc)"
DESTDIR="${app_dir}" cmake --install "${build_dir}" --strip

# Pinned versions and SHA256 checksums for reproducible builds.
# Update these when upgrading linuxdeploy.
LINUXDEPLOY_VER="1-alpha-20251107-1"
QT_PLUGIN_VER="1-alpha-20250213-1"
LINUXDEPLOY_SHA256="c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d"
QT_PLUGIN_SHA256="15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724"
RUNTIME_VER="20251108"
RUNTIME_SHA256="2fca8b443c92510f1483a883f60061ad09b46b978b2631c807cd873a47ec260d"

# Download a tool from pinned release, falling back to continuous on failure.
download_tool() {
    local output="$1" pinned_url="$2" continuous_url="$3" expected_sha="$4"

    if curl --fail --location --retry 3 --retry-all-errors --connect-timeout 20 -o "${output}" "${pinned_url}"; then
        if echo "${expected_sha}  ${output}" | sha256sum -c --strict 2>/dev/null; then
            echo "Downloaded pinned release: ${pinned_url##*/}"
            return 0
        fi
        echo "WARNING: SHA256 mismatch for pinned release, falling back to continuous"
    else
        echo "WARNING: Pinned release download failed, falling back to continuous"
    fi

    curl --fail --location --retry 3 --retry-all-errors --connect-timeout 20 -o "${output}" "${continuous_url}"
    echo "Downloaded continuous release: ${continuous_url##*/}"
}

linuxdeploy="${tool_dir}/linuxdeploy-x86_64.AppImage"
qt_plugin="${tool_dir}/linuxdeploy-plugin-qt-x86_64.AppImage"

download_tool "${linuxdeploy}" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/${LINUXDEPLOY_VER}/linuxdeploy-x86_64.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage" \
    "${LINUXDEPLOY_SHA256}"

download_tool "${qt_plugin}" \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/${QT_PLUGIN_VER}/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "https://github.com/linuxdeploy/linuxdeploy-plugin-qt/releases/download/continuous/linuxdeploy-plugin-qt-x86_64.AppImage" \
    "${QT_PLUGIN_SHA256}"

# Pre-download AppImage runtime so appimagetool doesn't fetch it at build time.
runtime="${tool_dir}/runtime-x86_64"
download_tool "${runtime}" \
    "https://github.com/AppImage/type2-runtime/releases/download/${RUNTIME_VER}/runtime-x86_64" \
    "https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64" \
    "${RUNTIME_SHA256}"

chmod +x "${linuxdeploy}" "${qt_plugin}"
ln -sf "${qt_plugin}" "${tool_dir}/linuxdeploy-plugin-qt"
export PATH="${tool_dir}:${PATH}"
export LDAI_RUNTIME_FILE="${runtime}"

if command -v qmake6 >/dev/null 2>&1; then
    export QMAKE="$(command -v qmake6)"
elif [ -x /usr/lib/qt6/bin/qmake ]; then
    export QMAKE=/usr/lib/qt6/bin/qmake
fi

export APPIMAGE_EXTRACT_AND_RUN=1

copy_library_dependency() {
    local source="$1"
    local lib_dir="${app_dir}/usr/lib"
    local dep dep_name target

    mkdir -p "${lib_dir}"
    while IFS= read -r dep; do
        dep_name="$(basename "${dep}")"
        case "${dep_name}" in
            ld-linux-*|libBrokenLocale.so.*|libanl.so.*|libc.so.*|libdl.so.*|libm.so.*|libmvec.so.*|libnsl.so.*|libpthread.so.*|libresolv.so.*|librt.so.*|libthread_db.so.*|libutil.so.*)
                continue
                ;;
        esac

        target="${lib_dir}/${dep_name}"
        if [ -e "${target}" ]; then
            continue
        fi

        cp -L "${dep}" "${target}"
        chmod u+w "${target}" 2>/dev/null || true
        copy_library_dependency "${target}"
    done < <(ldd "${source}" 2>/dev/null | awk '/=> \// { print $3 }')
}

# --- Qt multimedia plugin ---
# linuxdeploy-plugin-qt only deploys plugins the executable links directly.
# QMediaPlayer loads the multimedia backend at runtime; EXTRA_QT_PLUGINS is
# deprecated in current linuxdeploy-plugin-qt, so copy manually.
qt_plugin_dir="${QMAKE:+$(${QMAKE} -query QT_INSTALL_PLUGINS 2>/dev/null)}"
qt_plugin_dir="${qt_plugin_dir:-/usr/lib/x86_64-linux-gnu/qt6/plugins}"
if [ -d "${qt_plugin_dir}/multimedia" ]; then
    mkdir -p "${app_dir}/usr/plugins/multimedia"
    for so in "${qt_plugin_dir}/multimedia/"*.so; do
        [ -f "${so}" ] || continue
        cp -a "${so}" "${app_dir}/usr/plugins/multimedia/"
        copy_library_dependency "${app_dir}/usr/plugins/multimedia/$(basename "${so}")"
    done
    echo "Bundled Qt multimedia plugins: $(ls "${app_dir}/usr/plugins/multimedia/" 2>/dev/null | tr '\n' ' ')"
else
    echo "WARNING: Qt multimedia plugin directory not found at ${qt_plugin_dir}/multimedia" >&2
fi

# --- GStreamer plugins for OGG playback ---
gst_dir="/usr/lib/x86_64-linux-gnu/gstreamer-1.0"
if [ -d "${gst_dir}" ]; then
    gst_target="${app_dir}/usr/lib/gstreamer-1.0"
    mkdir -p "${gst_target}"
    for plugin in libgstcoreelements libgsttypefindfunctions libgstplayback \
                  libgstogg libgstvorbis libgstaudioconvert libgstaudioresample \
                  libgstvolume libgstautodetect libgstapp libgstvideoconvert libgstvideoscale \
                  libgstgio libgstpulseaudio libgstalsa; do
        if [ -f "${gst_dir}/${plugin}.so" ]; then
            cp -a "${gst_dir}/${plugin}.so" "${gst_target}/"
            copy_library_dependency "${gst_target}/${plugin}.so"
        fi
    done
    echo "Bundled GStreamer plugins: $(ls "${gst_target}" 2>/dev/null | tr '\n' ' ')"
fi

gst_scanner="/usr/lib/x86_64-linux-gnu/gstreamer1.0/gstreamer-1.0/gst-plugin-scanner"
if [ -x "${gst_scanner}" ]; then
    gst_scanner_target="${app_dir}/usr/libexec/gstreamer-1.0"
    mkdir -p "${gst_scanner_target}"
    cp -a "${gst_scanner}" "${gst_scanner_target}/"
    copy_library_dependency "${gst_scanner_target}/gst-plugin-scanner"
fi

"${linuxdeploy}" \
    --appdir "${app_dir}" \
    --executable "${app_dir}/usr/bin/${package}" \
    --executable "${app_dir}/usr/bin/tl4ai-ctl" \
    --desktop-file "${app_dir}/usr/share/applications/${package}.desktop" \
    --icon-file "${app_dir}/usr/share/pixmaps/${package}.png" \
    --plugin qt

if [ -d "${app_dir}/usr/lib/gstreamer-1.0" ]; then
    find "${app_dir}/usr/lib/gstreamer-1.0" -name '*.so' \
        -exec patchelf --set-rpath '$ORIGIN:$ORIGIN/..' {} \;
fi
if [ -x "${app_dir}/usr/libexec/gstreamer-1.0/gst-plugin-scanner" ]; then
    patchelf --set-rpath '$ORIGIN/../../lib:$ORIGIN' \
        "${app_dir}/usr/libexec/gstreamer-1.0/gst-plugin-scanner"
fi

export OUTPUT="${output_dir}/${package}-${version}-linux-amd64.AppImage"
export LDAI_OUTPUT="${OUTPUT}"
"${linuxdeploy}" \
    --appdir "${app_dir}" \
    --output appimage

# --- Post-build verification ---
echo "=== Verifying AppImage contents ==="
qt_multimedia_dir="${app_dir}/usr/plugins/multimedia"
gst_plugins_dir="${app_dir}/usr/lib/gstreamer-1.0"
gst_scanner_path="${app_dir}/usr/libexec/gstreamer-1.0/gst-plugin-scanner"
if [ -d "${qt_multimedia_dir}" ] && [ -n "$(find "${qt_multimedia_dir}" -maxdepth 1 -name '*.so' -print -quit)" ]; then
    echo "Qt multimedia plugins: $(ls "${qt_multimedia_dir}")"
else
    echo "No Qt multimedia plugin directory found; relying on bundled GStreamer plugins."
fi

if [ ! -d "${gst_plugins_dir}" ] || [ -z "$(find "${gst_plugins_dir}" -maxdepth 1 -name '*.so' -print -quit)" ]; then
    echo "FATAL: no GStreamer plugins in AppDir" >&2
    exit 1
fi
echo "GStreamer plugins: $(ls "${gst_plugins_dir}")"
for required_plugin in libgstcoreelements.so libgsttypefindfunctions.so libgstplayback.so \
                       libgstogg.so libgstvorbis.so libgstaudioconvert.so \
                       libgstaudioresample.so libgstvolume.so libgstautodetect.so \
                       libgstapp.so libgstvideoconvert.so; do
    if [ ! -f "${gst_plugins_dir}/${required_plugin}" ]; then
        echo "FATAL: missing required GStreamer plugin ${required_plugin}" >&2
        exit 1
    fi
done
if [ ! -x "${gst_scanner_path}" ]; then
    echo "FATAL: missing bundled GStreamer plugin scanner" >&2
    exit 1
fi

dep_dirs=("${gst_plugins_dir}" "${gst_scanner_path}")
if [ -d "${qt_multimedia_dir}" ]; then
    dep_dirs+=("${qt_multimedia_dir}")
fi
missing_deps="$(find "${dep_dirs[@]}" -name '*.so' -exec ldd {} \; 2>/dev/null | grep 'not found' || true)"
if [ -n "${missing_deps}" ]; then
    echo "FATAL: some bundled libraries have missing dependencies:" >&2
    echo "${missing_deps}"
    exit 1
fi
