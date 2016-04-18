#!/usr/bin/env bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# Some common functionality to be used by ethupdate and ethbuild

PROJECTS_HELP="    --project NAME          Will only clone/update/build repos for the requested project. Valid values are: [\"all\", \"webthree-helpers\", \"libweb3core\", \"libethereum\", \"webthree\", \"solidity\", \"alethzero\", \"mix\"]."

ALL_CLONE_REPOSITORIES=(libweb3core libethereum evmjit webthree-helpers tests web3.js webthree solidity alethzero mix)
ALL_BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum evmjit solidity webthree alethzero mix)

function set_repositories() {
	if [[ $1 == "" || $2 == "" ]]; then
		echo "ETHBUILD - ERROR: get_repositories() function called without the 2 needed arguments."
		exit 1
	fi
	REQUESTER_SCRIPT=$1
	REQUESTED_PROJECT=$2
	case $REQUESTED_PROJECT in
		"all" | "webthree-helpers")
			CLONE_REPOSITORIES=("${ALL_CLONE_REPOSITORIES[@]}")
			BUILD_REPOSITORIES=("${ALL_BUILD_REPOSITORIES[@]}")
			;;
		"libweb3core")
			CLONE_REPOSITORIES=(libweb3core webthree-helpers tests)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core)
			;;
		"libethereum")
			CLONE_REPOSITORIES=(libweb3core libethereum evmjit webthree-helpers tests)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum evmjit)
			;;
		"webthree")
			CLONE_REPOSITORIES=(libweb3core libethereum evmjit webthree-helpers web3.js tests solidity webthree)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum evmjit solidity webthree)
			;;
		"solidity")
			CLONE_REPOSITORIES=(libweb3core libethereum evmjit webthree-helpers tests solidity)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum evmjit solidity)
			;;
		"alethzero")
			CLONE_REPOSITORIES=(libweb3core libethereum evmjit webthree-helpers tests web3.js webthree solidity alethzero)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum evmjit webthree solidity alethzero)
			;;
		"mix")
			CLONE_REPOSITORIES=(libweb3core libethereum evmjit webthree-helpers tests web3.js webthree solidity mix)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum evmjit webthree solidity mix)
			;;
		*)
			echo "${REQUESTER_SCRIPT} - ERROR: Unrecognized value \"${REQUESTED_PROJECT}\" for the --project argument."
			exit 1
			;;
	esac
}
