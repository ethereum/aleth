// Aleth: Ethereum C++ client, tools and libraries.
// Copyright 2014-2019 Aleth Authors.
// Licensed under the GNU General Public License, Version 3.


#pragma once

#include <libdevcore/Exceptions.h>
#include "Common.h"

namespace dev
{
namespace eth
{

// information to add to exceptions
using errinfo_name = boost::error_info<struct tag_field, std::string>;
using errinfo_field = boost::error_info<struct tag_field, int>;
using errinfo_data = boost::error_info<struct tag_data, std::string>;
using errinfo_nonce = boost::error_info<struct tag_nonce, h64>;
using errinfo_difficulty = boost::error_info<struct tag_difficulty, u256>;
using errinfo_target = boost::error_info<struct tag_target, h256>;
using errinfo_seedHash = boost::error_info<struct tag_seedHash, h256>;
using errinfo_mixHash = boost::error_info<struct tag_mixHash, h256>;
using errinfo_ethashResult = boost::error_info<struct tag_ethashResult, std::tuple<h256, h256>>;
using errinfo_importResult = boost::error_info<struct tag_importResult, ImportResult>;
using BadFieldError = boost::tuple<errinfo_field, errinfo_data>;

DEV_SIMPLE_EXCEPTION(OutOfGasBase);
DEV_SIMPLE_EXCEPTION(OutOfGasIntrinsic);
DEV_SIMPLE_EXCEPTION(NotEnoughAvailableSpace);
DEV_SIMPLE_EXCEPTION(NotEnoughCash);
DEV_SIMPLE_EXCEPTION(GasPriceTooLow);
DEV_SIMPLE_EXCEPTION(BlockGasLimitReached);
DEV_SIMPLE_EXCEPTION(FeeTooSmall);
DEV_SIMPLE_EXCEPTION(TooMuchGasUsed);
DEV_SIMPLE_EXCEPTION(ExtraDataTooBig);
DEV_SIMPLE_EXCEPTION(ExtraDataIncorrect);
DEV_SIMPLE_EXCEPTION(TransactionIsUnsigned);
DEV_SIMPLE_EXCEPTION(InvalidSignature);
DEV_SIMPLE_EXCEPTION(InvalidTransactionFormat);
DEV_SIMPLE_EXCEPTION(InvalidBlockFormat);
DEV_SIMPLE_EXCEPTION(InvalidUnclesHash);
DEV_SIMPLE_EXCEPTION(TooManyUncles);
DEV_SIMPLE_EXCEPTION(UncleTooOld);
DEV_SIMPLE_EXCEPTION(UncleIsBrother);
DEV_SIMPLE_EXCEPTION(UncleInChain);
DEV_SIMPLE_EXCEPTION(UncleParentNotInChain);
DEV_SIMPLE_EXCEPTION(InvalidStateRoot);
DEV_SIMPLE_EXCEPTION(InvalidGasUsed);
DEV_SIMPLE_EXCEPTION(InvalidTransactionsRoot);
DEV_SIMPLE_EXCEPTION(InvalidDifficulty);
DEV_SIMPLE_EXCEPTION(InvalidGasLimit);
DEV_SIMPLE_EXCEPTION(InvalidReceiptsStateRoot);
DEV_SIMPLE_EXCEPTION(InvalidTimestamp);
DEV_SIMPLE_EXCEPTION(InvalidLogBloom);
DEV_SIMPLE_EXCEPTION(InvalidNonce);
DEV_SIMPLE_EXCEPTION(InvalidBlockHeaderItemCount);
DEV_SIMPLE_EXCEPTION(InvalidBlockNonce);
DEV_SIMPLE_EXCEPTION(InvalidParentHash);
DEV_SIMPLE_EXCEPTION(InvalidUncleParentHash);
DEV_SIMPLE_EXCEPTION(InvalidNumber);
DEV_SIMPLE_EXCEPTION(InvalidZeroSignatureTransaction);
DEV_SIMPLE_EXCEPTION(InvalidTransactionReceiptFormat);
DEV_SIMPLE_EXCEPTION(TransactionReceiptVersionError);
DEV_SIMPLE_EXCEPTION(PendingTransactionAlreadyExists);
DEV_SIMPLE_EXCEPTION(TransactionAlreadyInChain);
DEV_SIMPLE_EXCEPTION(BlockNotFound);
DEV_SIMPLE_EXCEPTION(UnknownParent);
DEV_SIMPLE_EXCEPTION(DisjointChain);
DEV_SIMPLE_EXCEPTION(AddressAlreadyUsed);
DEV_SIMPLE_EXCEPTION(ZeroSignatureTransaction);
DEV_SIMPLE_EXCEPTION(UnknownTransactionValidationError);
DEV_SIMPLE_EXCEPTION(UnknownError);

DEV_SIMPLE_EXCEPTION(InvalidDatabaseKind);
DEV_SIMPLE_EXCEPTION(DatabaseAlreadyOpen);
DEV_SIMPLE_EXCEPTION(DatabaseCorruption);
DEV_SIMPLE_EXCEPTION(DatabaseExists);
DEV_SIMPLE_EXCEPTION(DatabaseRebuildFailed);

DEV_SIMPLE_EXCEPTION(DAGCreationFailure);
DEV_SIMPLE_EXCEPTION(DAGComputeFailure);

DEV_SIMPLE_EXCEPTION(UnsupportedSnapshotManifestVersion);
DEV_SIMPLE_EXCEPTION(InvalidSnapshotManifest);
DEV_SIMPLE_EXCEPTION(StateTrieReconstructionFailed);
DEV_SIMPLE_EXCEPTION(InvalidStateChunkData);
DEV_SIMPLE_EXCEPTION(InvalidBlockChunkData);
DEV_SIMPLE_EXCEPTION(AccountAlreadyImported);
DEV_SIMPLE_EXCEPTION(InvalidWarpStatusPacket);
DEV_SIMPLE_EXCEPTION(FailedToDownloadManifest);
DEV_SIMPLE_EXCEPTION(FailedToDownloadDaoForkBlockHeader);

DEV_SIMPLE_EXCEPTION(AccountLocked);
DEV_SIMPLE_EXCEPTION(TransactionRefused);
DEV_SIMPLE_EXCEPTION(UnknownAccount);

DEV_SIMPLE_EXCEPTION(PeerDisconnected);
}
}
