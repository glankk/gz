@echo off
pushd .
cd %~dp0/..
if not exist patch\ups mkdir patch\ups
gru patch/lua/make-patch.lua %*
popd
