#! /bin/bash -ex
#
# This script runs after everything has finished. It should be used to upload
# any build assets, logs and coverage information.


if [[ $TYPE != "style" ]]; then

	case $TEST in
		python-api )
			cd python-api
			;;
		server )
			find ~ -name *.gcda
			COVERALL_ARGS="-n -r $TRAVIS_BUILD_DIR/gst-switch-coverage"
			;;
	esac

	coveralls $COVERALL_ARGS || {
		printf "Coveralls failed!\n"
		exit -1
	}

fi
