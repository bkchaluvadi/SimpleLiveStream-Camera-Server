#include "httplib.h"
#include <opencv2/opencv.hpp>
#include <sstream>

int main(void) {
	httplib::Server svr;

	svr.Get("/video_feed", [&](const httplib::Request&, httplib::Response& res) {
		res.set_content_provider("multipart/x-mixed-replace; boundary=frame",
		[&](size_t offset, httplib::DataSink& sink) {
				cv::VideoCapture camera; // Move the camera object inside the lambda

				// Open the default camera (Webcam) when a client connects
				if (!camera.open(0)) {
					std::cerr << "Error opening video stream from webcam" << std::endl;
					sink.done();
					return false;
				}

				while (sink.is_writable()) {
					cv::Mat frame;
					camera >> frame; // Capture a frame from the webcam

					if (frame.empty()) {
						camera.release(); // Release the camera when done
						sink.done();
						return false;
					}

					std::vector<uchar> buffer;
					cv::imencode(".jpg", frame, buffer);
					std::stringstream ss;
					ss << "--frame\r\nContent-Type: image/jpeg\r\n\r\n";
					ss.write(reinterpret_cast<const char*>(buffer.data()), buffer.size());
					ss << "\r\n";
					sink.write(ss.str().c_str(), ss.str().size());
				}

				camera.release(); // Release the camera when the client disconnects
				return true;
			});
		});

	svr.Get("/", [&](const httplib::Request&, httplib::Response& res) {
		res.set_content("text/html", "<html><body><img src=\"/video_feed\" /></body></html>");
		});

	svr.listen("localhost", 8080);

	return 0;
}
