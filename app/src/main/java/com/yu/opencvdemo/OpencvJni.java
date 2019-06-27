package com.yu.opencvdemo;

import android.view.Surface;

public class OpencvJni {
    static {
        System.loadLibrary("native-lib");
    }

    /**
     * 初始化native层相关逻辑
     * @param path 人脸识别模型路径
     */
    public native void init(String path) ;

    /**
     * 摄像头采集的数据发送到native层
     * @param data 每一帧数据（NV21）
     * @param width 摄像头采集图片的宽
     * @param height 摄像头采集图片的高
     * @param cameraId 摄像头ID（前置、后置）
     */
    public native void postData(byte[] data, int width, int height, int cameraId);

    /**
     * 发送Surface到native，用于把数据后的图像数据直接显示到Surface上
     * @param surface
     */
    public native void setSurface(Surface surface);
}
