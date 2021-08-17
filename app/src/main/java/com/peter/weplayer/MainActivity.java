package com.peter.weplayer;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import android.Manifest;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.net.Uri;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.util.Log;
import android.view.SurfaceView;
import android.view.WindowManager;
import android.widget.TextView;
import android.widget.Toast;

//import com.peter.weplayer.databinding.ActivityMainBinding;
import com.peter.weplayer.player.WePlayer;

import java.io.File;

public class MainActivity extends AppCompatActivity {
    private static final String TAG="player_"+MainActivity.class.getSimpleName();
    private static final String[] permissions = new String[]{
            Manifest.permission.CAMERA,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };
//    private ActivityMainBinding binding;
    private WePlayer player;
    private String ffmpegVersion;
    private SurfaceView mSurfaceView;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        Log.d(TAG,"onCreate +");
        super.onCreate(savedInstanceState);

        getWindow().setFlags(WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON, WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);

//        binding = ActivityMainBinding.inflate(getLayoutInflater());
//        setContentView(binding.getRoot());
        setContentView(R.layout.activity_main);

        mSurfaceView = ((SurfaceView) findViewById(R.id.surfaceView));

        checkPermission();

        // Example of a call to a native method
        //ViewBinding  LiveData Jetpack中内容，稍后学习
//        TextView tv = binding.sampleText;
//        tv.setText(stringFromJNI());

        player = new WePlayer();
        player.setSurfaceView(mSurfaceView);
        ffmpegVersion = player.getFfmpegVersion();
        Log.d(TAG,"ffmpegVersion = "+ffmpegVersion);
//        player.setDataSource(new File(Environment.getExternalStorageDirectory() + File.separator + "demo.mp4")
//                .getAbsolutePath());
        ///mnt/shared/Other/testvideo/
        player.setDataSource(new File("/mnt/sdcard/shared/demo.mp4")
                .getAbsolutePath());

        // 准备成功的回调处    <----  C++ 子线程调用的
        player.setOnPreparedListener(new WePlayer.OnPreparedListener() {
            @Override
            public void onPrepared() {
                Log.d(TAG,"onPrepared +");
                runOnUiThread(new Runnable() {
                    @Override
                    public void run() {
                        Log.d(TAG,"onPrepared toast");
                        Toast.makeText(MainActivity.this, "准备成功，即将播放, "+ffmpegVersion, Toast.LENGTH_SHORT).show();
                    }
                });
                player.start();// 调用 C++ 开始播放
                Log.d(TAG,"onPrepared -");

            }
        });
        Log.d(TAG,"onCreate -");
    }

    @Override
    protected void onResume() {
        Log.d(TAG,"onResume +");
        super.onResume();
        player.prepare();
        Log.d(TAG,"onResume -");
    }

    @Override
    protected void onStop() {
        Log.d(TAG,"onStop +");
        super.onStop();
        player.stop();
        Log.d(TAG,"onStop -");
    }

    @Override
    protected void onDestroy() {
        Log.d(TAG,"onDestroy +");
        player.release();
        super.onDestroy();
        Log.d(TAG,"onDestroy -");
    }

    private void checkPermission() {
        Log.d(TAG,"checkPermission +");
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            for (String permission : permissions) {
                if (ContextCompat.checkSelfPermission(this, permission) != PackageManager.PERMISSION_GRANTED) {
                    ActivityCompat.requestPermissions(this, permissions, 200);
                    return;
                }
            }
        }
        Log.d(TAG,"checkPermission -");
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && requestCode == 200) {
            for (int i = 0; i < permissions.length; i++) {
                if (grantResults[i] != PackageManager.PERMISSION_GRANTED) {
                    Intent intent = new Intent();
                    intent.setAction(Settings.ACTION_APPLICATION_DETAILS_SETTINGS);
                    Uri uri = Uri.fromParts("package", getPackageName(), null);
                    intent.setData(uri);
                    startActivityForResult(intent, 200);
                    return;
                }
            }
        }
    }

    @Override
    protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
        super.onActivityResult(requestCode, resultCode, data);
        if (requestCode == 200 && resultCode == RESULT_OK) {
            checkPermission();
        }
    }

}