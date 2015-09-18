#!/bin/bash
# author: Lefteris Karapetsas <lefteris@refu.co>
#
# Some common functionality to be used by ethupdate and ethbuild

PROJECTS_HELP="    --project NAME            Will only clone/update/build repos for the requested project. Valid values are: [\"all\", \"libweb3core\", \"libethereum\", \"\webthree\", \"solidity\", \"alethzero\", \"mix\"]."
CLONE_REPOSITORIES=(libweb3core libethereum libwhisper webthree-helpers tests web3.js webthree solidity alethzero mix)
BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum webthree solidity alethzero mix)

function set_repositories() {
	if [[ $1 == "" || $2 == "" ]]; then
		echo "ETHBUILD - ERROR: get_repositories() function called without the 2 needed arguments."
		exit 1
	fi
	REQUESTER_SCRIPT=$1
	REQUESTED_PROJECT=$2
	case $REQUESTED_PROJECT in
		"all")
			CLONE_REPOSITORIES=(libweb3core libethereum libwhisper webthree-helpers web3.js tests webthree solidity alethzero mix)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum webthree solidity alethzero mix)
			;;
		"libweb3core")
			CLONE_REPOSITORIES=(libweb3core webthree-helpers tests)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core)
			;;
		"libethereum")
			CLONE_REPOSITORIES=(libweb3core libethereum webthree-helpers tests)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum)
			;;
		"webthree")
			CLONE_REPOSITORIES=(libweb3core libethereum webthree-helpers web3.js tests webthree)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum webthree)
			;;
		"solidity")
			CLONE_REPOSITORIES=(libweb3core libethereum webthree-helpers tests webthree web3.js solidity)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum webthree solidity)
			;;
		"alethzero")
			CLONE_REPOSITORIES=(libweb3core libethereum webthree-helpers tests web3.js webthree solidity alethzero)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum webthree solidity alethzero)
			;;
		"mix")
			CLONE_REPOSITORIES=(libweb3core libethereum webthree-helpers tests web3.js webthree solidity mix)
			BUILD_REPOSITORIES=(webthree-helpers/utils libweb3core libethereum webthree solidity mix)
			;;
		*)
			echo "${REQUESTER_SCRIPT} - ERROR: Unrecognized value \"${REQUESTED_PROJECT}\" for the --project argument."
			exit 1
			;;
	esac
}
