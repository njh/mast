BUILT_SOURCES = VERSION

VERSION: configure.in
	echo "${PACKAGE}" > VERSION; \
	echo "${VERSION}" >> VERSION

SUBDIRS = src scripts doc debian
EXTRA_DIST = VERSION bootstrap

