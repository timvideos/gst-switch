#!/bin/sh
# you can either set the environment variables AUTOCONF, AUTOHEADER, AUTOMAKE,
# ACLOCAL, AUTOPOINT and/or LIBTOOLIZE to the right versions, or leave them
# unset and get the defaults

if [ -f "autoregen.sh" ]; then
  rm autoregen.sh
fi
echo "#!/bin/sh" > autoregen.sh
echo "./autogen.sh $@ \$@" >> autoregen.sh
chmod +x autoregen.sh

autoreconf --verbose --force --install || {
 echo 'autogen.sh failed';
 exit 1;
}

./configure $@ || {
 echo 'configure failed';
 exit 1;
}

echo
echo "Now type 'make' to compile this module."
echo
