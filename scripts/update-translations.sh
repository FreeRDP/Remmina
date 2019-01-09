#!/bin/bash -

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
REMMINATOP="$(dirname "$SCRIPTPATH")"

cd "$REMMINATOP" || exit 1

if ! find src plugins -name "*.c" | sed 's/^.\///' >| po/POTFILES.in ; then
    exit 1
fi
if ! find data -name "*.glade" | sed 's/^.\///' >> po/POTFILES.in ; then
    exit 1
fi
if ! find data -name "*.appdata.xml" | sed 's/^.\///' >> po/POTFILES.in ; then
    exit 1
fi

xgettext --from-code=UTF-8 -k_ -kN_ -ktranslatable \
    --keyword=C_:1c,2 --keyword=NC_:1c,2 \
    --keyword=g_dngettext:2,3 \
    --add-comments \
    --files-from=po/POTFILES.in \
    --output=po/messages.pot \
    --copyright-holder="2014-2019 Antenore Gatta, Giovanni Panozzo" \
    --package-name="Remmina" --package-version="1.2.31" \
    --msgid-bugs-address="admin@remmina.org"

cd "$REMMINATOP"/po || exit 1

for i in *.po; do
    msgmerge -N --backup=off --update "$i" messages.pot
done

for i in "$REMMINATOP"/po/*.po ; do
    TMPF=/tmp/f$$.txt
    sed '/^#~/d' "$i" > "$TMPF"
    awk 'BEGIN{bl=0}/^$/{bl++;if(bl==1)print;else next}/^..*$/{bl=0;print}' $TMPF >| "$i"
    rm $TMPF
done

rm messages.pot
