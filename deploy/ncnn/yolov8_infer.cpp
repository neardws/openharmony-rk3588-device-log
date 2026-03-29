// YOLOv8 ncnn CPU inference for OHOS (ARM64 static binary)
// Usage: ./yolov8_infer model.ncnn.param model.ncnn.bin [test|image.jpg]
//
// Build:
//   aarch64-linux-gnu-g++ -O2 -march=armv8.2-a+dotprod \
//     -I/tmp/ncnn/src -I/tmp/ncnn/build-aarch64/src \
//     yolov8_infer.cpp -o yolov8_infer -static \
//     /tmp/ncnn/build-aarch64/src/libncnn.a -lgomp -lpthread -lm

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include "net.h"

#define INPUT_W     640
#define INPUT_H     640
#define NUM_CLASSES 80
#define CONF_THRESH 0.25f
#define NMS_THRESH  0.45f

static const char* CLASS_NAMES[80] = {
    "person","bicycle","car","motorcycle","airplane","bus","train","truck","boat",
    "traffic light","fire hydrant","stop sign","parking meter","bench","bird","cat",
    "dog","horse","sheep","cow","elephant","bear","zebra","giraffe","backpack",
    "umbrella","handbag","tie","suitcase","frisbee","skis","snowboard","sports ball",
    "kite","baseball bat","baseball glove","skateboard","surfboard","tennis racket",
    "bottle","wine glass","cup","fork","knife","spoon","bowl","banana","apple",
    "sandwich","orange","broccoli","carrot","hot dog","pizza","donut","cake","chair",
    "couch","potted plant","bed","dining table","toilet","tv","laptop","mouse",
    "remote","keyboard","cell phone","microwave","oven","toaster","sink","refrigerator",
    "book","clock","vase","scissors","teddy bear","hair drier","toothbrush"
};

struct Det { float x1,y1,x2,y2,score; int cls; };

static float iou(const Det& a, const Det& b) {
    float ix1=std::max(a.x1,b.x1), iy1=std::max(a.y1,b.y1);
    float ix2=std::min(a.x2,b.x2), iy2=std::min(a.y2,b.y2);
    float iw=std::max(0.f,ix2-ix1), ih=std::max(0.f,iy2-iy1);
    float inter=iw*ih;
    return inter/((a.x2-a.x1)*(a.y2-a.y1)+(b.x2-b.x1)*(b.y2-b.y1)-inter+1e-6f);
}

static void nms(std::vector<Det>& d, float thr) {
    std::sort(d.begin(),d.end(),[](const Det& a,const Det& b){return a.score>b.score;});
    std::vector<bool> s(d.size(),false);
    for(size_t i=0;i<d.size();i++){
        if(s[i]) continue;
        for(size_t j=i+1;j<d.size();j++)
            if(!s[j]&&d[i].cls==d[j].cls&&iou(d[i],d[j])>thr) s[j]=true;
    }
    std::vector<Det> out; for(size_t i=0;i<d.size();i++) if(!s[i]) out.push_back(d[i]);
    d=out;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <model.param> <model.bin> [test]\n", argv[0]);
        printf("  test: run on random input (benchmark)\n");
        return 1;
    }

    printf("=== YOLOv8n ncnn CPU Inference ===\n");
    printf("Model: %s / %s\n", argv[1], argv[2]);

    ncnn::Net net;
    net.opt.use_vulkan_compute = false;
    net.opt.num_threads = 4;

    if (net.load_param(argv[1])) { fprintf(stderr,"load param failed\n"); return 1; }
    if (net.load_model(argv[2])) { fprintf(stderr,"load model failed\n"); return 1; }
    printf("Model loaded OK\n");

    // Create input: random test image
    int W = INPUT_W, H = INPUT_H;
    unsigned char* pixels = (unsigned char*)malloc(W * H * 3);
    // Fill with a simple gradient pattern (test input)
    for (int y=0; y<H; y++)
        for (int x=0; x<W; x++) {
            pixels[(y*W+x)*3+0] = (unsigned char)(x*255/W);  // B
            pixels[(y*W+x)*3+1] = (unsigned char)(y*255/H);  // G
            pixels[(y*W+x)*3+2] = 128;                        // R
        }

    ncnn::Mat in = ncnn::Mat::from_pixels_resize(pixels, ncnn::Mat::PIXEL_BGR2RGB, W, H, W, H);
    free(pixels);

    // Normalize [0,1]
    const float mean[3] = {0,0,0};
    const float norm[3] = {1/255.f,1/255.f,1/255.f};
    in.substract_mean_normalize(mean, norm);

    // Warmup
    {
        ncnn::Extractor ex = net.create_extractor();
        ex.input("in0", in);
        ncnn::Mat dummy; ex.extract("out0", dummy);
    }

    // Timed run (average of 5)
    double total_ms = 0;
    int RUNS = 5;
    ncnn::Mat out;
    for (int r=0; r<RUNS; r++) {
        struct timespec t0, t1;
        clock_gettime(CLOCK_MONOTONIC, &t0);
        ncnn::Extractor ex = net.create_extractor();
        ex.input("in0", in);
        ex.extract("out0", out);
        clock_gettime(CLOCK_MONOTONIC, &t1);
        double ms = (t1.tv_sec-t0.tv_sec)*1000.0+(t1.tv_nsec-t0.tv_nsec)/1e6;
        total_ms += ms;
        printf("  Run %d: %.1f ms\n", r+1, ms);
    }

    printf("\n=== Results ===\n");
    printf("Avg inference: %.1f ms (%.1f FPS)\n", total_ms/RUNS, 1000.0/(total_ms/RUNS));
    printf("Output shape: [%d, %d] (expect [84, 8400])\n", out.h, out.w);

    // Parse detections
    // YOLOv8 out: shape=[84, 8400], row 0-3=xywh, row 4-83=class scores
    std::vector<Det> dets;
    int na = out.w;  // 8400 anchors
    for (int i=0; i<na; i++) {
        float cx=out.row(0)[i], cy=out.row(1)[i], w=out.row(2)[i], h=out.row(3)[i];
        float max_s=0; int max_c=0;
        for (int c=0; c<NUM_CLASSES; c++) {
            float s=out.row(4+c)[i];
            if (s>max_s){ max_s=s; max_c=c; }
        }
        if (max_s < CONF_THRESH) continue;
        Det d; d.x1=cx-w/2; d.y1=cy-h/2; d.x2=cx+w/2; d.y2=cy+h/2;
        d.score=max_s; d.cls=max_c; dets.push_back(d);
    }
    nms(dets, NMS_THRESH);

    printf("Detections (conf>%.2f): %zu\n", CONF_THRESH, dets.size());
    for (size_t i=0; i<dets.size() && i<10; i++)
        printf("  %s (%.3f) [%.0f,%.0f,%.0f,%.0f]\n",
               CLASS_NAMES[dets[i].cls], dets[i].score,
               dets[i].x1, dets[i].y1, dets[i].x2, dets[i].y2);

    printf("\nDone! Binary is ready for OHOS deployment.\n");
    return 0;
}
