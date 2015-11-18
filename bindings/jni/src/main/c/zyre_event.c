#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include "zyre.h"
#include "zyre_event.h"
#include "../../native/include/org_zeromq_zyre_ZyreEvent.h"

JNIEXPORT jlong JNICALL
Java_org_zeromq_zyre_ZyreEvent__1_1init (JNIEnv *env, jclass c) {
    return -1;
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_ZyreEvent__1_1destroy (JNIEnv *env, jclass c, jlong ptr) {
    zyre_event_t *zyre_event = (zyre_event_t *) ptr;
    zyre_event_destroy (&zyre_event);
}





