#
#  By Duzy Chan <code@duzy.info>, 2012, 2013
#  
function main()
{
    local INDENT=indent
    local INDENT_PARAMETERS="--braces-on-if-line \
	--case-brace-indentation0 \
	--case-indentation2 \
	--braces-after-struct-decl-line \
	--line-length80 \
	--no-tabs \
	--cuddle-else \
	--dont-line-up-parentheses \
	--continuation-indentation4 \
	--honour-newlines \
	--tab-size8 \
	--indent-level2 \
	--leave-preprocessor-space"

    $INDENT ${INDENT_PARAMETERS} "$@"
    $INDENT ${INDENT_PARAMETERS} "$@"
}
