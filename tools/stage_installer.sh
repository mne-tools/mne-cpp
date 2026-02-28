#!/bin/bash
set -e

cd "$(dirname "$0")/.."
BASE="$PWD"
OUT="$BASE/out/Release"
STAGING="$BASE/tools/packaging/installer/packages"

echo "BASE=$BASE"
echo "OUT=$OUT"
echo "STAGING=$STAGING"

# Clean existing data dirs
echo "=== Cleaning old staged data ==="
for pkg_dir in "$STAGING"/*/; do
  if [ -d "$pkg_dir/data" ]; then
    rm -rf "$pkg_dir/data"
  fi
done

CLI_APPS="mne_anonymize mne_edf2fiff mne_show_fiff mne_compute_raw_inverse mne_compute_mne mne_inverse_operator mne_make_source_space mne_forward_solution mne_setup_mri mne_surf2bem mne_setup_forward_model mne_rt_server mne_dipole_fit mne_process_raw"
GUI_APPS="mne_scan mne_analyze mne_browse mne_inspect"

# --- CLI Tools ---
echo "=== Staging CLI apps ==="
CLI_DATA="${STAGING}/org.mnecpp.applications.cli/data"
mkdir -p "${CLI_DATA}/bin"
for app in $CLI_APPS; do
  if [ -f "${OUT}/apps/$app" ]; then
    cp "${OUT}/apps/$app" "${CLI_DATA}/bin/"
    echo "  $app (binary)"
  elif [ -d "${OUT}/apps/${app}.app" ]; then
    cp "${OUT}/apps/${app}.app/Contents/MacOS/$app" "${CLI_DATA}/bin/"
    echo "  $app (from .app)"
  fi
done
if [ -d "${OUT}/apps/mne_rt_server_plugins" ]; then
  cp -r "${OUT}/apps/mne_rt_server_plugins" "${CLI_DATA}/bin/"
  echo "  mne_rt_server_plugins/"
fi

# --- GUI Apps ---
echo "=== Staging GUI apps ==="
GUI_DATA="${STAGING}/org.mnecpp.applications.gui/data"
mkdir -p "${GUI_DATA}/bin"
for app in $GUI_APPS; do
  if [ -d "${OUT}/apps/${app}.app" ]; then
    # Use rsync to avoid symlink cycles (deploy.bat copies resources as symlinks)
    rsync -a --copy-links --exclude='resources/resources' \
      "${OUT}/apps/${app}.app/" "${GUI_DATA}/bin/${app}.app/"
    echo "  ${app}.app"
  fi
done

# --- Runtime libs ---
echo "=== Staging runtime libs ==="
RT_DATA="${STAGING}/org.mnecpp.runtime/data"
mkdir -p "${RT_DATA}/lib"
cp -r "${OUT}/lib/"* "${RT_DATA}/lib/" 2>/dev/null || true

# --- Qt plugins for CLI tools ---
# CLI tools are not .app bundles and need QT_PLUGIN_PATH to find platform plugins.
# The configure_environment script sets QT_PLUGIN_PATH to $INSTALL_DIR/lib/plugins.
echo "=== Staging Qt plugins for CLI tools ==="
QT_PLUGINS_SRC=""
if [ -n "${QT_ROOT_DIR}" ] && [ -d "${QT_ROOT_DIR}/plugins" ]; then
  QT_PLUGINS_SRC="${QT_ROOT_DIR}/plugins"
elif [ -n "${Qt6_DIR}" ] && [ -d "$(cd "${Qt6_DIR}/../../.." 2>/dev/null && pwd)/plugins" ]; then
  QT_PLUGINS_SRC="$(cd "${Qt6_DIR}/../../.." 2>/dev/null && pwd)/plugins"
elif [ -d "$HOME/Qt/6.10.2/macos/plugins" ]; then
  QT_PLUGINS_SRC="$HOME/Qt/6.10.2/macos/plugins"
fi
if [ -n "${QT_PLUGINS_SRC}" ] && [ -d "${QT_PLUGINS_SRC}" ]; then
  mkdir -p "${RT_DATA}/lib/plugins"
  for plugdir in platforms imageformats iconengines networkinformation tls styles; do
    if [ -d "${QT_PLUGINS_SRC}/${plugdir}" ]; then
      cp -r "${QT_PLUGINS_SRC}/${plugdir}" "${RT_DATA}/lib/plugins/"
      echo "  Copied ${plugdir}"
    fi
  done
else
  echo "  WARNING: Qt plugins directory not found. CLI tools may not work without QT_PLUGIN_PATH."
fi

# --- SDK (headers) ---
echo "=== Staging SDK ==="
SDK_DATA="${STAGING}/org.mnecpp.sdk/data"
mkdir -p "${SDK_DATA}/include"
for lib_dir in src/libraries/*/; do
  lib_name=$(basename "$lib_dir")
  [ "$lib_name" = "CMakeLists.txt" ] && continue
  find "$lib_dir" -name '*.h' | while read hdr; do
    rel="${hdr#src/libraries/}"
    dest_dir="${SDK_DATA}/include/$(dirname "$rel")"
    mkdir -p "$dest_dir"
    cp "$hdr" "$dest_dir/"
  done
done

# --- Scripts ---
echo "=== Staging scripts ==="
for comp in org.mnecpp.sampledata org.mnecpp.mnepython org.mnecpp.pathconfig; do
  mkdir -p "${STAGING}/${comp}/data/scripts"
done
cp tools/packaging/scripts/download_sample_data.sh  "${STAGING}/org.mnecpp.sampledata/data/scripts/"
cp tools/packaging/scripts/download_sample_data.bat  "${STAGING}/org.mnecpp.sampledata/data/scripts/"
cp tools/packaging/scripts/install_mne_python.sh     "${STAGING}/org.mnecpp.mnepython/data/scripts/"
cp tools/packaging/scripts/install_mne_python.bat    "${STAGING}/org.mnecpp.mnepython/data/scripts/"
cp tools/packaging/scripts/configure_environment.sh  "${STAGING}/org.mnecpp.pathconfig/data/scripts/"
cp tools/packaging/scripts/configure_environment.bat "${STAGING}/org.mnecpp.pathconfig/data/scripts/"

echo ""
echo "=== Component sizes ==="
du -sh "${CLI_DATA}" "${GUI_DATA}" "${RT_DATA}" "${SDK_DATA}"
echo ""
echo "Qt frameworks in GUI bundles:"
find "${GUI_DATA}" -name "Qt*.framework" -type d 2>/dev/null | wc -l
echo "Qt frameworks in runtime/lib:"
find "${RT_DATA}" -name "Qt*.framework" -type d 2>/dev/null | wc -l
echo ""
echo "=== Staging complete ==="
