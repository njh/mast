BUILT_SOURCES = VERSION

VERSION: configure.in
	echo "${PACKAGE}" > VERSION; \
	echo "${VERSION}" >> VERSION

SUBDIRS = src doc
EXTRA_DIST = VERSION bootstrap

