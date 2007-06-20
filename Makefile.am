BUILT_SOURCES = VERSION

VERSION: configure.ac
	echo "${PACKAGE_NAME}" > VERSION; \
	echo "${PACKAGE_VERSION}" >> VERSION

SUBDIRS = libgsm liblpc10 src @BUILD_DOC@
EXTRA_DIST = VERSION autogen.sh

