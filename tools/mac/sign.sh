#!/bin/bash
set -euo pipefail

ARCH=${1:-arm}
APP="bin/${ARCH}/MiRay.app"
IDENTITY=${DEVELOPER_ID:-""}
ENTITLEMENTS="MiRay/MiRay.entitlements"

if [ ! -d "${APP}" ]; then
	echo "Error: ${APP} not found."
	exit 1
fi

if [ -z "${IDENTITY}" ]; then
	echo "Warning: DEVELOPER_ID is not set. Skipping code signing."
	exit 0
fi

echo "==> Signing ${APP} with identity: ${IDENTITY}"

# Sign all nested dynamic libraries and plugins across Contents (Frameworks, PlugIns, Resources/qml)
find "${APP}/Contents" -type f \( -name "*.dylib" -o -name "*.plugin" -o -name "*.so" \) -exec \
	codesign -f --options runtime --timestamp --entitlements "${ENTITLEMENTS}" -s "${IDENTITY}" -v {} +

if [ -d "${APP}/Contents/Frameworks" ]; then
	find "${APP}/Contents/Frameworks" -depth -name "*.framework" -exec \
		codesign -f --options runtime --timestamp --entitlements "${ENTITLEMENTS}" -s "${IDENTITY}" -v {} +
fi

# Sign main executable and bundle
codesign -f --options runtime --timestamp --entitlements "${ENTITLEMENTS}" -s "${IDENTITY}" -v "${APP}/Contents/MacOS/MiRay"
codesign -f --options runtime --timestamp --entitlements "${ENTITLEMENTS}" -s "${IDENTITY}" -v "${APP}"

echo "==> Verifying signature..."
codesign --verify --verbose=4 --deep --strict "${APP}"
spctl --assess --verbose=4 --type=execute "${APP}" || true

echo "==> Code signing finished successfully."
