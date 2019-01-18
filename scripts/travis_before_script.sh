#!/bin/bash
if [ "$TRAVIS_OS_NAME" = "linux" ]
then
	sudo add-apt-repository -y ppa:beineri/opt-qt-5.11.0-xenial
	sudo apt-get update
	sudo apt-get install qt511base qt511tools qt511svg
fi
