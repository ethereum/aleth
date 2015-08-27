#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# A script to update the different ethereum repositories to latest develop
# Invoke from the root directory and make sure you have upstream set as the
# canonical repository and origin as your fork

ROOT_DIR=$(pwd)
PROJECTS=(cpp-ethereum cpp-ethereum-cmake webthree solidity alethzero mix)

for project in "${PROJECTS[@]}"
do
    cd $project >/dev/null 2>/dev/null
    if [[ $? -ne 0 ]]; then
	echo "Skipping ${project} because directory does not exit";
	cd $ROOT_DIR
	continue
    fi
    BRANCH="$(git symbolic-ref HEAD 2>/dev/null)" ||
    BRANCH="(unnamed branch)"     # detached HEAD
    BRANCH=${BRANCH##refs/heads/}
    if [[ $BRANCH != "develop" ]]; then
	echo "Not updating ${project} because it's not in the develop branch"
	cd $ROOT_DIR
	continue
    fi

    # Assuming origin is your fork and upstream is the canonical repository under the /ethereum domain
    git pull upstream develop
    git push origin develop
    echo "${project} succesfully updated!"
    # Go back to root directory
    cd $ROOT_DIR
done
