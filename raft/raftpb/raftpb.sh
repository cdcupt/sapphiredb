#!/bin/sh
set -x

protoc -I=./ --cpp_out=./ ./raftpb.proto

echo "come on baby!";
