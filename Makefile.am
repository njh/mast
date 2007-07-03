BUILT_SOURCES = VERSION

VERSION: configure.ac
	echo "${PACKAGE_NAME}" > VERSION; \
	echo "${PACKAGE_VERSION}" >> VERSION

SUBDIRS = libgsm src doc
EXTRA_DIST = VERSION TODO autogen.sh

