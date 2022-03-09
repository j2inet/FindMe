cmake_minimum_required(VERSION 3.1...3.22)

project(
  FindMe
  VERSION 1.0
  LANGUAGES CXX)


  

add_executable(findMe findMe.cpp)
target_link_libraries( findMe pthread )