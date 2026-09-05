#!/usr/bin/env python3

import argparse
import fnmatch
import os
import subprocess
import sys


def get_default_executable(this_path):
	exe = "miray.exe"
	if os.name == "posix":
		exe = "MiRay.app/Contents/MacOS/MiRay"

	# Try arm first, then x86_64, then bin root
	candidates = [
		os.path.normpath(os.path.join(this_path, f"../bin/arm/{exe}")),
		os.path.normpath(os.path.join(this_path, f"../bin/x86_64/{exe}")),
		os.path.normpath(os.path.join(this_path, f"../bin/{exe}")),
	]
	for c in candidates:
		if os.path.exists(c):
			return c
	return candidates[0]


def main():
	this_path = os.path.dirname(os.path.realpath(__file__))
	full_path = os.path.join(this_path, "Library/Materials")

	scene_default = os.path.join(this_path, "Preview/Ball.mirayScene")
	scene_dark = os.path.join(this_path, "Preview/Dark.mirayScene")
	scene_sss = os.path.join(this_path, "Preview/SSS.mirayScene")

	parser = argparse.ArgumentParser(description="Update material thumbnails.")
	parser.add_argument("filter", nargs="?", default="", help="Optional filter substring for material path")
	parser.add_argument("-f", "--filter", dest="filter_opt", default=None, help="Optional filter substring for material path")
	parser.add_argument("-p", "--passes", type=int, default=None, help="Number of rendering passes for thumbnail (default: 500 for SSS, 250 for others)")
	parser.add_argument("--scene", default=None, help="Override preview scene file")
	parser.add_argument("--exe", default=None, help="Path to MiRay executable")

	args = parser.parse_args()

	console = os.path.abspath(args.exe) if args.exe else get_default_executable(this_path)
	print("Our main executable is %s" % (console))

	if not os.path.exists(console):
		print("Error: Executable '%s' does not exist." % (console), file=sys.stderr)
		sys.exit(1)

	material_filter = (args.filter_opt or args.filter or "").lower()

	materials = []
	for root, dirnames, filenames in os.walk(full_path):
		for filename in fnmatch.filter(filenames, "*.mirayMaterial"):
			materials.append(os.path.join(root, filename))

	for m in materials:
		m_lower = m.lower()

		if material_filter:
			if m_lower.find(material_filter) == -1:
				continue

		if args.scene:
			scene = os.path.abspath(args.scene) if os.path.exists(args.scene) else os.path.join(this_path, args.scene)
		else:
			scene = scene_default
			if "/light/" in m_lower or "\\light\\" in m_lower:
				scene = scene_dark
			if "/glass/" in m_lower or "\\glass\\" in m_lower:
				scene = scene_sss
			if "/liquid/" in m_lower or "\\liquid\\" in m_lower:
				scene = scene_sss

		passes = args.passes
		if passes is None or passes <= 0:
			passes = 500 if scene == scene_sss else 250

		print("processing material '%s' (%d passes)" % (m, passes))

		params = [console, "--thumbnail", "--passes=%d" % passes, m, scene]

		p = subprocess.Popen(params, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
		(so, se) = p.communicate()
		ret = p.wait()

		if ret != 0:
			print("=== stdout ===\n%s\n\n=== stderr ===\n%s\n" % (so, se))
			sys.exit(2)

	print("All Done")


if __name__ == "__main__":
	main()