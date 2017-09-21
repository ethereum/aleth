#!/usr/bin/env bash

files=(
  cmake/EthCcache.cmake
  cmake/EthCheckCXXCompilerFlag.cmake
  cmake/EthCompilerSettings.cmake
)

project_root=$(readlink -f "$(dirname $0)/..")
solidity_root=$(readlink -f "$project_root/../solidity")

echo "project_root: $project_root"
echo "solidity_root: $solidity_root"

for file in "${files[@]}"; do
  here_timestamp=$(cd "$project_root" && git log -1 --format="%at" -- "$file")
  there_timestamp=$(cd "$solidity_root" && git log -1 --format="%at" -- "$file")

  if [ "$there_timestamp" ] && [ "$there_timestamp" -gt "$here_timestamp" ]; then
    echo "$solidity_root/$file -> $project_root/$file"
    cp "$solidity_root/$file" "$project_root/$file"
  elif [ "$here_timestamp" ]; then
    echo "$project_root/$file -> $solidity_root/$file"
    cp "$project_root/$file" "$solidity_root/$file"
  fi
done




