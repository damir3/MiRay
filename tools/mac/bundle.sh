#!/bin/bash
set -euo pipefail

ARCH=${1:-arm}
BASE_DIR="bin/${ARCH}"
APP="${BASE_DIR}/MiRay.app"
FRAMEWORKS="${APP}/Contents/Frameworks"
RESOURCES="${APP}/Contents/Resources"

if [ ! -d "${APP}" ]; then
	echo "Error: ${APP} does not exist. Please build the project first."
	exit 1
fi

echo "==> Preparing macOS Bundle: ${APP}"

mkdir -p "${FRAMEWORKS}"
mkdir -p "${RESOURCES}"

# Copy dynamic libraries to Frameworks
cp "${BASE_DIR}"/lib*.dylib "${FRAMEWORKS}/" 2>/dev/null || true

# Ensure Application and Document icons are present in Resources
if [ -f "MiRay/res/icons/Application.icns" ]; then
	cp "MiRay/res/icons/Application.icns" "${RESOURCES}/"
fi
if [ -f "MiRay/res/icons/Document.icns" ]; then
	cp "MiRay/res/icons/Document.icns" "${RESOURCES}/"
fi

# Copy QML folder if available in bin
if [ -d "${BASE_DIR}/qml" ]; then
	echo "==> Copying QML runtime assets..."
	mkdir -p "${RESOURCES}/qml"
	cp -R "${BASE_DIR}/qml/"* "${RESOURCES}/qml/"
	find "${RESOURCES}/qml" -name '*.dSYM' -type d -exec rm -rf {} + 2>/dev/null || true
	find "${RESOURCES}/qml" -name '*.qmlc' -type f -delete 2>/dev/null || true
fi

# Check if executable was built in Debug or Release configuration
IS_DEBUG=0
if otool -L "${APP}/Contents/MacOS/MiRay" 2>/dev/null | grep -q "_debug"; then
	IS_DEBUG=1
fi

# Locate macdeployqt from active Conan package matching configuration
if [ "${IS_DEBUG}" -eq 1 ]; then
	QT_PACKAGE_DIR=$(grep "qt_PACKAGE_FOLDER_DEBUG" tmp/Qt5-*-data.cmake 2>/dev/null | head -n 1 | cut -d'"' -f2 || true)
else
	QT_PACKAGE_DIR=$(grep -E "qt_PACKAGE_FOLDER_(RELWITHDEBINFO|RELEASE)" tmp/Qt5-*-data.cmake 2>/dev/null | head -n 1 | cut -d'"' -f2 || true)
fi

if [ -z "${QT_PACKAGE_DIR}" ] || [ ! -d "${QT_PACKAGE_DIR}" ]; then
	QT_PACKAGE_DIR=$(grep "qt_PACKAGE_FOLDER_" tmp/Qt5-*-data.cmake 2>/dev/null | head -n 1 | cut -d'"' -f2 || true)
fi

