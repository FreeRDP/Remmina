#!/bin/bash -

# --------------------------------------------------------------------------
# Remmina - The GTK+ Remote Desktop Client
# Copyright (C) 2016-2020 Antenore Gatta, Giovanni Panozzo
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
REMMINATOP="$(dirname "$SCRIPTPATH")"


#===============================================================================
#  FUNCTION DEFINITIONS
#===============================================================================

rem_varhasvalue () {
	if [[ -n ${!1:-} ]]; then
		return 0
	fi
	return 1

}	# ----------  end of function rem_varhasvalue  ----------

rem_varisdefined () {
	typeset -p ${1:-} >/dev/null 2>&1         # Not portable, bash specific
}	# ----------  end of function rem_varisdefined  ----------

rem_log () {
	local _cmnhead="${HOSTNAME:=$(hostname)}"
	local _header=""
	local _message="$*"
	#local _stdout=""
	local _msgdate=""
	case "$1" in
		CRITICAL)
			_header="CRITICAL"
			shift
			_message="$*"
			;;
		ERROR)
			_header="ERROR"
			shift
			_message="$*"
			;;
		WARNING)
			_header="WARNING"
			shift
			_message="$*"
			;;
		DEBUG)
			_header="DEBUG"
			shift
			_message="$*"
			;;
		INFO)
			# We can add color support adding colors in the beginning
			# GREEN="\033[0;32m"
			# RESET="\033[0m"
			# _reset=${RESET:-'\033[0m'}
			# _color=${_reset}
			#_color=${GREEN}
			_header="INFO"
			shift
			_message="$*"
			;;
		*)
			_header="INFO"
			_message="$*"
			;;
	esac
	if ! rem_varisdefined DFORMAT ; then
		local _dateformat='%d/%m/%y %H:%M:%S'
	else
		local _dateformat=${DFORMAT:-}
	fi
	_msgdate="$(date +"$_dateformat")"

	# printf "%s%s - [%s] - %s - %s%s\n" "$_color" "$_header" "$_msgdate" "${_cmnhead}" "$_message" "$_reset"
	printf "%s - [%s] - %s - %s\n" "$_header" "$_msgdate" "${_cmnhead}" "$_message"

}	# ----------  end of function rem_log  ----------
#-------------------------------------------------------------------------------
# rem_which a poorman which function
# Return 0 un success or 1 in case of failure
rem_which () {
	local _tool=()
	local _ret=
	for _tool in "$@" ; do
		if type "$_tool" >/dev/null 2>&1 ; then
			_ret=0
		else
			_ret=1
		fi
		case $_ret in
			0)
				rem_log INFO "$_tool found"
				;;
			83)
				rem_log ERROR "$_tool not found"
				;;
		esac
	done
	unset _tool
	return "$_ret"
}	# ----------  end of function rem_which  ----------

if ! rem_which "xgettext" "msgmerge" "git" "diff" ; then
	rem_log ERROR "Some tools have not been found"
	exit 1
fi

cd "$REMMINATOP" || exit 1

GIT_TAG="$(git describe --abbrev=0 )"
rem_log INFO "GIT_TAG is set to $GIT_TAG"

if ! rem_varhasvalue GIT_TAG ; then
	rem_log ERROR "GIT_TAG is either empty or not set. Probably you are not in a git repository"
	exit 1
fi


if ! find src plugins -name "*\.c" -o -name "*\.h" | sed 's/^.\///'  >| po/POTFILES.in ; then
	exit 1
fi
if ! find data -name "*.glade" | sed 's/^.\///' >> po/POTFILES.in ; then
	exit 1
fi

xgettext --from-code=UTF-8 \
	--keyword=_ \
	--keyword=N_ \
	--keyword=translatable \
	--keyword=C_:1c,2 \
	--keyword=NC_:1c,2 \
	--keyword=g_dngettext:2,3 \
	--add-comments=TRANSLATORS: \
	--files-from=po/POTFILES.in \
	--output=po/remmina.temp.pot \
	--package-version="$GIT_TAG" \
	--package-name="Remmina" \
	--msgid-bugs-address="l10n@lists.remmina.org"

cd "$REMMINATOP"/po || exit 1

# Set charset to UTF-8
sed -i -e 's/charset=CHARSET/charset=UTF-8/g' remmina.temp.pot

if diff -qI "POT-Creation-Date" remmina.temp.pot remmina.pot ; then
	rem_log INFO "No new strings to be translated"
	rm remmina.temp.pot
	exit 0
fi

mv remmina.temp.pot remmina.pot

for i in *.po; do
	msgmerge --backup=off --update "$i" remmina.pot
done

for i in "$REMMINATOP"/po/*.po ; do
	TMPF=/tmp/f$$.txt
	sed '/^#~/d' "$i" > "$TMPF"
	awk 'BEGIN{bl=0}/^$/{bl++;if(bl==1)print;else next}/^..*$/{bl=0;print}' $TMPF >| "$i"
	rm $TMPF
done
