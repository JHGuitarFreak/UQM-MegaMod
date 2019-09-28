#!/bin/sh
# Script for creating self-extracting installer files for unix-like systems.
# By Serge van den Boom, 2003-02-20

TEMPDIR="/tmp/buildinstaller_$$"

if [ $# -ne 2 ]; then
	cat >&2 << EOF
Usage: buildinstaller.sh <installername> <template>
where 'installername' is the name you want to give the final installer
and 'template' is the template file describing the installer.
EOF
	exit 1
fi
DESTFILE="$1"
TEMPLATE="$2"

if [ ! -d src -o ! -d build ]; then
cat >&2 << EOF
Error: The current directory should be the top of the cvs tree.
       Please try again from that dir
EOF
	exit 1
fi

if [ ! -f uqm ]; then
	cat >&2 << EOF
Error: There should be an 'uqm' binary in the top of the cvs tree.
       please recompile and try again.
EOF
	exit 1
fi

mkdir "$TEMPDIR" 2> /dev/null

if [ "$?" -ne 0 ]; then
	echo "Could not create temporary dir." >&2
	exit 1
fi

. "$TEMPLATE"

while read FILE DEST; do
	DESTDIR="${TEMPDIR}/attach/${DEST%/*}"
	if [ ! -d "$DESTDIR" ]; then
		mkdir -p -- "$DESTDIR" || exit 1
	fi
	cp -- "$FILE" "${TEMPDIR}/attach/$DEST" || exit 1
done << EOF
$UQM_ATTACH_FILES
EOF

chmod -R go+rX "${TEMPDIR}/attach"

echo "Making tar.gz file for everything except from the content."
echo "Using maximum compression; this may take a moment."
tar -c -C "${TEMPDIR}/attach/" -f - . | gzip -9 > "${TEMPDIR}/attach.tar.gz"

{
	cat << EOF
UQM_VERSION="$UQM_VERSION"
UQM_PACKAGES="$UQM_PACKAGES"
EOF
	for PACKAGE in $UQM_PACKAGES; do
		eval PACKAGE_NAME="\$UQM_PACKAGE_${PACKAGE}_NAME"
		eval PACKAGE_TITLE="\$UQM_PACKAGE_${PACKAGE}_TITLE"
		eval PACKAGE_LOCATION="\$UQM_PACKAGE_${PACKAGE}_LOCATION"
		eval PACKAGE_OPTIONAL="\$UQM_PACKAGE_${PACKAGE}_OPTIONAL"
		eval PACKAGE_DEFAULT="\$UQM_PACKAGE_${PACKAGE}_DEFAULT"
		cat << EOF
UQM_PACKAGE_${PACKAGE}_NAME="$PACKAGE_NAME"
UQM_PACKAGE_${PACKAGE}_TITLE="$PACKAGE_TITLE"
UQM_PACKAGE_${PACKAGE}_LOCATION="$PACKAGE_LOCATION"
UQM_PACKAGE_${PACKAGE}_OPTIONAL="$PACKAGE_OPTIONAL"
UQM_PACKAGE_${PACKAGE}_DEFAULT="$PACKAGE_DEFAULT"
EOF
	done
} > "${TEMPDIR}/packages"

# A slow way, but a reliable way.
ATTACHLEN=`wc -c < "${TEMPDIR}/attach.tar.gz"`

LICENSE_TEXT="$(cat $UQM_LICENSE_FILE)"

# Now we've got all the parts, we can make the final .sh file.
echo "Making final executable."
{
	for SCRIPT in ${TEMPDIR}/packages $UQM_SCRIPT_FILES; do
		echo "# --- Start ${SCRIPT##*/} ---"
		# Very ugly, but I can't get sed to replace a pattern by the contents
		# of a file.
		FILEDATA="$(cat $SCRIPT)"
		sed -e "s/@ATTACHLEN@/$ATTACHLEN/" \
				-e "s/@CONTENT_FILES@/$CONTENT_FILES/" << EOF
${FILEDATA/@LICENCE@/$LICENSE_TEXT}
EOF
		echo "# --- End ${SCRIPT##*/} ---"
	done
	cat "${TEMPDIR}/attach.tar.gz"
} > "$DESTFILE"

chmod 755 "$DESTFILE"

rm -r -- "$TEMPDIR"

echo "Done."