if [ -z "${QT_PACKAGE_DIR}" ] || [ ! -d "${QT_PACKAGE_DIR}" ]; then
	QT_PACKAGE_DIR=$(conan list "qt/5.15.18:*" --format=json 2>/dev/null | python3 -c '
import sys, json, subprocess
try:
    data = json.load(sys.stdin)
    for rev, v in data.get("Local Cache", {}).get("qt/5.15.18", {}).get("revisions", {}).items():
        for pkg_id in v.get("packages", {}):
            p = subprocess.check_output(["conan", "cache", "path", f"qt/5.15.18:{pkg_id}"]).decode().strip()
            print(p)
            sys.exit(0)
except Exception:
    pass
' 2>/dev/null || true)
fi

if [ -n "${QT_PACKAGE_DIR}" ] && [ -x "${QT_PACKAGE_DIR}/bin/macdeployqt" ]; then
	MACDEPLOYQT="${QT_PACKAGE_DIR}/bin/macdeployqt"
else
	echo "Error: Could not locate Conan Qt5 macdeployqt. Please ensure qt/5.15.18 is installed."
	exit 1
fi

echo "==> Using macdeployqt: ${MACDEPLOYQT}"

# Temporarily rename .plugin to .dylib so macdeployqt properly fixes link paths
allplugins=$(ls "${FRAMEWORKS}" 2>/dev/null | grep '\.plugin$' || true)
for p in ${allplugins}; do
	name=$(basename "${p}" .plugin)
	mv "${FRAMEWORKS}/${p}" "${FRAMEWORKS}/${name}.dylib"
done

# Clean existing qt.conf before running macdeployqt, otherwise macdeployqt skips plugin deployment
rm -f "${RESOURCES}/qt.conf"

DEPLOY_FLAGS="-no-strip"
if [ "${IS_DEBUG}" -eq 1 ]; then
	DEPLOY_FLAGS="-use-debug-libs"
fi

echo "==> Running macdeployqt..."
"${MACDEPLOYQT}" "${APP}" ${DEPLOY_FLAGS}

# Restore .plugin extension
for p in ${allplugins}; do
	name=$(basename "${p}" .plugin)
	mv "${FRAMEWORKS}/${name}.dylib" "${FRAMEWORKS}/${p}"
done

# Deploy OpenImageDenoise device plugins and Intel TBB runtime (dlopen dependencies missed by macdeployqt)
echo "==> Deploying OpenImageDenoise device plugins and TBB..."
if [ "${IS_DEBUG}" -eq 1 ]; then
	OIDN_PACKAGE_DIR=$(grep "oidn_PACKAGE_FOLDER_DEBUG" tmp/OpenImageDenoise-*-data.cmake 2>/dev/null | head -n 1 | cut -d'"' -f2 || true)
else
	OIDN_PACKAGE_DIR=$(grep -E "oidn_PACKAGE_FOLDER_(RELWITHDEBINFO|RELEASE)" tmp/OpenImageDenoise-*-data.cmake 2>/dev/null | head -n 1 | cut -d'"' -f2 || true)
fi

if [ -z "${OIDN_PACKAGE_DIR}" ] || [ ! -d "${OIDN_PACKAGE_DIR}" ]; then
	OIDN_PACKAGE_DIR=$(grep "oidn_PACKAGE_FOLDER_" tmp/OpenImageDenoise-*-data.cmake 2>/dev/null | head -n 1 | cut -d'"' -f2 || true)
fi

if [ -n "${OIDN_PACKAGE_DIR}" ] && [ -d "${OIDN_PACKAGE_DIR}/lib" ]; then
	cp -P "${OIDN_PACKAGE_DIR}"/lib/libOpenImageDenoise_device* "${FRAMEWORKS}/" 2>/dev/null || true
	cp -P "${OIDN_PACKAGE_DIR}"/lib/libtbb* "${FRAMEWORKS}/" 2>/dev/null || true

	# Ensure major and unversioned symlinks exist for dynamic loader resolution
	for devlib in "${FRAMEWORKS}"/libOpenImageDenoise_device_*.dylib; do
		[ -f "${devlib}" ] || continue
		base=$(basename "${devlib}")
		if [[ "${base}" =~ ^(libOpenImageDenoise_device_[a-z0-9]+)\.([0-9]+)\.([0-9]+)\.([0-9]+)\.dylib$ ]]; then
			modname="${BASH_REMATCH[1]}"
			major="${BASH_REMATCH[2]}"
			ln -sf "${base}" "${FRAMEWORKS}/${modname}.${major}.dylib"
			ln -sf "${base}" "${FRAMEWORKS}/${modname}.dylib"
		fi
	done
fi

# Ensure standard qt.conf matching reference bundle
cat << 'EOF' > "${RESOURCES}/qt.conf"
[Paths]
Plugins = PlugIns
Imports = Resources/qml
Qml2Imports = Resources/qml
EOF

# Prune .framework directories (Conan Qt5 uses flat .dylibs, matching Koru)
rm -rf "${FRAMEWORKS}"/*.framework

# Prune PlugIns: keep only platforms, imageformats, and styles (matching Koru)
echo "==> Pruning unneeded Qt plugins..."
PLUGINS="${APP}/Contents/PlugIns"
if [ -d "${PLUGINS}" ]; then
	for p in "${PLUGINS}"/*; do
		[ -d "${p}" ] || continue
		pname=$(basename "${p}")
		if [ "${pname}" != "platforms" ] && [ "${pname}" != "imageformats" ] && [ "${pname}" != "styles" ]; then
			echo "    Removing plugin dir: ${pname}"
			rm -rf "${p}"
		fi
	done
fi

# Prune Resources/qml: keep only QtGraphicalEffects, QtQuick, QtQuick.2 (matching Koru)
echo "==> Pruning unneeded QML runtime modules..."
if [ -d "${RESOURCES}/qml" ]; then
	for q in "${RESOURCES}/qml"/*; do
		[ -d "${q}" ] || continue
		qname=$(basename "${q}")
		if [ "${qname}" != "QtGraphicalEffects" ] && [ "${qname}" != "QtQuick" ] && [ "${qname}" != "QtQuick.2" ]; then
			echo "    Removing QML dir: ${qname}"
			rm -rf "${q}"
		fi
	done
fi

# Remove debug Qt dylibs only if bundling a release build (e.g. from multi-config Conan packages)
if [ "${IS_DEBUG}" -eq 0 ]; then
	rm -f "${FRAMEWORKS}"/*_debug*.dylib
fi

# Remove dangling third-party dylibs if any exist
rm -f "${FRAMEWORKS}"/libsqlite3* "${FRAMEWORKS}"/libmng* "${FRAMEWORKS}"/libjasper* "${FRAMEWORKS}"/libicu* "${FRAMEWORKS}"/libglib* "${FRAMEWORKS}"/libgio* "${FRAMEWORKS}"/libgobject* "${FRAMEWORKS}"/libdbus*

# Touch bundle to prompt macOS Finder and LaunchServices to refresh icon cache
touch "${APP}"
touch "${APP}/Contents/Info.plist"
if [ -x "/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister" ]; then
	/System/Library/Frameworks/CoreServices.framework/Versions/A/Frameworks/LaunchServices.framework/Versions/A/Support/lsregister -f "${APP}" 2>/dev/null || true
fi

echo "==> Bundle preparation complete: ${APP}"


