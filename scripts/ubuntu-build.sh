#!/bin/bash
# set -x
set -e
usage () {
    echo "$0 [<-s|--sersfx> <numerical_series_suffix>] [-d|--dev]"
    exit $1
}

#
#  MAIN
#
BDIR=build-ubuntu
SER_SFX=1
# a development release build automatically the new 'changelog' chapter
# an official release use 'changelog' as is except for change serie name
IS_DEV_RELEASE=n
while [ "$1" != "" ]; do
    case $1 in
        -d|--dev)
            IS_DEV_RELEASE=y
            ;;
        -s|--sersfx)
            SER_SFX=$2
            shift
            ;;
        *)
            usage 1
            ;;
    esac
    shift
done

if [ "$DEBEMAIL" == "" -o "$DEBFULLNAME" == "" ]; then
    echo "DEBEMAIL and DEBFULLNAME variables must be set before run this script"
    exit 1
fi

if ! gpg --list-secret-keys "${DEBFULLNAME} <${DEBEMAIL}>" >/dev/null 2>&1 ; then
    echo "gpg secret key not found for '${DEBFULLNAME} <${DEBEMAIL}>' address"
    exit 2
fi

PKG_NAME=remmina
PKG_DATE="$(date -R)"
VER_MAJ="$(grep 'set('"${PKG_NAME^^}"'_VERSION_MAJOR' CMakeLists.txt \
               | sed 's/set('"${PKG_NAME^^}"'_VERSION_[^"]*"//g;s/".*//g')"
VER_MIN="$(grep 'set('"${PKG_NAME^^}"'_VERSION_MINOR' CMakeLists.txt \
               | sed 's/set('"${PKG_NAME^^}"'_VERSION_[^"]*"//g;s/".*//g')"
VER_REV="$(grep 'set('"${PKG_NAME^^}"'_VERSION_REVISION' CMakeLists.txt \
               | sed 's/set('"${PKG_NAME^^}"'_VERSION_[^"]*"//g;s/".*//g')"
VER_SFX="$(grep 'set('"${PKG_NAME^^}"'_VERSION_SUFFIX' CMakeLists.txt \
               | sed 's/set('"${PKG_NAME^^}"'_VERSION_[^"]*"//g;s/".*//g')"
VER_BRANCH="$(git branch | grep '^\*' | cut -c 3-)"
test "$VER_BRANCH" && VER_BRANCH="+${VER_BRANCH}"
VER_DATE="$(date -u -d "$(git log -1 --date=iso  | grep '^Date:' | sed 's/^Date: *//g')" "+%Y%m%d%H%M")"

PKG_VER="${VER_MAJ}.${VER_MIN}.${VER_REV}${VER_SFX}${VER_BRANCH}+${VER_DATE}"
PKG_DIR="${PKG_NAME}_${PKG_VER}"

REMMINA_SOURCE_DATE=$(date -d "$(git log --date=rfc --pretty=format:%ad -1)" +%Y%m%d%H%M)
REMMINA_SOURCE_HASH=$(git log -1 | grep "^commit" | sed "s/^commit *\(.......\).*/\1/g")

echo $PKG_VER

mkdir -p ${BDIR}
rm -rf ${BDIR}/*

# exports repo without .git folder and other operative system clients
git archive --format tar --prefix "${BDIR}/${PKG_DIR}/" HEAD | \
    tar xv

# Override original ChangeLog with git logs (maybe necessary for debian policy ?
git --no-pager log --format="%ai %aN (%h) %n%n%x09*%w(68,0,10) %s%d%n" > "${BDIR}/${PKG_DIR}/ChangeLog"

# NOTE: artificially files date reconstruction is skipped


mv ${BDIR}/${PKG_DIR}/debian/changelog.in ${BDIR}/changelog.in.orig
cd ${BDIR}/${PKG_DIR}/
tar zcvf "../${PKG_NAME}_${PKG_VER}.orig.tar.gz" .
for serie in yakkety wily xenial trusty; do
    if [ "$IS_DEV_RELEASE" = "y" ]; then
        cat <<EOF >debian/changelog.in
${PKG_NAME} (${PKG_VER}-1${serie}${SER_SFX}) ${serie}; urgency=medium

  * New upstream release.

 -- ${DEBFULLNAME} <${DEBEMAIL}>  ${PKG_DATE}

EOF
    else
        rm -f debian/changelog.in
        touch debian/changelog.in
    fi

    cat ../changelog.in.orig >>debian/changelog.in
    ./debian/rules debian/changelog REMMINA_SOURCE_DATE=$REMMINA_SOURCE_DATE REMMINA_SOURCE_HASH=$REMMINA_SOURCE_HASH

    debuild -eUBUNTU_SERIE="$serie" -us -uc -S -sa # add ' -us -uc' flags to avoid signing
    rm -rf *
    tar zxf "../${PKG_NAME}_${PKG_VER}.orig.tar.gz"
done
cd -
mv ${BDIR}/changelog.in.orig ${BDIR}/${PKG_DIR}/debian/changelog.in

echo "now cd in ${BDIR} directory and run:"
echo "dput <your-ppa-address> *.changes"

exit 0
