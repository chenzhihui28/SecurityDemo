package com.chenzhihui.demo;

import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.widget.TextView;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        TextView tv = findViewById(R.id.sample_text);
        String text = "secret = " + SecurityUtil.getSecret() + " sign = " + getSignJava();
        tv.setText(text);
        Log.e("sign = ", ""+getSignJava());
    }

    private String getSignJava() {
        String sign = "";
        try {
            PackageManager pm = getPackageManager();
            PackageInfo pi = pm.getPackageInfo(getPackageName(), PackageManager.GET_SIGNATURES);
            Signature[] signatures = pi.signatures;
            Signature signature0 = signatures[0];
            sign = signature0.toCharsString();
        } catch (Exception e) {
            e.printStackTrace();
        }
        return sign;
    }


}
