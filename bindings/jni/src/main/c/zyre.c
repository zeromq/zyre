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
    const char* uuid = zyre_uuid((zyre_t *) ptr);
    return (*env)->NewStringUTF (env, uuid);
}

JNIEXPORT jstring JNICALL
Java_org_zeromq_zyre_Zyre__1_1name (JNIEnv *env, jclass c, jlong ptr) {
    const char* name = zyre_name((zyre_t *) ptr);
    return (*env)->NewStringUTF (env, name);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1shout (JNIEnv *env, jclass c, jlong ptr, jstring str, jlong zmsg_ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zmsg_t *msg = (zmsg_t *) zmsg_ptr;
    const char *group = (const char *) (*env)->GetStringUTFChars (env, str, NULL);
    (*env)->ReleaseStringUTFChars (env, str, group);

    zyre_shout (zyre, group, &msg);
}

JNIEXPORT void JNICALL
Java_org_zeromq_zyre_Zyre__1_1whisper (JNIEnv *env, jclass c, jlong ptr, jstring str, jlong zmsg_ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zmsg_t *msg = (zmsg_t *) zmsg_ptr;
    const char *peer = (const char *) (*env)->GetStringUTFChars (env, str, NULL);
    (*env)->ReleaseStringUTFChars (env, str, peer);

    zyre_whisper (zyre, peer, &msg);
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
    (*env)->ReleaseStringUTFChars (env, key, k);

    const char *v = (const char *) (*env)->GetStringUTFChars (env, value, NULL);
    (*env)->ReleaseStringUTFChars (env, value, v);

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
Java_org_zeromq_zyre_Zyre__1_1dump (JNIEnv *env, jclass c, jlong ptr) {
    zyre_t *zyre = (zyre_t *) ptr;
    zyre_dump (zyre);
}