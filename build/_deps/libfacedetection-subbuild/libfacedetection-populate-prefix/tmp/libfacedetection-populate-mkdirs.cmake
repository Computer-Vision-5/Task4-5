# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "G:/FaceRecognition/build/_deps/libfacedetection-src")
  file(MAKE_DIRECTORY "G:/FaceRecognition/build/_deps/libfacedetection-src")
endif()
file(MAKE_DIRECTORY
  "G:/FaceRecognition/build/_deps/libfacedetection-build"
  "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix"
  "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix/tmp"
  "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix/src/libfacedetection-populate-stamp"
  "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix/src"
  "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix/src/libfacedetection-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix/src/libfacedetection-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "G:/FaceRecognition/build/_deps/libfacedetection-subbuild/libfacedetection-populate-prefix/src/libfacedetection-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
