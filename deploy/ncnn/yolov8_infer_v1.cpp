// YOLOv8 ncnn inference - static binary for OHOS/musl
// Compile: aarch64-linux-gnu-g++ -O2 -static -I/tmp/ncnn/include/ncnn
//          yolov8_ncnn_infer.cpp -o yolov8_infer
//          /tmp/ncnn/build-aarch64/src/libncnn.a -lgomp -lpthread -lm

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <vector>
#include <algorithm>

#include "net.h"
#include "simpleocv.h"  // ncnn built-in minimal cv

#define INPUT_W 640
#define INPUT_H 640
#define NUM_CLASSES 80
#define NMS_THRESH 0.45f
#define CONF_THRESH 0.25f

// COCO class names
static const char* CLASS_NAMES[] = {
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

struct Detection {
    float x1, y1, x2, y2;
    float score;
    int class_id;
};

static float iou(const Detection& a, const Detection& b) {
    float ix1 = std::max(a.x1, b.x1), iy1 = std::max(a.y1, b.y1);
    float ix2 = std::min(a.x2, b.x2), iy2 = std::min(a.y2, b.y2);
    float iw = std::max(0.f, ix2 - ix1), ih = std::max(0.f, iy2 - iy1);
    float inter = iw * ih;
    float ua = (a.x2-a.x1)*(a.y2-a.y1) + (b.x2-b.x1)*(b.y2-b.y1) - inter;
    return inter / (ua + 1e-6f);
}

static void nms(std::vector<Detection>& dets, float thresh) {
    std::sort(dets.begin(), dets.end(), [](const Detection& a, const Detection& b){
        return a.score > b.score;
    });
    std::vector<bool> suppressed(dets.size(), false);
    for (size_t i = 0; i < dets.size(); i++) {
        if (suppressed[i]) continue;
        for (size_t j = i+1; j < dets.size(); j++) {
            if (!suppressed[j] && dets[i].class_id == dets[j].class_id) {
                if (iou(dets[i], dets[j]) > thresh) suppressed[j] = true;
            }
        }
    }
    std::vector<Detection> out;
    for (size_t i = 0; i < dets.size(); i++)
        if (!suppressed[i]) out.push_back(dets[i]);
    dets = out;
}

int main(int argc, char** argv) {
    if (argc < 3) {
        printf("Usage: %s <model.param> <model.bin> [image.jpg]\n", argv[0]);
        printf("  model.param/bin: ncnn model files (yolov8n_ncnn_model/)\n");
        return 1;
    }

    const char* param_path = argv[1];
    const char* bin_path   = argv[2];
    const char* img_path   = argc > 3 ? argv[3] : NULL;

    // Load model
    ncnn::Net net;
    net.opt.use_vulkan_compute = false;
    net.opt.num_threads = 4;  // Use 4 A76 cores

    if (net.load_param(param_path) != 0) {
        printf("ERROR: Failed to load %s\n", param_path);
        return 1;
    }
    if (net.load_model(bin_path) != 0) {
        printf("ERROR: Failed to load %s\n", bin_path);
        return 1;
    }
    printf("Model loaded: %s / %s\n", param_path, bin_path);

    // Prepare input
    int img_w = INPUT_W, img_h = INPUT_H;
    ncnn::Mat in;

    if (img_path) {
        // Load image using simpleocv (BMP support built-in)
        // For JPEG we need a simple loader - use ncnn's from_pixels
        FILE* f = fopen(img_path, "rb");
        if (!f) { printf("Cannot open image: %s\n", img_path); return 1; }

        // Read raw PPM/BMP - try as raw RGB if file ends with .rgb
        // Simplest approach: create test pattern if file parsing complex
        fseek(f, 0, SEEK_END);
        long fsize = ftell(f);
        fclose(f);
        printf("Image file: %s (%ld bytes)\n", img_path, fsize);
        printf("Note: For full image loading, link with stb_image\n");

        // Create test pattern for now
        unsigned char* pixels = (unsigned char*)malloc(INPUT_W * INPUT_H * 3);
        for (int i = 0; i < INPUT_W * INPUT_H * 3; i++) pixels[i] = 128;
        in = ncnn::Mat::from_pixels_resize(pixels, ncnn::Mat::PIXEL_BGR, INPUT_W, INPUT_H, INPUT_W, INPUT_H);
        free(pixels);
    } else {
        // Test with blank image
        printf("No image provided, running inference on blank 640x640...\n");
        unsigned char* pixels = (unsigned char*)calloc(INPUT_W * INPUT_H * 3, 1);
        in = ncnn::Mat::from_pixels_resize(pixels, ncnn::Mat::PIXEL_BGR, INPUT_W, INPUT_H, INPUT_W, INPUT_H);
        free(pixels);
    }

    // Normalize
    const float mean_vals[3] = {0.f, 0.f, 0.f};
    const float norm_vals[3] = {1/255.f, 1/255.f, 1/255.f};
    in.substract_mean_normalize(mean_vals, norm_vals);

    // Run inference (with timing)
    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    ncnn::Extractor ex = net.create_extractor();
    ex.input("images", in);

    ncnn::Mat out;
    ex.extract("output0", out);

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ms = (t1.tv_sec - t0.tv_sec) * 1000.0 + (t1.tv_nsec - t0.tv_nsec) / 1e6;
    printf("Inference time: %.1f ms (%.1f FPS)\n", ms, 1000.0 / ms);

    // Parse detections
    // YOLOv8 output: [batch, 84, 8400] where 84 = 4(box) + 80(classes)
    printf("Output shape: %d x %d x %d\n", out.w, out.h, out.c);

    std::vector<Detection> dets;
    // out is [84, 8400] for YOLOv8
    int num_anchors = out.w; // 8400
    int num_features = out.h; // 84

    for (int i = 0; i < num_anchors; i++) {
        float cx = out.row(0)[i];
        float cy = out.row(1)[i];
        float w  = out.row(2)[i];
        float h  = out.row(3)[i];

        float max_score = 0;
        int max_cls = 0;
        for (int c = 0; c < NUM_CLASSES; c++) {
            float s = out.row(4 + c)[i];
            if (s > max_score) { max_score = s; max_cls = c; }
        }

        if (max_score < CONF_THRESH) continue;

        Detection d;
        d.x1 = cx - w / 2;
        d.y1 = cy - h / 2;
        d.x2 = cx + w / 2;
        d.y2 = cy + h / 2;
        d.score = max_score;
        d.class_id = max_cls;
        dets.push_back(d);
    }

    nms(dets, NMS_THRESH);

    printf("Detections: %zu\n", dets.size());
    for (const auto& d : dets) {
        printf("  [%s] score=%.3f box=(%.0f,%.0f,%.0f,%.0f)\n",
               CLASS_NAMES[d.class_id], d.score, d.x1, d.y1, d.x2, d.y2);
    }

    return 0;
}
