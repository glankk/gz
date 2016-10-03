@echo off
pushd .
cd %~dp0/..
gru patch/lua/make-patch.lua %*
popd
