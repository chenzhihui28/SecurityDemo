# SecurityDemo
ndk进行简单的签名校验，密钥保护
# 使用Cmake进行NDK开发(应用签名校验、密钥保护、so文件适配相关)

[NDK官方指南](https://developer.android.com/ndk/guides/concepts.html)


[CMake官方文档](https://developer.android.com/ndk/guides/cmake.html)


[CMake译文](https://www.zybuluo.com/khan-lau/note/254724)


[Google官方samples](https://github.com/googlesamples/android-ndk)



## 从HelloWorld开始咯
新建项目，勾选include c++ support
<br>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqonkidf5wj21e011c789.jpg" width="500px"/>
<br>

建立完成之后，看看现在的项目文件结构
<br>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqonno0dorj20qq13agq1.jpg" width="350px"/>
<br>
 
 build.gradle
 <br>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqopdwoa8fj21ca1aijzu.jpg" width ="500"/>
<br>

多了一个.externalNativeBuild文件夹
main下多了个cpp文件夹
以及app里面多了CMakeLists.txt
build.gradle文件多了两处externalNativeBuild块
defaultConfig外面的那个externalNativeBuild块主要是指定CMakeLists.txt的文件路径
defaultConfig里面的那个externalNativeBuild块主要填写CMake的命令参数。即由 arguments 中的参数最后转化成一个可执行的 CMake 的命令，可以在.externalNativeBuild/cmake/debug/{abi}/cmake_build_command.txt 中查到
<br>

来看看CMakeLists.txt
<br>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqopw1wxrmj211a1iyaj8.jpg" width="500"/>
<br>

CMakeLists文件里面指出了需要编译的文件
<br>

默认已经帮我们生成了一个jni方法，返回一个helloworld字符串
<br>
<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqonwbykunj20w60hudih.jpg" width="500px"/>
<br>
<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqoo1mh2phj21540yc7ap.jpg" width="500px"/>
<br>

运行结果:


<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqoo3whrdgj20pi1akjtl.jpg" width="250px"/>

## 做一下简单的密钥保存
先专门弄一个类来生命native方法和加载so库，就叫SecurityUtil类吧
放在包名的目录下（注意，定义native方法的时候，跟加载so库所在的类有关系，比如这里的SecurityUtil类，我放在com.a.b目录下，在这个类的静态代码块加载库，System.loadLibrary("native-lib");那么，定义native方法的格式就应该是Java_com_a_b_方法名,而当我放在com.a.b.c下，那native方法的格式就是Java_com_a_b_c_方法名，具体可以查一下jni方法的命名）
在cpp文件夹新建一个security.cpp文件，改一下CMakeLists文件让他编译我们的cpp文件,新增一个getSecret方法,运行


<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqow77c2f7j20rq0he415.jpg" width=350/>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqow8ie1rpj20q40cu0un.jpg" width=350/>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqow93dzcaj20uo0i8mzk.jpg" width=350/>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqowe7ewqzj215q1hjtjm.jpg" width = 350/>

运行效果:

<br>
<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqow9hxximj20pi1ak40t.jpg" width = 350/>


然后可以看到包名/build/intermediate/cmake目录下已经生成了so文件


## 签名校验
上一步做的密钥保存生成的so库文件其实也不安全，由于jni的机制是反射实现的java端和native端的互相调用，所以在打包时候不能混淆native方法，所以只需要反编译提取出so文件和知道相应的native方法名，自己新建一个应用load一下那个库调用一下方法，那个密钥串就能简单被debug出来，所以我们一般会做应用的签名校验，当签名不一致时直接崩溃就好了。

java端获取应用签名的方法
```
PackageManager pm = context.getPackageManager();
PackageInfo pi = pm.getPackageInfo(context.getPackageName(), PackageManager.GET_SIGNATURES);
Signature[] signatures = pi.signatures;
Signature signature0 = signatures[0];
signature0.toCharsString();
```
获取签名跟context对象有关，所以假如重写一下context和packagemanager，还是有可能仿冒正确的context对象，这里使用反射ActivityThread类，然后通过invoke反射调用currentApplication方法获取当前的context对象，避免由参数传入context对象的方式增大破解的难度
对应到native端，获取context的代码就变成了
```
static jobject getApplication(JNIEnv *env) {
    jobject application = NULL;
    jclass activity_thread_clz = env->FindClass("android/app/ActivityThread");
    if (activity_thread_clz != NULL) {
        jmethodID currentApplication = env->GetStaticMethodID(
                activity_thread_clz, "currentApplication", "()Landroid/app/Application;");
        if (currentApplication != NULL) {
            application = env->CallStaticObjectMethod(activity_thread_clz, currentApplication);
        } else {
            LOGE("Cannot find method: currentApplication() in ActivityThread.");
        }
        env->DeleteLocalRef(activity_thread_clz);
    } else {
        LOGE("Cannot find class: android.app.ActivityThread");
    }

    return application;
}
```
验证签名的时机，放在jni的onload方法，也就是当加载动态库的时候就进行签名校验，跟内置的串进行对比，校验不通过直接崩溃。

所以最后的cpp文件如下

```
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
```


换一个签名再运行或者改一下内置的签名串让它不相等：

<br>

<img src="http://ww1.sinaimg.cn/large/64fe80f1gy1fqp08vgfq9j20pi1akdwb.jpg" width=300/>

<br>

这样就完成了签名校验过程
打个包会发现 包名/build/intermediate/cmake目录下已经生成了各个abi对应的so文件，当然也可以在gradle文件指定需要的abi



## so文件相关
不同 Android 手机使用不同的 CPU，因此支持不同的指令集。CPU 与指令集的每种组合都有其自己的应用二进制界面（或 ABI）。 ABI 可以非常精确地定义应用的机器代码在运行时如何与系统交互。 您必须为应用要使用的每个 CPU 架构指定 ABI。
<br>
[官网对abi的介绍](https://developer.android.com/ndk/guides/abis.html)




### 兼容方面
对于CPU来说，不同的架构并不意味着一定互不兼容，根据目前Android共支持七种不同类型的CPU架构，其兼容特点可总结如下：
- armeabi设备只兼容armeabi；
- armeabi-v7a设备兼容armeabi-v7a、armeabi；
- arm64-v8a设备兼容arm64-v8a、armeabi-v7a、armeabi；
- X86设备兼容X86、armeabi；
- X86_64设备兼容X86_64、X86、armeabi；
- mips64设备兼容mips64、mips；
- mips只兼容mips；
armeabi的so能运行在除了mips和mips64的设备上（在非armeabi上性能有所损耗）
64位的CPU架构总能向下兼容其对应的32位指令集

### 适配方面
- ARM架构几乎垄断，而且绝大部分都是armeabi-v7a、arm64-v8a，几乎可以不考虑X86、X86_64、mips、mips64架构
- 假如你只提供了armeabi，而没有提供v7a,v8a，且某些so文件在v7a或v8a架构中使用armeabi的so时性能差距明显，则可以考虑单独把那份so的v7a,v8a的so文件也放进armeabi的文件夹，通过代码判断当前cpu架构然后再来加载对应的so文件

