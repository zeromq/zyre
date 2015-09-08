# jyre-jni

## generating files

Generate c header. From project directory root run:

    javah -d src/main/c -v -classpath build/classes/main/:lib/jeromq-0.3.6-SNAPSHOT.jar org.zyre.Zyre
    
Implmenet/edit c file manually.  Then compile c file:

    gcc -fPIC -I"$JAVA_HOME/include" -I"$JAVA_HOME/include/linux" -shared -o src/main/c/libzrejni.so src/main/c/org_zyre_Zyre.c
    
Important: you may need to set the following environment variable to the location of libzyre. 
This allows the linker to find the zyre symbols.  See 
[this link](http://stackoverflow.com/questions/9558909/jni-symbol-lookup-error-in-shared-library-on-linux/13086028#13086028) 
for the hint to use this variable.
    
    export LD_PRELOAD=/usr/local/lib/libzyre.so 

Run ldd to confirm that the right libraries are linked. You should see
libzyre, czmq, libzmq, libsodium, etc. in the output:

    # ldd -d src/main/c/libzrejni.so 
    linux-vdso.so.1 =>  (0x00007ffcc6d31000)
    /usr/local/lib/libzyre.so (0x00007fc4cdb2f000)
    libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fc4cd76a000)
    libczmq.so.3 => /usr/local/lib/libczmq.so.3 (0x00007fc4cd4c3000)
    libzmq.so.5 => /usr/local/lib/libzmq.so.5 (0x00007fc4cd25c000)
    /lib64/ld-linux-x86-64.so.2 (0x00007fc4cdf52000)
    libsodium.so.13 => /usr/local/lib/libsodium.so.13 (0x00007fc4cd009000)
    librt.so.1 => /lib/x86_64-linux-gnu/librt.so.1 (0x00007fc4cce01000)
    libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007fc4ccbe3000)
    libstdc++.so.6 => /usr/lib/x86_64-linux-gnu/libstdc++.so.6 (0x00007fc4cc8df000)
    libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (0x00007fc4cc6c9000)
    libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007fc4cc3c3000)

compile java classes:

    ./gradlew build

run HelloZyre from the command line:

    java -Djava.library.path=./src/main/c:/usr/local/lib -cp build/classes/main/:lib/jeromq-0.3.6-SNAPSHOT.jar org.zyre.HelloZyre

