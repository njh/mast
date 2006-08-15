BUILT_SOURCES = VERSION

VERSION: configure.ac
	echo "${PACKAGE}" > VERSION; \
	echo "${VERSION}" >> VERSION

SUBDIRS = src doc
EXTRA_DIST = VERSION autogen.sh

