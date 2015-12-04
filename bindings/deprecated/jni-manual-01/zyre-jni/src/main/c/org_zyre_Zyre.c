#include <jni.h>
#include <stdio.h>
#include "org_zyre_Zyre.h"
#include "zyre.h"

static jfieldID nodeFID;

inline void put_node(JNIEnv *env, jobject obj, void *node) {
    (*env)->SetLongField(env, obj, nodeFID, (jlong) node);
}

inline void* get_node(JNIEnv *env, jobject obj) {
    return (void*) (*env)->GetLongField(env, obj, nodeFID);
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_nativeInit (JNIEnv *env, jclass clazz) {
    nodeFID  = (*env)->GetFieldID(env, clazz, "nodeHandle", "J");
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_create (JNIEnv * env, jobject thisObj) {

    zyre_t* node = zyre_new(NULL);
    put_node(env, thisObj, node);

    zyre_start(node);

    printf("create done\n");
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_destroy (JNIEnv * env, jobject thisObj) {

    zyre_t* node = get_node(env, thisObj);
    zyre_stop(node);
    zclock_sleep(100);

    zyre_destroy(&node);
    printf("destroyed\n");
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_join (JNIEnv * env, jobject thisObj, jstring group) {
    const char *grp = (*env)->GetStringUTFChars(env, group, 0);
    zyre_t* node = get_node(env, thisObj);
    zyre_join(node, grp);
    printf("joined group: %s\n", grp);
}

JNIEXPORT void JNICALL Java_org_zyre_Zyre_shout (JNIEnv * env, jobject thisObj, jstring group, jstring message) {
    const char *grp  = (*env)->GetStringUTFChars(env, group, 0);
    const char *msg = (*env)->GetStringUTFChars(env, message, 0);
    zyre_t* node = get_node(env, thisObj);
    zyre_shouts(node, grp, "%s", msg);
}
    
JNIEXPORT void JNICALL Java_org_zyre_Zyre_whisper (JNIEnv * env, jobject thisObj, jstring nodeId, jstring message) {
    const char *id  = (*env)->GetStringUTFChars(env, nodeId, 0);
    const char *msg = (*env)->GetStringUTFChars(env, message, 0);
    zyre_t* node = get_node(env, thisObj);
    zyre_whispers(node, id, "%s", msg);
}

JNIEXPORT jstring JNICALL Java_org_zyre_Zyre_recv (JNIEnv * env, jobject thisObj) {

    zyre_t* node = get_node(env, thisObj);
    zmsg_t *zmsg = zyre_recv (node);

    if (!zmsg) {
        printf("interrupted\n");
        return NULL;
    }

    char *event = zmsg_popstr (zmsg);
    char *peer = zmsg_popstr (zmsg);
    char *name = zmsg_popstr (zmsg);
    char *group = zmsg_popstr (zmsg);
    char *message = zmsg_popstr (zmsg); 

     
     char ret[128];
     sprintf(ret, "{\"event\":\"%s\", \"peer\":\"%s\", \"group\":\"%s\", \"message\":\"%s\"}", event, peer, group, message);

    jstring jstrMsg = (*env)->NewStringUTF(env, ret);

    free (event);
    free (peer);
    free (name);
    free (group);
    free (message);
    zmsg_destroy (&zmsg);
    return jstrMsg;
}

