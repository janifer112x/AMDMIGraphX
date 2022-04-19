#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <fstream>
#include <queue>
#include <numeric>
#include <random>
#include <string>
#include <vector>

#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/videoio.hpp>

#include <migraphx/migraphx.hpp>

void make_nxn(cv::Mat& image, int n)
{
    const auto image_height = image.rows;
    const auto image_width  = image.cols;

    if(image_height != image_width)
    {
        float scale   = image_height < image_width ? 256.f / image_height : 256.f / image_width;
        auto new_size = cv::Size(image_height * scale, image_height * scale);
        cv::resize(image, image, new_size, cv::INTER_LINEAR);

        const int offsetW = (image.cols - n) / 2;
        const int offsetH = (image.rows - n) / 2;
        const cv::Rect crop(offsetW, offsetH, n, n);
        image = image(crop);
    }
    else
    {
        cv::resize(image, image, cv::Size(n, n), cv::INTER_LINEAR);
    }
}

std::vector<float> preprocess(cv::Mat& image)
{
    const auto image_size = 224;
    const auto channels   = 3;
    const auto scale      = 1.0;

    make_nxn(image, image_size);

    image.convertTo(image, CV_32F, 1.0 / 255.0);

    const std::array<float, 3> mean{0.485, 0.456, 0.406};
    const std::array<float, 3> std_dev{0.229, 0.224, 0.225};

    for(int h = 0; h < image.rows; h++)
    {
        for(int w = 0; w < image.cols; w++)
        {
            for(int c = 0; c < 3; c++)
            {
                image.at<cv::Vec3f>(h, w)[c] = (image.at<cv::Vec3f>(h, w)[c] - mean.at(c)) * scale;
            }
        }
    }

    std::vector<float> chw_buffer(image.elemSize() * image.total());
    cv::Mat transposed[4];
    for(int j = 0; j < channels; j++)
        transposed[j] = cv::Mat(
            image.rows, image.cols, CV_32F, chw_buffer.data() + j * image_size * image_size);
    std::swap(transposed[0], transposed[2]);
    split(image, transposed);

    return chw_buffer;
}

// void calc_softmax(const float* data, size_t size, double* result) {
//   double sum = 0;

//   auto max = data[0];
//   for (size_t i = 1; i < size; i++) {
//     if (data[i] > max) {
//       max = data[i];
//     }
//   }

//   for (size_t i = 0; i < size; i++) {
//     result[i] = exp(data[i] - max);
//     sum += result[i];
//   }

//   for (size_t i = 0; i < size; i++) {
//     result[i] /= sum;
//   }
// }

// /**
//  * @brief After running softmax, get the labels associated with the top k values
//  *
//  * @param d pointer to the data
//  * @param size number of elements in the data
//  * @param k number of top elements to return
//  * @return std::vector<int>
//  */
// std::vector<int> get_top_k(const double* d, int size, int k) {
//   std::priority_queue<std::pair<float, int>> q;

//   for (auto i = 0; i < size; ++i) {
//     q.push(std::pair<float, int>(d[i], i));
//   }
//   std::vector<int> topKIndex;
//   for (auto i = 0; i < k; ++i) {
//     std::pair<float, int> ki = q.top();
//     q.pop();
//     topKIndex.push_back(ki.second);
//   }
//   return topKIndex;
// }

// /**
//  * @brief Perform postprocessing of the data
//  *
//  * @param output output from Proteus
//  * @param k number of top elements to return
//  * @return std::vector<int>
//  */
// std::vector<int> postprocess(float* output, int k) {
//   auto* data = static_cast<std::vector<float>*>(output.getData());
//   auto size = output.getSize();

//   std::vector<double> softmax;
//   softmax.resize(size);

//   calc_softmax(data->data(), size, softmax.data());
//   return get_top_k(softmax.data(), size, k);
// }

