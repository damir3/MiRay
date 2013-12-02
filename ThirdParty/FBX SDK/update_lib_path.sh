#!/bin/bash
install_name_tool -id "@executable_path/../Frameworks/libfbxsdk.dylib" lib/clang/ub/release/libfbxsdk.dylib
otool -D lib/clang/ub/release/libfbxsdk.dylib

