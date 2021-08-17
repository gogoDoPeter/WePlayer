package com.peter.weplayer.player;

import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.NonNull;

import com.peter.weplayer.MainActivity;

public class WePlayer implements SurfaceHolder.Callback {
    private static final String TAG = "player_" + WePlayer.class.getSimpleName();
    private String dataSource;// 媒体源（文件路径， 直播地址rtmp）
    private SurfaceHolder surfaceHolder;

    // Used to load the 'native-lib' library on application startup.
    static {
        System.loadLibrary("native-lib");
    }

    private OnPreparedListener onPreparedListener;// C++层准备情况的接口

    public WePlayer() {

    }

    public void setDataSource(String dataSource) {
        this.dataSource = dataSource;
    }

    /**
     * 给jni反射调用的, 底层jni调用，提示prepare ok，从mainActivity启动player的start
     */
    public void onPrepared() {
        Log.d(TAG, "onPrepared +");
        if (onPreparedListener != null) {
            Log.d(TAG, "底层jni调用  onPrepared +");
            onPreparedListener.onPrepared();
        }
        Log.d(TAG, "onPrepared -");
    }

    public void setOnPreparedListener(OnPreparedListener onPreparedListener) {
        Log.d(TAG, "setOnPreparedListener ");
        this.onPreparedListener = onPreparedListener;
    }

    public String getFfmpegVersion() {
        return getFfmpegVersionNative();
    }

    /**
     * set SurfaceView
     *
     * @param surfaceView
     */
    public void setSurfaceView(SurfaceView surfaceView) {
        if (this.surfaceHolder != null) {
            surfaceHolder.removeCallback(this); // 清除上一次的
        }
        surfaceHolder = surfaceView.getHolder();//是否可以调整下顺序，和上面一句
        surfaceHolder.addCallback(this); // 监听
    }

    @Override
    public void surfaceCreated(@NonNull SurfaceHolder holder) {
        // 之前开发是：写这里的
    }

    @Override   // 界面发生了改变
    public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        setSurfaceNative(surfaceHolder.getSurface());
    }

    @Override
    public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
    }

    /**
     * 准备的监听接口
     */
    public interface OnPreparedListener {
        void onPrepared();
    }

    /**
     * 播放前的 准备工作
     */
    public void prepare() {
        Log.d(TAG, "prepare +");
        prepareNative(dataSource);
        Log.d(TAG, "prepare -");
    }

    /**
     * 开始播放
     */
    public void start() {
        Log.d(TAG, "start +");
        startNative();
        Log.d(TAG, "start -");
    }

    public void stop() {
        stopNative();
    }


    public void release() {
        releaseNative();
    }


    // TODO >>>>>>>>>>> 下面是native函数区域
    private native void prepareNative(String dataSource);

    private native void startNative();

    private native void stopNative();

    private native void releaseNative();

    /**
     * A native method that is implemented by the 'native-lib' native library,
     * which is packaged with this application.
     */
//    public native String stringFromJNI();
    private native String getFfmpegVersionNative();

    private native void setSurfaceNative(Surface surface);

}
