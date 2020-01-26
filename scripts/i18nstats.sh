#!/bin/bash -
#===============================================================================
#
#          FILE: i18nstats.sh
#
#         USAGE: ./i18nstats.sh
#
#   DESCRIPTION:
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Antenore Gatta (tmow), antenore@simbiosi.org
#  ORGANIZATION: Remmina
#       CREATED: 30. 01. 19 00:05:25
#       LICENSE: GPLv2
#      REVISION:  ---
#===============================================================================

set -o nounset                        # Treat unset variables as an error

SCRIPTPATH="$( cd "$(dirname "$0")" ; pwd -P )"
REMMINATOP="$(dirname "$SCRIPTPATH")"
REMTMPDIR="$(mktemp -d)"
REMTMPFILE="$(mktemp -p "$REMTMPDIR")"

trap "rm -rf $REMTMPDIR" HUP INT QUIT TERM EXIT

declare -x TRANSLATED
declare -x UNTRANSLATED
declare -x FUZZY

#===============================================================================
#  FUNCTION DEFINITIONS
#===============================================================================

#-------------------------------------------------------------------------------
# TODO: Move these functions in an external library file
#-------------------------------------------------------------------------------

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
			1)
				rem_log ERROR "$_tool not found"
				;;
		esac
	done
	unset _tool
	return "$_ret"
}	# ----------  end of function rem_which  ----------


#===============================================================================
#  MAIN SCRIPT
#===============================================================================


if ! rem_which "xgettext" "msgmerge" "git" ; then
	rem_log ERROR "Some tools have not been found"
	exit 1
fi

cd "$REMMINATOP"/po || { rem_log ERROR "$REMMINATOP/po not found" ; exit 1 ; }

for _pofile in *po ; do
	printf "%s: " "$_pofile" ; msgfmt --statistics "$_pofile"
done >| "$REMTMPFILE" 2>&1

TRANSLATED=""
FUZZY=""
UNTRANSLATED=""
MAX=0
while IFS= read -r _msgstat ; do
	#rem_log INFO "dealing with data: $_msgstat"
	if echo "$_msgstat" | grep '^[a-z@_A-Z]\+\.po:.*\.$' >/dev/null 2>&1 ; then
		_translated="$(echo "$_msgstat" | sed -e 's/\(^[a-z]\+.*: \)\([0-9]\+\)\( \)\(translated messages\?\)\(.*\.$\)/\2/g')"
		_fuzzy="$(echo "$_msgstat" | sed -e 's/\(^[a-z]\+.* \)\([0-9]\+\)\( \)\(fuzzy translations\?\)\(.*\.$\)/\2/g')"
		_untranslated="$(echo "$_msgstat" | sed -e 's/\(^[a-z]\+.* \)\([0-9]\+\)\( \)\(untranslated messages\?\)\(.*\.$\)/\2/g')"
		case $_translated in
			''|*[!0-9]*)
				_translated=0
				;;
			*)
				#rem_log INFO "translated: $_translated"
				;;
		esac
		case $_untranslated in
			''|*[!0-9]*)
				_untranslated=0
				;;
			*)
				#rem_log INFO "untranslated: $_untranslated"
				;;
		esac
		case $_fuzzy in
			''|*[!0-9]*)
				_fuzzy=0
				;;
			*)
				#rem_log INFO "fuzzy: $_fuzzy"
				;;
		esac
		_pofile="$(echo "$_msgstat" | cut -d: -f1)"
		#printf "%s %s %s %s\n" "$_pofile" "$_translated" "$_fuzzy" "$_untranslated"
		_sum=$((_translated + _fuzzy + _untranslated))
		[[ "$MAX" -lt "$_sum" ]] && MAX="$_sum"
		# [["0",32],["1",46],["2",28],["3",21],["4",20],["5",13],["6",27]]
		_tav="$(printf "[\"%s\",%s]," "$_pofile" "$_translated")"
		TRANSLATED="${TRANSLATED}$_tav"
		_tav="$(printf "[\"%s\",%s]," "$_pofile" "$_fuzzy")"
		FUZZY="${FUZZY}$_tav"
		_tav="$(printf "[\"%s\",%s]," "$_pofile" "$_untranslated")"
		UNTRANSLATED="${UNTRANSLATED}$_tav"
		unset _translated _untranslated _fuzzy _tav
	fi
done < "$REMTMPFILE"
cat << EOF > "$REMMINATOP"/data/reports/postats.html
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">
<html xmlns="http://www.w3.org/1999/xhtml" lang="en-us">
    <head>
        <meta http-equiv="Content-Type" content="text/html; charset=UTF-8" />
        <title>Remmina Translation Status</title>
        <script type="text/javascript" src="chartkick.min.js"></script>
        <script type="text/javascript" src="https://cdnjs.cloudflare.com/ajax/libs/Chart.js/2.7.2/Chart.bundle.js"></script>
        <script type="text/javascript">
            Chartkick.CustomChart = function (element, dataSource, options) {
            };
        </script>
        <style type="text/css">
            body {
                padding: 20px;
                margin: 0;
                font-family: "Helvetica Neue", Arial, Helvetica, sans-serif;
            }
            h1 {
                text-align: center;
            }
            .container-fluid {
                max-width: 900px;
                margin-left: auto;
                margin-right: auto;
            }
            #multiple-bar-stacked {
                height: 750px;
            }
        </style>
    </head>
    <body>
        <div class="container-fluid">
            <h1>Remmina Translation Status</h1>
            <div id="multiple-bar-stacked"></div>
            <script type="text/javascript">
                new Chartkick.BarChart(
                    "multiple-bar-stacked", [
                        { name: "Translated", data: [${TRANSLATED:0:-1}] },
                        { name: "Fuzzy", data: [${FUZZY:0:-1}] },
                        { name: "Untranslated", data: [${UNTRANSLATED:0:-1}] }
                    ],
                    { max: ${MAX}, stacked: true }
                );
            </script>
            <!--#include virtual="howto-i18n.html" -->
        </div>
    </body>
</html>
EOF
