#!/bin/bash

# NODE: using this script is deprecated! Better install meshrus(-dev).deb package
# This script installs requirements by `apt` if not already installed
# `distribution.sh` uses this script as preinstall

ALL_REQUIRED_PACKAGES_INSTALLED=true
MISSED_PACKAGES=""
function checkPackage {
 PKG_OK=$(dpkg-query -W --showformat='${Status}\n' ${1} | grep "install ok installed")
 if [ "" = "$PKG_OK" ]; then
  ALL_REQUIRED_PACKAGES_INSTALLED=false
  MISSED_PACKAGES="${MISSED_PACKAGES} ${1}"
 fi
}

BASEDIR=$(dirname "$0")
requirements_file="$BASEDIR"/../requirements/ubuntu.txt
for req in `cat $requirements_file`
do	
  checkPackage "${req}"
done

if $ALL_REQUIRED_PACKAGES_INSTALLED; then
 printf "\rAll required packages are already installed!                    \n"
 exit 0
fi

printf "\rSome required package(s) are not installed!                     \n"
printf "${MISSED_PACKAGES}\n"

if [ "$EUID" -ne 0 ]; then
 printf "Root access required!\n"
 RUN_AS_ROOT="NO"
fi

sudo -s printf "Root access acquired!\n" && \
sudo apt update && sudo apt install ${MISSED_PACKAGES}

#check python3.8 pip
PIP_OK=$(python3.8 -m pip --vesrion | grep "No module named pip")
if [ "" != "$PIP_OK" ]; then
 printf "no pip for python3.8 found. installing...\n"
 wget https://bootstrap.pypa.io/get-pip.py
 python3.8 get-pip.py
 rm get-pip.py
fi

# install requirements for python libs
python3.8 -m pip install -r requirements/python.txt

# fix boost signal2 C++20 error in default version 1.71.0 from `apt`
# NOTE: 1.75+ version already has this fix
# https://github.com/boostorg/signals2/commit/15fcf213563718d2378b6b83a1614680a4fa8cec
FILENAME=/usr/include/boost/signals2/detail/auto_buffer.hpp
cat $FILENAME | tr '\n' '\r' | \
sed -e 's/\r        typedef typename Allocator::pointer              allocator_pointer;\r/\
#ifdef BOOST_NO_CXX11_ALLOCATOR\
        typedef typename Allocator::pointer allocator_pointer;\
#else\
        typedef typename std::allocator_traits<Allocator>::pointer allocator_pointer;\
#endif\
/g' | tr '\r' '\n' | sudo tee $FILENAME

if [ "${RUN_AS_ROOT}" = "NO" ]; then
 sudo -k
fi

exit 0
