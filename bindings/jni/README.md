# zyre-jni

JNI Binding for zyre

## Building the JNI Layer for Linux

Ensure you have gradle and cmake installed, then run:

    gradle build jar
    gradle test

This calls javah to build the headers in src/native/include, and then compiles the C and Java pieces to create a jar file a sharable library (.so).

## Building the JNI Layer for Android

See bindings/jni/android/build.sh.

You need the Android Native Development Kit (NDK) installed.

Set these environment variables, e.g:

    ANDROID_NDK_ROOT=$HOME/android-ndk-r10e
    TOOLCHAIN_NAME=arm-linux-androideabi-4.9
    TOOLCHAIN_HOST=arm-linux-androideabi
    TOOLCHAIN_ARCH=arm
    TOOLCHAIN_PATH=$ANDROID_NDK_ROOT/toolchains/$TOOLCHAIN_NAME/prebuilt/linux-x86_64/bin

Then in the android directory, run:

    ./build.sh

This does the following:

* It compiles the zyre C sources for Android, into a native library libzyre.so in builds/android/
* It compiles the JNI Java classes into a jar file zyre-jni-1.1.0.jar in bindings/jni/build/libs
* It compiles the JNI C sources for Android, into a native library libzyrejni.so.
* It takes czmq-jni-*.jar, which must already be built in ../czmq/bindings/jni/build/libs/
* It combines all these into zyre-android.jar, which you can use in your Android projects.

## Building the JNI Layer for Windows

You need MS Visual Studio 2010 or later.

You need the Java SDK. Set the JAVA_HOME environment to the installation location, e.g. C:Program FilesJavajdk1.8.0_66.

1. Check out all dependent projects from github, at the same level as this project. E.g.: libsodium, libzmq, czmq.
2. In each project, open a console in builds/msvc/vs2010 and run the build.bat batch file.
3. In this project, open a console in bindings/jni/msvc/vs2010 and run the build.bat batch file.

The resulting libraries (zyrejni.dll, zyrejni.lib) are created in bindings/jni/msvc/bin.

## Using the JNI API

- to be written.

## License


Copyright (c) the Contributors as noted in the AUTHORS file.

This file is part of Zyre, an open-source framework for proximity-based
peer-to-peer applications -- See http://zyre.org.

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

