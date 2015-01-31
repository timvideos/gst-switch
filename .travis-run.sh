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

export DISPLAY=:99.0
export GCOV_PREFIX=$TRAVIS_BUILD_DIR/gst-switch-coverage

case $TEST in
	python-api )
		cd python-api
		make $TYPE
		;;
	server )
		make $TYPE
		;;
	* )
		echo "Unknown TEST='$TEST'"
		exit -1
		;;
esac
