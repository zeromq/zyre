#include <stdio.h>
#include <stdlib.h>
#include <jni.h>
#include "zyre.h"
#include "../../native/include/org_zeromq_zyre_Zyre.h"

JNIEXPORT jlong JNICALL
Java_org_zeromq_zyre_Zyre__1_1init (JNIEnv *env, jclass c, jstring s) {
    const char *name = (const char *) (*env)->GetStringUTFChars (env, s, NULL);
    zyre_t *zyre = zyre_new(name);
    (*env)->ReleaseStringUTFChars (env, s, name);
    if (zyre) {
        return (jlong) zyre;
    }
    return -1;
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1destroy (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zyre_destroy (&zyre);    
}

JNIEXPORT jstring JNICALL
Java_org_zeromq_zyre_Zyre__1_1uuid (JNIEnv *env, jclass c, jlong ptr) {
    const char* uuid = zyre_uuid ((zyre_t *) ptr);
    // zyre manages uuid
    return (*env)->NewStringUTF (env, uuid);
}

JNIEXPORT jstring JNICALL
Java_org_zeromq_zyre_Zyre__1_1name (JNIEnv *env, jclass c, jlong ptr) {
    const char* name = zyre_name ((zyre_t *) ptr);
    // zyre manages name
    return (*env)->NewStringUTF (env, name);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1shout (JNIEnv *env, jclass c, jlong ptr, jstring str, jlong zmsg_ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zmsg_t *msg = (zmsg_t *) zmsg_ptr;
    const char *group = (const char *) (*env)->GetStringUTFChars (env, str, NULL);

    zyre_shout (zyre, group, &msg);
    (*env)->ReleaseStringUTFChars (env, str, group);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1whisper (JNIEnv *env, jclass c, jlong ptr, jstring str, jlong zmsg_ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zmsg_t *msg = (zmsg_t *) zmsg_ptr;
    const char *peer = (const char *) (*env)->GetStringUTFChars (env, str, NULL);

    zyre_whisper (zyre, peer, &msg);
    (*env)->ReleaseStringUTFChars (env, str, peer);
}

JNIEXPORT jint JNICALL
Java_org_zeromq_zyre_Zyre__1_1start (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    return zyre_start (zyre);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1stop (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zyre_stop (zyre);
}

JNIEXPORT jint JNICALL
Java_org_zeromq_zyre_Zyre__1_1join (JNIEnv *env, jclass c, jlong ptr, jstring str) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *group = (const char *) (*env)->GetStringUTFChars (env, str, NULL);
    (*env)->ReleaseStringUTFChars (env, str, group);

    return zyre_join (zyre, group);
}

JNIEXPORT jint JNICALL
Java_org_zeromq_zyre_Zyre__1_1leave (JNIEnv *env, jclass c, jlong ptr, jstring str) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *group = (const char *) (*env)->GetStringUTFChars (env, str, NULL);
    (*env)->ReleaseStringUTFChars (env, str, group);

    return zyre_leave (zyre, group);
}

JNIEXPORT jlong JNICALL
Java_org_zeromq_zyre_Zyre__1_1zyre_1recv (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zmsg_t *msg = zyre_recv (zyre);
    if (msg) {
        return (jlong) msg;
    }
    return -1;
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1set_1header (JNIEnv *env, jclass c, jlong ptr, jstring key, jstring value) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *k = (const char *) (*env)->GetStringUTFChars (env, key, NULL);
    const char *v = (const char *) (*env)->GetStringUTFChars (env, value, NULL);
    // Don't need to explictly deallocate since zyre_set_header does that for you
    zyre_set_header (zyre, k, "%s", v);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1set_1verbose (JNIEnv *env, jclass c , jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zyre_set_verbose (zyre);
}

JNIEXPORT jint JNICALL
Java_org_zeromq_zyre_Zyre__1_1set_1endpoint (JNIEnv *env, jclass c, jlong ptr, jstring endpoint) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *ep = (const char *) (*env)->GetStringUTFChars (env, endpoint, NULL);
    (*env)->ReleaseStringUTFChars (env, endpoint, ep);

    return zyre_set_endpoint (zyre, "%s", ep);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1gossip_1bind (JNIEnv *env, jclass c, jlong ptr, jstring endpoint) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *ep = (const char *) (*env)->GetStringUTFChars (env, endpoint, NULL);
    (*env)->ReleaseStringUTFChars (env, endpoint, ep);

    zyre_gossip_bind (zyre, "%s", ep);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1gossip_1connect (JNIEnv *env, jclass c, jlong ptr, jstring endpoint) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *ep = (const char *) (*env)->GetStringUTFChars (env, endpoint, NULL);
    (*env)->ReleaseStringUTFChars (env, endpoint, ep);

    zyre_gossip_connect (zyre, "%s", ep);
}


JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1dump (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zyre_dump (zyre);
}

JNIEXPORT jlong JNICALL
Java_org_zeromq_zyre_Zyre__1_1own_1groups (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zlist_t *list = zyre_own_groups (zyre);
    if (list) {
        return (jlong) list;
    }
    return -1;
}

JNIEXPORT jlong JNICALL
Java_org_zeromq_zyre_Zyre__1_1peers (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zlist_t *list = zyre_peers (zyre);
    if (list) {
        return (jlong) list;
    }
    return -1;
}

JNIEXPORT jlong JNICALL
Java_org_zeromq_zyre_Zyre__1_1peer_1groups (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zlist_t *list = zyre_peer_groups (zyre);
    if (list) {
        return (jlong) list;
    }
    return -1;
}

JNIEXPORT jstring JNICALL
Java_org_zeromq_zyre_Zyre__1_1peer_1header_1value (JNIEnv *env, jclass c, jlong ptr, jstring jpeer, jstring jname) {
    zyre_t *zyre = (zyre_t *) ptr;
    const char *peer = (const char *) (*env)->GetStringUTFChars (env, jpeer, NULL);
    const char *name = (const char *) (*env)->GetStringUTFChars (env, jname, NULL);

    char *value = zyre_peer_header_value (zyre, peer, name);

    jstring s = (*env)->NewStringUTF (env, value);
    zstr_free (&value);

    (*env)->ReleaseStringUTFChars (env, jpeer, peer);
    (*env)->ReleaseStringUTFChars (env, jname, name);

    return s;
}

JNIEXPORT jint JNICALL
Java_org_zeromq_zyre_Zyre__1_1shouts (JNIEnv *env, jclass c, jlong ptr, jstring jgroup, jstring jvalue) {
    zyre_t *zyre = (zyre_t *) ptr;

    const char *group = (const char *) (*env)->GetStringUTFChars (env, jgroup, NULL);
    const char *value = (const char *) (*env)->GetStringUTFChars (env, jvalue, NULL);

    int rc = zyre_shouts (zyre, group, "%s", value);

    (*env)->ReleaseStringUTFChars (env, jgroup, group);
    (*env)->ReleaseStringUTFChars (env, jvalue, value);

    return rc;
}

JNIEXPORT jint JNICALL
Java_org_zeromq_zyre_Zyre__1_1whispers (JNIEnv *env, jclass c, jlong ptr, jstring jpeer, jstring jvalue) {
    zyre_t *zyre = (zyre_t *) ptr;

    const char *peer = (const char *) (*env)->GetStringUTFChars (env, jpeer, NULL);
    const char *value = (const char *) (*env)->GetStringUTFChars (env, jvalue, NULL);

    int rc = zyre_whispers (zyre, peer, "%s", value);

    (*env)->ReleaseStringUTFChars (env, jpeer, peer);
    (*env)->ReleaseStringUTFChars (env, jvalue, value);

    return rc;
}

JNIEXPORT jstring JNICALL
Java_org_zeromq_zyre_Zyre__1_1peerAddress (JNIEnv *env, jclass c, jlong ptr, jstring jpeer) {
    zyre_t *zyre = (zyre_t *) ptr;

    const char *peer = (const char *) (*env)->GetStringUTFChars (env, jpeer, NULL);

    char *address = zyre_peer_address (zyre, peer);

    jstring s = (*env)->NewStringUTF (env, address);
    zstr_free (&address);
    (*env)->ReleaseStringUTFChars (env, jpeer, peer);

    return s;
}
