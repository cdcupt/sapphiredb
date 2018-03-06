#!/bin/sh
set -x

protoc -I=./ --cpp_out=../raft/ ./shapphiredb.proto

echo "come on baby!";
