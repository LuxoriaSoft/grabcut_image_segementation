#include <opencv2/opencv.hpp>
#include <iostream>
#include <tuple>
#include <vector>

cv::Mat multi_scale_canny(const cv::Mat& image, const std::vector<double>& sigma_list = {1.0, 2.0, 3.0}) {
    cv::Mat edges_combined = cv::Mat::zeros(image.size(), CV_8U);

    for (double sigma : sigma_list) {
        cv::Mat blurred;
        int kernel_size = std::max(3, static_cast<int>(std::round(sigma * 2.0 + 1)));
        cv::GaussianBlur(image, blurred, cv::Size(kernel_size, kernel_size), sigma);
        cv::Mat edges;
        cv::Canny(blurred, edges, 50, 150);
        edges_combined = cv::max(edges_combined, edges);
    }

    cv::Mat edges_refined;
    cv::morphologyEx(edges_combined, edges_refined, cv::MORPH_CLOSE, cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3)));

    return edges_refined;
}

cv::Mat grabcut_foreground(const cv::Mat& image) {
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    mask.setTo(cv::GC_BGD);  // Initialize all pixels as background

    cv::Mat bgd_model, fgd_model;
    bgd_model.create(1, 65, CV_64F);
    fgd_model.create(1, 65, CV_64F);

    int border = std::min(image.cols, image.rows) / 10;
    cv::Rect rect(border, border, image.cols - 2 * border, image.rows - 2 * border);

    // Ensure that the bounding box is large enough
    if (rect.width <= 1 || rect.height <= 1) {
        std::cerr << "Error: Bounding box is too small!" << std::endl;
        return cv::Mat::zeros(image.size(), CV_8UC1);
    }

    try {
        // Perform GrabCut with 5 iterations
        cv::grabCut(image, mask, rect, bgd_model, fgd_model, 5, cv::GC_INIT_WITH_RECT);
    } catch (const cv::Exception& e) {
        std::cerr << "OpenCV Exception: " << e.what() << std::endl;
        return cv::Mat::zeros(image.size(), CV_8UC1);
    }

    // Create binary mask for foreground (GC_FGD and GC_PR_FGD are foreground labels)
    cv::Mat fg_mask = (mask == cv::GC_FGD) | (mask == cv::GC_PR_FGD);
    fg_mask.convertTo(fg_mask, CV_64F);  // Convert to double for further computations

    return fg_mask;
}

std::tuple<cv::Mat, double, double, double> compute_foreground_background_probability(const cv::Mat& image_rgb, const cv::Mat& edges_refined) {
    std::cout << "Computing foreground and background probabilities... ";
    cv::Mat fg_prob = grabcut_foreground(image_rgb);  // Get foreground probability
    std::cout << "done!" << std::endl;

    std::cout << "Normalizing foreground probability to [0, 1]... ";
    fg_prob.convertTo(fg_prob, CV_64F, 1.0 / 255.0);  // Normalize fg_prob to range [0, 1]
    std::cout << "done!" << std::endl;

    // Ensure both fg_prob and edges_refined are of the same type (CV_64F for floating-point operations)
    std::cout << "Converting edge map to floating-point... ";
    if (fg_prob.type() != CV_64F) {
        fg_prob.convertTo(fg_prob, CV_64F);
    }
    std::cout << "done!" << std::endl;

    std::cout << "Converting edge map to floating-point... ";
    cv::Mat edges_refined_64F;
    edges_refined.convertTo(edges_refined_64F, CV_64F);
    edges_refined_64F /= 255.0;  // Normalize edges_refined to [0, 1]
    std::cout << "done!" << std::endl;

    // Calculate the foreground score (mean of foreground probability)
    std::cout << "Calculating foreground and background scores... ";
    double foreground_score = cv::mean(fg_prob)[0];
    double background_score = 1.0 - foreground_score;  // Background is the complement
    std::cout << "done!" << std::endl;

    // Calculate edge-weighted foreground score (based on refined edges)
    std::cout << "Calculating edge-weighted foreground score... ";
    double edge_weighted_fg = 0.0;
    if (cv::sum(edges_refined_64F)[0] > 0) {  // Check if edges exist
        edge_weighted_fg = cv::sum(fg_prob.mul(edges_refined_64F))[0] / cv::sum(edges_refined_64F)[0];
    }
    std::cout << "done!" << std::endl;

    return std::make_tuple(fg_prob, foreground_score, background_score, edge_weighted_fg);
}

int main(int ac, char** av) {
    if (ac != 2) {
        std::cout << "Usage: ./grabcut_edge_detection <image_path>" << std::endl;
        return -1;
    }

    cv::Mat image = cv::imread(av[1], cv::IMREAD_COLOR);  // Load the image in color
    if (image.empty()) {
        std::cout << "Could not open or find the image!" << std::endl;
        return -1;
    }

    // Resize image if it's too small
    if (image.cols < 100 || image.rows < 100) {
        std::cout << "Resizing image to avoid failure with GrabCut..." << std::endl;
        cv::resize(image, image, cv::Size(100, 100));  // Resize to a minimum of 100x100
    }

    std::cout << "Loaded image with size: " << image.cols << " x " << image.rows << std::endl;

    // Convert the image to grayscale for edge detection
    cv::Mat gray_image;
    cv::cvtColor(image, gray_image, cv::COLOR_BGR2GRAY);

    std::cout << "Applying multi-scale Canny edge detection..." << std::endl;
    cv::Mat edges_refined = multi_scale_canny(gray_image);  // Apply multi-scale Canny edge detection

    std::cout << "Computing foreground and background probabilities..." << std::endl;
    cv::Mat fg_prob;
    double foreground_score, background_score, edge_weighted_fg;
    std::tie(fg_prob, foreground_score, background_score, edge_weighted_fg) = compute_foreground_background_probability(image, edges_refined);

    // Output the computed probabilities
    std::cout << "Foreground Probability Score: " << foreground_score << std::endl;
    std::cout << "Background Probability Score: " << background_score << std::endl;
    std::cout << "Edge-Weighted Foreground Score: " << edge_weighted_fg << std::endl;

    return 0;
}
