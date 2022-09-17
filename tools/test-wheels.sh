#!/bin/bash

set -e
set -x

project_dir=$1

# Move the $(project_dir)/tests to a temporary directory
# so that the tests doesn't use inplace build.

tmp_dir=$(mktemp -d)
cp -r $project_dir/tests $tmp_dir
cd $tmp_dir/tests

# Run the tests
python -m pytest $tmp_dir/tests
