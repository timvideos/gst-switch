#! /bin/bash

cd python-api


case $TEST in

	python-api )
		case $TYPE in
			unittest )
				make unittests
				coveralls
				;;
			integration )
				make integration
				coveralls
				;;
		esac
		;;
	module )
		case $TYPE in
			python )
				make test
				coveralls
				;;
			c )
				make test
				cd ../tools
				coveralls -r .
		esac
esac || {
	printf "Failed!!\n"
	exit -1
}