#!/bin/sh
##############################################################################
## This is used to package .deb packages and upload them to the launchpad
## ppa servers for building. It requires one argument (passed via env):
##
## ethbranch:   the branch to use for webthree-umbrella, either develop or
##              release
## debplatform: target ubuntu "release", i.e. trusty, vivid, wily or xenial
##              (we should switch to numbered releases at some point)
##
## The "debian" directories can be found in github.com/ethereum/ethereum-ppa
## The develop branch will be used for anything before wily, while wily and
## xenial have their own branches.
##
## The gnupg key for "build@ethdev.com" has to be present in order to sign
## the package.
##
## Caveats: A package with the same version number should not be uploaded
## multiple times to the build servers. This is not a big problem for the
## develop snapshots as the version will change with every commit, but
## for release packages, the version will be taken from the CMakeLists.txt
## file. This means that this script should only be run once for every
## release. If the source package was uploaded before, add '-sd' to the
## debuild command (see below).
##############################################################################

set -e

distribution=${debplatform}
echo "building for ${debplatform}"
if [ -z "$distribution" ]
	then distribution=trusty
fi
arch=amd64
mainrepo=webthree-umbrella
now=$(date +"%Y%m%d")
project="cpp-ethereum"
ppabranch=develop
if [ "${distribution}" = "wily" ]; then
	ppabranch=wily
elif [ "${distribution}" = "xenial" ]; then
	ppabranch=xenial
fi
codebranch=${ethbranch}
pparepo=ethereum/ethereum
if [ -z "$codebranch" ]
then
	codebranch=develop
fi
if [ "$codebranch" = "develop" ]
then
    pparepo=ethereum/ethereum-dev
fi
echo codebranch=${codebranch}
echo pparepo=${pparepo}

keyid=703F83D0
email=builds@ethereum.org
mainppa="http://ppa.launchpad.net/ethereum/ethereum/ubuntu"
devppa="http://ppa.launchpad.net/ethereum/ethereum-dev/ubuntu"

# clone source repo
git clone https://github.com/ethereum/${mainrepo}.git -b ${codebranch} --recursive

# create source tarball"
cd ${mainrepo}
version=`grep -oP "$project VERSION \"?\K[0-9.]+(?=\")"? CMakeLists.txt`
version="${version}"
revision=`git rev-parse --short HEAD`

if [ "${codebranch}" = "release" ]; then 
    	debversion=${version}~${distribution}
    else
    	debversion=${version}-SNAPSHOT-${BUILD_NUMBER}-${now}-${revision}~${distribution}
fi

echo debversion=${debversion}

tar --exclude .git -czf ../${project}_${debversion}.orig.tar.gz .

# get debian/ direcotry
wget https://github.com/ethereum/ethereum-ppa/archive/${ppabranch}.tar.gz -O- |
tar -zx --exclude package.sh --exclude README.md --strip-components=1

# bump version
EMAIL="$email" dch -v ${debversion}-0ubuntu1 "git build of ${revision}"

# build source package
# If packages is rejected because original source is already present, add
# -sd to remove it from the .changes file
debuild -S -sa -us -uc

# set PPA dependencies for pbuilder
echo "OTHERMIRROR=\"deb [trusted=yes] ${mainppa} ${distribution} main|deb-src [trusted=yes] ${mainppa} ${distribution} main|deb [trusted=yes] ${devppa} ${distribution} main|deb-src [trusted=yes] ${devppa} ${distribution} main|deb [trusted=yes]\"" > ~/.pbuilderrc

# do the build
#pdebuild --buildresult . --pbuilder /usr/sbin/cowbuilder --architecture amd64 -- --buildresult . --basepath /var/cache/pbuilder/${distribution}-${arch}-ethereum.cow

# prepare .changes file for Launchpad
sed -i -e s/UNRELEASED/${distribution}/ -e s/urgency=medium/urgency=low/ ../*.changes

# sign the package
debsign -k ${keyid} ../${project}_${debversion}-0ubuntu1_source.changes

# uploa
dput ppa:${pparepo} ../${project}_${debversion}-0ubuntu1_source.changes

