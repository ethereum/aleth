#!/bin/sh
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to list all changes of the submodule compared
# to a revision passed as the only argument.
# @
echo "Listing changes for umbreall repo itself"
git --no-pager diff $1
subs=(`git submodule | awk '{print $2}'`)
for sub in ${subs[*]}; do
	lastrevision=`git diff  $1 $sub | fgrep "Subproject" | head -n1 | awk '{print $3}'`
	cd $sub
	echo "Listing changes for $sub"
	git --no-pager diff $lastrevision
	cd ..
done
