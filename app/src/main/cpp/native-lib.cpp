#include <jni.h>
#include <string>
#include "opencv2/opencv.hpp"
#include <android/native_window_jni.h>
ANativeWindow *window = 0;
using namespace cv;

DetectionBasedTracker *tracker = 0;
class CascadeDetectorAdapter: public DetectionBasedTracker::IDetector
{
public:
    CascadeDetectorAdapter(cv::Ptr<cv::CascadeClassifier> detector):
            IDetector(),
            Detector(detector)
    {
    }
    void detect(const cv::Mat &Image, std::vector<cv::Rect> &objects)
    {
        Detector->detectMultiScale(Image, objects, scaleFactor, minNeighbours, 0, minObjSize, maxObjSize);
    }

    virtual ~CascadeDetectorAdapter()
    {
    }

private:
    CascadeDetectorAdapter();
    cv::Ptr<cv::CascadeClassifier> Detector;
};


extern "C"
JNIEXPORT void JNICALL
Java_com_yu_opencvdemo_OpencvJni_init(JNIEnv *env, jobject instance, jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);


    //创建检测器  Ptr是智能指针，不需要关心释放
    Ptr<CascadeClassifier> mainClassifier = makePtr<CascadeClassifier>(path);
    Ptr<CascadeDetectorAdapter> mainDetector = makePtr<CascadeDetectorAdapter>(mainClassifier);

    //创建跟踪器
    Ptr<CascadeClassifier> trackClassifier = makePtr<CascadeClassifier>(path);
    Ptr<CascadeDetectorAdapter> trackingDetector = makePtr<CascadeDetectorAdapter>(trackClassifier);

    //开始识别，结果在CascadeDetectorAdapter中返回
    DetectionBasedTracker::Parameters DetectorParams;
    tracker= new DetectionBasedTracker(mainDetector, trackingDetector, DetectorParams);
    tracker->run();

    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_yu_opencvdemo_OpencvJni_postData(JNIEnv *env, jobject instance, jbyteArray data_,
                                          jint width, jint height, jint cameraId) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    // 数据的行数也就是数据高度，因为数据类型是NV21，所以为Y+U+V的高度, 也就是height + height/4 + height/4
    Mat src(height*3/2, width, CV_8UC1, data);

    // 转RGB
    cvtColor(src, src, COLOR_YUV2RGBA_NV21);
    if (cameraId == 1) {// 前置摄像头
        //逆时针旋转90度
        rotate(src, src, ROTATE_90_COUNTERCLOCKWISE);
        //1：水平翻转   0：垂直翻转
        flip(src, src, 1);
    } else {
        //顺时针旋转90度
        rotate(src, src, ROTATE_90_CLOCKWISE);

    }
    Mat gray;
    //灰度化
    cvtColor(src, gray, COLOR_RGBA2GRAY);
    //二值化
    equalizeHist(gray, gray);

    std::vector<Rect> faces;
    //检测图片
    tracker->process(gray);
    //获取CascadeDetectorAdapter中的检测结果
    tracker->getObjects(faces);
    //画出矩形
    for (Rect face : faces) {
        rectangle(src, face, Scalar(255, 0, 0));
    }

    //显示到serface
    if (window) {
        ANativeWindow_setBuffersGeometry(window, src.cols, src.rows, WINDOW_FORMAT_RGBA_8888);
        ANativeWindow_Buffer window_buffer;
        do {
            //lock失败 直接brek出去
            if (ANativeWindow_lock(window, &window_buffer, 0)) {
                ANativeWindow_release(window);
                window = 0;
                break;
            }

            uint8_t *dst_data = static_cast<uint8_t *>(window_buffer.bits);
            //stride : 一行多少个数据
            //（RGBA） * 4
            int dst_linesize = window_buffer.stride * 4;

            //一行一行拷贝，src.data是图片的RGBA数据，要拷贝到dst_data中，也就是window的缓冲区里
            for (int i = 0; i < window_buffer.height; ++i) {
                memcpy(dst_data + i * dst_linesize, src.data + i * src.cols * 4, dst_linesize);
            }
            //提交刷新
            ANativeWindow_unlockAndPost(window);
        } while (0);


    }
    src.release();
    gray.release();
    env->ReleaseByteArrayElements(data_, data, 0);
}


extern "C"
JNIEXPORT void JNICALL
Java_com_yu_opencvdemo_OpencvJni_setSurface(JNIEnv *env, jobject instance, jobject surface) {

    if (window) {
        ANativeWindow_release(window);
        window = 0;
    }
    window = ANativeWindow_fromSurface(env, surface);

}