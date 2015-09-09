#include <jni.h>
#include <stdio.h>
#include "org_zyre_Zyre.h"
#include "zyre.h"

static zyre_t *node;
static zpoller_t *poller;

JNIEXPORT void JNICALL Java_org_zyre_Zyre_create (JNIEnv * env, jobject thisObj) {

    node = zyre_new(NULL);
    assert(node);
    zyre_start(node);

    poller = zpoller_new (zyre_socket (node), NULL);

    printf("create done\n");
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_destroy (JNIEnv * env, jobject thisObj) {
    zpoller_destroy (&poller);

    // Notify peers that this peer is shutting down. Provide
    // a brief interval to ensure message is emitted.
    zyre_stop(node);
    zclock_sleep(100);

    zyre_destroy(&node);
    printf("destroyed\n");
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_join (JNIEnv * env, jobject thisObj, jstring group) {
    const char *grp = (*env)->GetStringUTFChars(env, group, 0);
    zyre_join(node, grp);
    printf("joined group: %s\n", grp);
}


JNIEXPORT void JNICALL Java_org_zyre_Zyre_shout (JNIEnv * env, jobject thisObj, jstring group, jstring message) {
    const char *grp  = (*env)->GetStringUTFChars(env, group, 0);
    const char *msg = (*env)->GetStringUTFChars(env, message, 0);
    zyre_shouts(node, grp, "%s", msg);
    printf("shouted to %s: %s\n", grp, msg);
}
    
JNIEXPORT void JNICALL Java_org_zyre_Zyre_whisper (JNIEnv * env, jobject thisObj, jstring nodeId, jstring message) {
    const char *id  = (*env)->GetStringUTFChars(env, nodeId, 0);
    const char *msg = (*env)->GetStringUTFChars(env, message, 0);
    zyre_whispers(node, id, "%s", msg);
    printf("whispered to %s: %s\n", id, msg);
}

JNIEXPORT jobject JNICALL Java_org_zyre_Zyre_recv (JNIEnv * env, jobject thisObj) {
    void *which = zpoller_wait (poller, -1); // no timeout

    if (which == zyre_socket (node)) {
        zmsg_t *msg = zmsg_recv (which);
        char *event = zmsg_popstr (msg);
        char *peer = zmsg_popstr (msg);
        char *name = zmsg_popstr (msg);
        char *group = zmsg_popstr (msg);
        char *message = zmsg_popstr (msg); 

        // instantiate and populate a ZMsg
        
        jclass cls = (*env)->FindClass(env, "org/zeromq/ZMsg");
        if (!cls) {
            printf("Find class failed for ZMsg\n");
            return 0;
        }

        jmethodID ctor = (*env)->GetMethodID(env, cls, "<init>", "()V");
        if (!ctor) {
            printf("Find method for ctor failed\n");
            return 0;
        }

        jobject zmsg = (*env)->NewObject(env, cls, ctor);
        jstring msgStr = (*env)->NewStringUTF(env, message);

        jmethodID push = (*env)->GetMethodID(env, cls, "push", "(I)V");
        (*env)->CallVoidMethod(env, zmsg, push, msgStr);
        
        free (event);
        free (peer);
        free (name);
        free (group);
        free (message);
        zmsg_destroy (&msg);

        // return jobject
        printf("received: %s \n", message);
        return zmsg;
    }
    else {
        printf("unexpected socket type");
        return 0;
    }
}

