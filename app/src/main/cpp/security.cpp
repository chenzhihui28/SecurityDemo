#include <android/log.h>
#include <string>
#include <string.h>
#include <jni.h>

#define LOGINFO(...) ((void)__android_log_print(ANDROID_LOG_INFO, "security", __VA_ARGS__))
#define LOGERROR(...) ((void)__android_log_print(ANDROID_LOG_ERROR, "security", __VA_ARGS__))

static int verifySign(JNIEnv *env);

jint JNI_OnLoad(JavaVM *vm, void *reserved) {
    JNIEnv *env = NULL;
    if (vm->GetEnv((void **) &env, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }
    if (verifySign(env) == JNI_OK) {
        return JNI_VERSION_1_4;
    }
    LOGERROR("签名不一致!");
    return JNI_ERR;
}

static jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass activity_thread_clz = env->FindClass("android/app/ActivityThread");
    if (activity_thread_clz != NULL) {
        jmethodID currentApplication = env->GetStaticMethodID(
                activity_thread_clz, "currentApplication", "()Landroid/app/Application;");
        if (currentApplication != NULL) {
            application = env->CallStaticObjectMethod(activity_thread_clz, currentApplication);
        } else {
            LOGERROR("Cannot find method: currentApplication() in ActivityThread.");
        }
        env->DeleteLocalRef(activity_thread_clz);
    } else {
        LOGERROR("Cannot find class: android.app.ActivityThread");
    }

    return application;
}


static const char *SIGN = "308203533082023ba0030201020204691f1618300d06092a864886f70d01010b0500305a310d300b060355040613046368656e310d300b060355040813046368656e310d300b060355040713046368656e310d300b060355040a13046368656e310d300b060355040b13046368656e310d300b060355040313046368656e301e170d3138303432353038323530355a170d3433303431393038323530355a305a310d300b060355040613046368656e310d300b060355040813046368656e310d300b060355040713046368656e310d300b060355040a13046368656e310d300b060355040b13046368656e310d300b060355040313046368656e30820122300d06092a864886f70d01010105000382010f003082010a02820101008001ead218d523543023e195c065fb807f06313a915f510e59d556c55f192163a5acd4300851ae738a601c2b4a6284beb900c31338d4ce36dd3fe490c16560dc4dfca13685a1993c6705976797a5a23841edd261c95a419da6e62ebe17a25b787e1ff2097a9b112f48a3fe03b7777b9833799680bae0a9a1d423296b31a87e65758b6fb56f9ea78c4d0537924663563c8cc59c2877c4ee58704b03f3cd5bdb1383084064be54b0a8de408ca51ecb0c68194fc6adb643d5827053371a34d75d8c98ff7edc4d929747afe48a7e6701d9dfd7897a221ba82b63c27bf2ce8c0784c5810f425526e28b840c429d9f0984b44671d3d5a1b77827f529c7aad6877f3c450203010001a321301f301d0603551d0e0416041403a7a9f045b7ab586c6102c49ced45e8879d350a300d06092a864886f70d01010b050003820101007dc1ae5fabc39101d74987f535be17e9c80b71daf55193b4c6abd381adf59042dabeeebd59d8f2ad1cbe0dd4feaee803e62b9b4b425a0936a4041245b7893a51c40fe0f10ea67a0a00e6360c25c703c5801f8e8740c544ce3d3874620dc26efbf5331020d30ac9e0ae1b217ab841959386a56aab61981ba01a1d62ede59866d936d1ade91d6723ea5569128a6bd5a8158733bb18499f8100a29beee19713be64b82ba348bb4d8eb5591cd5b09323746ccd5b9b115c6c1462f07165070eab6cfd9f18af717e94f384059b72ee3866553adc6d248b7d4a7f945666edb822eb20d2229d3b8c8b1038b1f11640b7a00e8f79623ffedf697da528af1ac828b009af53";

static int verifySign(JNIEnv *env) {
    // Application object
    jobject application = getApplication(env);
    if (application == NULL) {
        return JNI_ERR;
    }
    // Context(ContextWrapper) class
    jclass context_clz = env->GetObjectClass(application);
    // getPackageManager()
    jmethodID getPackageManager = env->GetMethodID(context_clz, "getPackageManager",
                                                   "()Landroid/content/pm/PackageManager;");
    // android.content.pm.PackageManager object
    jobject package_manager = env->CallObjectMethod(application, getPackageManager);
    // PackageManager class
    jclass package_manager_clz = env->GetObjectClass(package_manager);
    // getPackageInfo()
    jmethodID getPackageInfo = env->GetMethodID(package_manager_clz, "getPackageInfo",
                                                "(Ljava/lang/String;I)Landroid/content/pm/PackageInfo;");
    // context.getPackageName()
    jmethodID getPackageName = env->GetMethodID(context_clz, "getPackageName",
                                                "()Ljava/lang/String;");
    // call getPackageName() and cast from jobject to jstring
    jstring package_name = (jstring) (env->CallObjectMethod(application, getPackageName));
    // PackageInfo object
    jobject package_info = env->CallObjectMethod(package_manager, getPackageInfo, package_name, 64);
    // class PackageInfo
    jclass package_info_clz = env->GetObjectClass(package_info);
    // field signatures
    jfieldID signatures_field = env->GetFieldID(package_info_clz, "signatures",
                                                "[Landroid/content/pm/Signature;");
    jobject signatures = env->GetObjectField(package_info, signatures_field);
    jobjectArray signatures_array = (jobjectArray) signatures;
    jobject signature0 = env->GetObjectArrayElement(signatures_array, 0);
    jclass signature_clz = env->GetObjectClass(signature0);

    jmethodID toCharsString = env->GetMethodID(signature_clz, "toCharsString",
                                               "()Ljava/lang/String;");
    // call toCharsString()
    jstring signature_str = (jstring) (env->CallObjectMethod(signature0, toCharsString));

    // release
    env->DeleteLocalRef(application);
    env->DeleteLocalRef(context_clz);
    env->DeleteLocalRef(package_manager);
    env->DeleteLocalRef(package_manager_clz);
    env->DeleteLocalRef(package_name);
    env->DeleteLocalRef(package_info);
    env->DeleteLocalRef(package_info_clz);
    env->DeleteLocalRef(signatures);
    env->DeleteLocalRef(signature0);
    env->DeleteLocalRef(signature_clz);

    const char *sign = env->GetStringUTFChars(signature_str, NULL);
    if (sign == NULL) {
        LOGERROR("分配内存失败");
        return JNI_ERR;
    }
//发布记得去掉log！！！！！！！
//    LOGINFO("应用中读取到的签名为：%s", sign);
//    LOGINFO("native中预置的签名为：%s", SIGN);
    int result = strcmp(sign, SIGN);
//    LOGINFO("strcmp：%d", result);
    // 使用之后要释放这段内存
    env->ReleaseStringUTFChars(signature_str, sign);
    env->DeleteLocalRef(signature_str);
    if (result == 0) { // 签名一致
        return JNI_OK;
    }

    return JNI_ERR;
}


extern "C" JNIEXPORT jstring

JNICALL
Java_com_chenzhihui_demo_SecurityUtil_getSecret(
        JNIEnv *env,
        jobject /* this */) {
    std::string hello = "hahahahasecrethahahahaha";
    return env->NewStringUTF(hello.c_str());
}



