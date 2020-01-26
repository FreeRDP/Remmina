#!/bin/bash -
#===============================================================================
#
#          FILE: gettranslator.sh
#
#         USAGE: ./gettranslator.sh [language]
#
#   DESCRIPTION: Get last translator and last translation date for all languages
#                or a given language specified as first argument
#
#       OPTIONS: ---
#  REQUIREMENTS: ---
#          BUGS: ---
#         NOTES: ---
#        AUTHOR: Davy Defaud (DevDef), davy.defaud@free.fr
#  ORGANIZATION: Remmina
#       CREATED: 2019-06-23
#       LICENSE: GPLv2
#      REVISION: ---
#===============================================================================

PODIR="$( cd $(dirname "$0")/../po ; pwd -P )"

last_translator_field="Last-Translator"
last_translation_field="PO-Revision-Date"

function getFieldOfFile () {
	fieldname="$1"
	filename="$2"
	regex="^\"$fieldname: (.*)\\\n\"\$"
	field_line=$( grep -E "$regex" "$filename" )
	if [[ "$field_line" =~ $regex ]]; then
		echo -n "${BASH_REMATCH[1]}"
	fi
}

if [ $# -eq 0 ]; then
	languages=$(cat "$PODIR/LINGUAS")
else
	languages="$1"
fi

is_header=1
for lang in $languages; do
	if [ $is_header -eq 1 ] && [ -f "$PODIR/$lang.po" ]; then
		echo "language;last_translator;last_translator_email;last_translation"
		is_header=0
	fi
	last_translator=$( getFieldOfFile "$last_translator_field" "$PODIR/$lang.po" )
	last_translation=$( getFieldOfFile "$last_translation_field" "$PODIR/$lang.po" )
	
	last_translator_email=${last_translator##*<}
	last_translator_email=${last_translator_email%>*}
	if [[ "$last_translator_email" =~ ^(EMAIL@ADDRESS|[^@]+)$ ]]; then
		last_translator_email=''
	fi
	
	echo "$lang;$last_translator;$last_translator_email;$last_translation"
done
