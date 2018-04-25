package com.chenzhihui.demo;

public class SecurityUtil {
    static {
        System.loadLibrary("security");
    }

    public static native String getSecret();
}
