#!/bin/sh

g++ *.cpp SFNUL/*.cpp MySql/*.cpp -std=c++17 -o ../RunDir/LunarPhase -lmysqlclient -lsfml-network-s -lsfml-system-s -lpthread -O3 -Ofast -s -m64