int main(int argc, char** argv)
{
    if(argc == 1)
    {
        std::cout << "Usage: " << argv[0] << " [options]" << std::endl
                  << "options:" << std::endl
                  << "\t -c, --cpu      Compile for CPU" << std::endl
                  << "\t -g, --gpu      Compile for GPU" << std::endl
                  << "\t -f, --fpga     Compile for FPGA" << std::endl
                  << "\t -p, --print    Print Graph at Each Stage" << std::endl
                  << std::endl
                  << std::endl;
    }

    char** begin   = argv + 1;
    char** end     = argv + argc;
    const bool CPU = (std::find(begin, end, std::string("-c")) != end) ||
                     std::find(begin, end, std::string("--cpu")) != end;
    const bool GPU = std::find(begin, end, std::string("-g")) != end ||
                     std::find(begin, end, std::string("--gpu")) != end;
    const bool FPGA = std::find(begin, end, std::string("-f")) != end ||
                      std::find(begin, end, std::string("--fpga")) != end;
    const bool PRINT = std::find(begin, end, std::string("-p")) != end ||
                       std::find(begin, end, std::string("--print")) != end;

    // const std::string base_path = "..";
    const std::string base_path = "/workspace/AMDMIGraphX/examples/vision/cpp_resnet50";
    const auto onnx_file        = base_path + "/resnet50.onnx";
    const std::string base_save = "_resnet.mxr";

    migraphx::program prog;
    migraphx::onnx_options onnx_opts;
    prog = parse_onnx(onnx_file.c_str(), onnx_opts);

    std::cout << "Parsing ONNX model..." << std::endl;
    if(PRINT)
        prog.print();
    std::cout << std::endl;

    std::string target_str;
    if(CPU)
        target_str = "cpu";
    else if(FPGA)
        target_str = "fpga";
    else if(GPU)
        target_str = "gpu";
    else
        target_str = "ref";
    migraphx::target targ = migraphx::target(target_str.c_str());

    const auto save_file = target_str + base_save;
    // try{
    //     prog = migraphx::load(save_file.c_str());
    // } catch(const std::runtime_error& e){
    if(GPU)
    {
        migraphx::compile_options comp_opts;
        comp_opts.set_offload_copy();
        prog.compile(targ, comp_opts);
    }
    else if(FPGA)
    {
        prog.compile(targ);
    }
    else
    {
        prog.compile(targ);
    }

    std::cout << "Compiling program for " << target_str << "..." << std::endl;
    if(PRINT)
        prog.print();
    std::cout << std::endl;
    migraphx::save(prog, save_file.c_str());
    // }

    const std::string video = base_path + "/sample_vid.mp4";
    cv::VideoCapture cap(video); // open the video file
    if(!cap.isOpened())
    { // check if we succeeded
        throw std::runtime_error("Video sample_vid.mp4 not opened successfully");
    }

    // const auto frames_to_read = static_cast<int>(
    //     cap.get(cv::VideoCaptureProperties::CAP_PROP_FRAME_COUNT));

    const auto frames_to_read = 5;

    for(auto i = 0; i < frames_to_read; i++)
    {

        cv::Mat frame;
        cap >> frame; // get the next frame from video
        if(frame.empty())
        {
            throw std::runtime_error("Frame empty");
        }

        auto data = preprocess(frame);

        migraphx::program_parameters prog_params;
        auto param_shapes = prog.get_parameter_shapes();
        auto input        = param_shapes.names().front();
        prog_params.add(input, migraphx::argument(param_shapes[input], data.data()));

        std::cout << "Model evaluating input..." << std::endl;
        auto start   = std::chrono::high_resolution_clock::now();
        auto outputs = prog.eval(prog_params);
        auto stop    = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(stop - start);
        std::cout << "Inference complete" << std::endl;
        std::cout << "Inference time: " << elapsed.count() * 1e-3 << "s" << std::endl;

        auto shape   = outputs[0].get_shape();
        auto lengths = shape.lengths();
        auto num_results =
            std::accumulate(lengths.begin(), lengths.end(), 1, std::multiplies<size_t>());
        float* results = reinterpret_cast<float*>(outputs[0].data());
        float* max     = std::max_element(results, results + num_results);
        int answer     = max - results;

        std::cout << std::endl
                  //   << "Randomly chosen digit: " << rand_digit << std::endl
                  << "Result from inference: " << answer << std::endl
                  << std::endl
                  //   << (answer == rand_digit ? "CORRECT" : "INCORRECT") << std::endl
                  << std::endl;
    }

    return 0;
}
