#! /bin/bash -ex

./autogen.sh --prefix=/usr || {
  printf "Failed to do autogen!!!\n"
  exit -1
}
make clean
make || {
  printf "make of gst-switch failed!!!\n"
  exit -1
}
sudo make install || {
  printf "make install of gst-switch failed!!!\n"
  exit -1
}

function blue {
	echo -en '\E[1;34m'
	echo $@
	tput sgr0
}

export DISPLAY=:99.0
export GCOV_PREFIX=$TRAVIS_BUILD_DIR/gst-switch-coverage

case $TEST in
	python-api )
		cd python-api
		{
			set +x
			echo -n "Checking under "
			blue "Python 3"
			echo "-----------------------------------------------------------"
		}
		make PYTHONVERSION=3.4 $TYPE
		{
			set +x
			echo "-----------------------------------------------------------"
			echo
			echo -n "Checking under "
			blue "Python 2"
			echo "-----------------------------------------------------------"
		}
		make PYTHONVERSION=2.7 $TYPE
		{
			set +x
			echo "-----------------------------------------------------------"
		}
		;;
	server )
		make $TYPE
		;;
	* )
		echo "Unknown TEST='$TEST'"
		exit -1
		;;
esac
