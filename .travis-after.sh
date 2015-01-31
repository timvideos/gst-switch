#! /bin/bash -ex
#
# This script runs after everything has finished. It should be used to upload
# any build assets, logs and coverage information.


if [[ $TYPE == "style" ]]; then
	exit 0
fi

make coverage

case $TEST in
	python-api )
		cd python-api
		;;
	server )
		cd tools
		;;
esac

coveralls $COVERALL_ARGS || {
	printf "Coveralls failed!\n"
	exit -1
}

