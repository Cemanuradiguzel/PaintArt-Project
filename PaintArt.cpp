#include <opencv2/opencv.hpp>
#include <iostream>
#include <vector>
#include <string>
#include <sstream>

using namespace cv;
using namespace std;

string shapes[] = { "LINE", "TRIANGLE", "RECTANGLE", "CIRCLE", "ERASER"};

struct drawingDetails {
    Mat picture;
    string window;
    bool paint = false;
    Point prevPoint = Point(-1, -1);
    vector<Point> points;
    int currentShape = 0;
    int eraserSize = 20;
    int pencilSize = 2;
    int colorCount = 0;
    Vec3b colors = Vec3b(255, 0, 0);
    Vec3b getColor() {
        switch (colorCount) {
        case 0:
            colors = Vec3b(255, 0, 0); //Red
            break;
        case 1:
            colors = Vec3b(0, 255, 0); //Green
            break;
        case 2:
            colors = Vec3b(0, 0, 255); //Blue
            break;
        case 3:
            colors = Vec3b(128, 128, 128); //Gray
            break;
        default:
            break;
        }
        return colors;
    }
};
struct drawTrackbarStruct {
    int currentShape = 0;
    int currentPencil = 10;
    int currentEraser = 10;
    int currentColor = 0;
    int maxValueShape = 4;
    int maxValuePencil = 30;
    int maxValueEraser = 50;
    int maxValueColor = 3;
    string windowName;
    drawingDetails* drawDetails;
};
void drawTrackbarControl(int position, void* userData) {
    drawTrackbarStruct* gtb = (drawTrackbarStruct*)userData;
    drawingDetails* drawDetails = gtb->drawDetails;

    drawDetails->currentShape = getTrackbarPos("Shape Type", gtb->windowName);
    drawDetails->pencilSize = getTrackbarPos("Pencil Size", gtb->windowName);
    drawDetails->eraserSize = getTrackbarPos("Eraser Size", gtb->windowName);
    drawDetails->colorCount = getTrackbarPos("Color", gtb->windowName);
}

struct textDetails {
    vector<string> lines;
    string currentLine = "";
    int fontFace = FONT_HERSHEY_SIMPLEX;
    int fontScale = 1;
    int thickness = 2;
    int lineSpacing = 30;
    int maxWidth = 20;
    Point textOrg;
    string windowName = "PaintArt";

    textDetails(int width)
        : currentLine(""), fontFace(FONT_HERSHEY_SIMPLEX), fontScale(1), thickness(2),
        lineSpacing(30), textOrg(10, 30), maxWidth(width - 20) {}
};

struct webcamTrackbarStruct {
    int brightness = 50;
    int contrast = 50;
    int frameWidth = 640;
    int frameHeight = 480;
    int maxBrightness = 100;
    int maxContrast = 100;
    int maxFrameWidth = 1280;
    int maxFrameHeight = 720;
};

void webcamTrackbarControl(int, void* data) {
    webcamTrackbarStruct* webcam = (webcamTrackbarStruct*)data;

    webcam->brightness = getTrackbarPos("Brightness", "Webcam Trackbar");
    webcam->contrast = getTrackbarPos("Contrast", "Webcam Trackbar");
    webcam->frameWidth = getTrackbarPos("Width", "Webcam Trackbar");
    webcam->frameHeight = getTrackbarPos("Height", "Webcam Trackbar");
}

class WindowProperties {
private:
    static void drawingOperationsStatic(int event, int x, int y, int flags, void* data) {
        WindowProperties* instance = reinterpret_cast<WindowProperties*>(data);
        instance->drawingOperations(event, x, y, flags, data);
    }
    void drawingOperations(int event, int x, int y, int flags, void* data) {
        drawingDetails* p = (drawingDetails*)data;

        if (event == EVENT_LBUTTONDOWN) {
            p->paint = true;
            p->prevPoint = Point(x, y);
            p->points.push_back(Point(x, y)); // Noktayı listeye ekle

            if (p->currentShape == 1 && p->points.size() == 3) {
                // Üç nokta tamamlandığında üçgeni çiz
                line(p->picture, p->points[0], p->points[1], Scalar(p->getColor()), p->pencilSize);
                line(p->picture, p->points[1], p->points[2], Scalar(p->getColor()), p->pencilSize);
                line(p->picture, p->points[2], p->points[0], Scalar(p->getColor()), p->pencilSize);
                imshow(p->window, p->picture);
                p->points.clear();
                p->paint = false;
                p->prevPoint = Point(-1, -1);
            }
            if (p->currentShape == 2 && p->points.size() == 2) {
                // İki nokta tamamlandığında dikdörtgeni çiz
                rectangle(p->picture, p->points[0], p->points[1], Scalar(p->getColor()), p->pencilSize);
                imshow(p->window, p->picture);
                p->points.clear();
                p->paint = false;
                p->prevPoint = Point(-1, -1);
            }
            if (p->currentShape == 3 && p->points.size() == 2) {
                // İki nokta tamamlandığında daireyi çiz
                Point center = p->points[0];
                int radius = cv::norm(p->points[0] - p->points[1]);
                circle(p->picture, center, radius, Scalar(p->getColor()), p->pencilSize);
                imshow(p->window, p->picture);
                p->points.clear();
                p->paint = false;
                p->prevPoint = Point(-1, -1);
            }
        }
        else if (event == EVENT_LBUTTONUP) {
            p->paint = false;
            p->prevPoint = Point(-1, -1);
        }

        if (p->paint) {
            if (p->currentShape == 4) {
                Mat mask = Mat::zeros(p->picture.size(), CV_8UC1);
                circle(mask, Point(x, y), p->eraserSize, Scalar(255), -1);
                Mat result;
                p->picture.copyTo(result);
                p->picture.setTo(Scalar(0, 0, 0), mask);
            }
            if (p->currentShape == 0 || p->points.size() < 3) {
                Point currentPoint(x, y);
                if (p->prevPoint.x >= 0 && p->prevPoint.y >= 0) {
                    line(p->picture, p->prevPoint, currentPoint, Scalar(p->getColor()), p->pencilSize);
                }
                p->prevPoint = currentPoint;

            }
            imshow(p->window, p->picture);
        }
    }
    void wrapText(const string& text, vector<string>& lines, int maxWidth,
        int fontFace, double fontScale, int thickness) {
        istringstream words(text);
        string word;
        string currentLine;

        while (words >> word) {
            string testLine = currentLine.empty() ? word : currentLine + " " + word;
            int baseline = 0;
            Size textSize = getTextSize(testLine, fontFace, fontScale, thickness, &baseline);

            if (textSize.width > maxWidth) {
                if (!currentLine.empty()) {
                    lines.push_back(currentLine);
                    currentLine = word;
                }
                else {
                    lines.push_back(testLine);
                    currentLine.clear();
                }
            }
            else {
                currentLine = testLine;
            }
        }

        if (!currentLine.empty())
            lines.push_back(currentLine);
    }

public:
    drawingDetails drawStruct;
    drawTrackbarStruct drawTb;
    textDetails textStruct;
    // webcamTrackbarStruct* webcamTb;
    WindowProperties() : drawStruct(), textStruct(1200) {}

    void printMenu() {
        cout << "PaintArt App" << endl;
        cout << "1. Draw the screen" << endl;
        cout << "2. Write text" << endl;
        cout << "3. Image processing" << endl;
        cout << "4. Webcam processing" << endl;
        cout << "5. Exit" << endl;
    }
    void createWindow() {
        drawStruct.picture = Mat::zeros(Size(800, 600), CV_8UC3);
        drawStruct.window = "PaintArt";
        namedWindow(drawStruct.window, WINDOW_AUTOSIZE);
        imshow(drawStruct.window, drawStruct.picture);
    }
    void setMouseCallback() {
        ::setMouseCallback(drawStruct.window, drawingOperationsStatic, this);
    }

    char handleTurkishKeys(int key) {
        unordered_map<int, char> turkishKeyMap = {
            {231, 'ç'}, {199, 'Ç'}, {287, 'ğ'}, {286, 'Ğ'}, {305, 'ı'}, {304, 'İ'},
            {351, 'ş'}, {350, 'Ş'}, {252, 'ü'}, {220, 'Ü'}, {246, 'ö'}, {214, 'Ö'}
        };

        if (turkishKeyMap.find(key) != turkishKeyMap.end()) {
            return turkishKeyMap[key];
        }

        return (char)key;
    }

    void writeText() {
        Mat display = Mat::zeros(600, 400, CV_8UC3);
        vector<string> displayLines = textStruct.lines;
        int y = textStruct.textOrg.y;
        wrapText(textStruct.currentLine, displayLines, textStruct.maxWidth, 
            textStruct.fontFace, textStruct.fontScale, textStruct.thickness);

        for (const string& line : displayLines) {
            putText(display, line, Point(textStruct.textOrg.x, y),
                textStruct.fontFace, textStruct.fontScale,
                Scalar::all(255), textStruct.thickness, 8);
            y += textStruct.lineSpacing;

            if (y + textStruct.lineSpacing > display.rows) {
                break;
            }
        }

        imshow(textStruct.windowName, display);

        int key = waitKey(1);
        handleTurkishKeys(key);
        if (key == 27) { // ESC tuşuna basıldığında döngüden çık
            return;
        }
        else if (key == 13) { //Enter tuşu
            textStruct.lines.push_back(textStruct.currentLine);
            textStruct.currentLine = "";
        }
        else if (key == 8) { //Backspace tuşu
            if (!textStruct.currentLine.empty()) {
                textStruct.currentLine.pop_back();
            }
            else if (!textStruct.lines.empty()) {
                textStruct.currentLine = textStruct.lines.back();
                textStruct.lines.pop_back();
            }
        }
        else if (key >= 32 && key < 127) {
            textStruct.currentLine.push_back((char)key);
        }
        //Satırlar ekranı aşıyorsa en üstteki satırı siler
        if (y + textStruct.lineSpacing > display.rows) {
            textStruct.lines.erase(textStruct.lines.begin());
        }
    }

    void drawTrackbar(drawTrackbarStruct* trackbar) {
        trackbar->windowName = "Draw Properties";
        trackbar->drawDetails = &drawStruct;

        string shape = "Shape Type";
        string pencil = "Pencil Size";
        string eraserSize = "Eraser Size";
        string color = "Color";

        namedWindow(trackbar->windowName, WINDOW_NORMAL);

        createTrackbar(shape, trackbar->windowName, nullptr, trackbar->maxValueShape, drawTrackbarControl, trackbar);
        createTrackbar(pencil, trackbar->windowName, nullptr, trackbar->maxValuePencil, drawTrackbarControl, trackbar);
        createTrackbar(eraserSize, trackbar->windowName, nullptr, trackbar->maxValueEraser, drawTrackbarControl, trackbar);
        createTrackbar(color, trackbar->windowName, nullptr, trackbar->maxValueColor, drawTrackbarControl, trackbar);

        setTrackbarPos(shape, trackbar->windowName, trackbar->currentShape);
        setTrackbarPos(pencil, trackbar->windowName, trackbar->currentPencil);
        setTrackbarPos(eraserSize, trackbar->windowName, trackbar->currentEraser);
        setTrackbarPos(color, trackbar->windowName, trackbar->currentColor);
    }

    void webcamTrackbar(webcamTrackbarStruct* trackbar) {

        namedWindow("Webcam Trackbar", WINDOW_AUTOSIZE);

        createTrackbar("Brightness", "Webcam Trackbar", nullptr, trackbar->maxBrightness, webcamTrackbarControl, trackbar);
        createTrackbar("Contrast", "Webcam Trackbar", nullptr, trackbar->maxContrast, webcamTrackbarControl, trackbar);
        createTrackbar("Width", "Webcam Trackbar", nullptr, trackbar->maxFrameWidth, webcamTrackbarControl, trackbar);
        createTrackbar("Height", "Webcam Trackbar", nullptr, trackbar->maxFrameHeight, webcamTrackbarControl, trackbar);

        setTrackbarPos("Brightness", "Webcam Trackbar", trackbar->brightness);
        setTrackbarPos("Contrast", "Webcam Trackbar", trackbar->contrast);
        setTrackbarPos("Width", "Webcam Trackbar", trackbar->frameWidth);
        setTrackbarPos("Height", "Webcam Trackbar", trackbar->frameHeight);
    }
    void webcam(int ch, webcamTrackbarStruct* trackbar) {
        VideoCapture video(ch, cv::CAP_DSHOW);

        if (!video.open(ch)) {
            cout << "Kanal: " << ch << " Kamera açılamadı!" << endl;
            return;
        }

        if (video.open(ch, cv::CAP_DSHOW)) {
            // cout << "Kamera acildi" << endl;
            namedWindow("Webcam", WINDOW_AUTOSIZE);
            Mat frame;

            while (video.grab()) {

                video.set(CAP_PROP_BRIGHTNESS, trackbar->brightness / 50.0);
                video.set(CAP_PROP_CONTRAST, trackbar->contrast / 50.0);
                video.set(CAP_PROP_FRAME_WIDTH, trackbar->frameWidth);
                video.set(CAP_PROP_FRAME_HEIGHT, trackbar->frameHeight);

                video >> frame;
                if (frame.empty()) break;

                imshow("Webcam", frame);

                if (waitKey(23) == 27) {
                    break;
                }
            }
        }
        destroyAllWindows();
    }
};

class imageProcessing : public WindowProperties {
public:
    char operation;
    Mat image;
    Mat modifiedImage;
    string filePath = samples::findFile("manzara-fotografciligi-1.jpg");

    void readImage() {
        filePath = samples::findFile("manzara-fotografciligi-1.jpg");
        image = imread(filePath, IMREAD_COLOR);

        if (image.empty()) {
            cout << "Could not open or find the image" << std::endl;
            return;
        }
    }

    void imageMenu() {
        cout << "Select the operation you want to perform:\n";
        cout << "1. Crop Image\n";
        cout << "2. Increase Resolution\n";
        cout << "3. Decrease Resolution\n";
        cout << "4. Blur Image\n";
        cout << "5. Morphological Operations\n";
        cout << "Enter your choice: ";
        cin >> operation;

        if (operation == '1') {
            cropImage();
        }
        else if (operation == '2') {
            increaseResolution();
            waitKey();
        }
        else if (operation == '3') {
            decreaseResolution();
        }
        else if (operation == '4') {
            blurImage();  
        }
        else if (operation == '5') {
            while (true) {
                morphologicalOperations();
                cout << "Do you want to perform another morphological operation? (y/n): ";
                char choice;
                cin >> choice;
                if (choice == 'n' || choice == 'N') {
                    break;
                }
            }
            destroyAllWindows();
        }
    }
    void cropImage () {
        readImage();
        Mat croppedImage;
        image.copyTo(croppedImage);
        croppedImage = image(cv::Rect(50, 50, 200, 200));
        imshow("Original Image", image);
        imshow("Cropped Image", croppedImage);
        waitKey();
    }
    void increaseResolution() {
        readImage();
        int enlarging;
        cout << "Enter increasing value: ";
        cin >> enlarging;
        if (enlarging <= 0) {
            cout << "Invalid enlarging value." << endl;
            return;
        }
        int newWidth = image.cols * enlarging;
        int newHeight = image.rows * enlarging;
        Mat increasedImage(Size(newWidth, newHeight), CV_8UC3);
        
        /*The following commented-out code achieves image enlargement by 
        directly replicating each pixel to the specified enlargement factor.
        This method simply scales the image by repeating the original pixels,
        which results in a lower quality image with noticeable pixelation.
        */
        Vec3b bgr;
        for (int i = 0; i < image.cols; i++) {
            for (int j = 0; j < image.rows; j++) {
                bgr = image.at<Vec3b>(Point(i, j));
                int beginX = i * enlarging;
                int beginY = j * enlarging;
                int endX = beginX + enlarging;
                int endY = beginY + enlarging;
                for (int x = beginX; x < endX; x++) {
                    for (int y = beginY; y < endY; y++) {
                        increasedImage.at<Vec3b>(Point(x, y)) = bgr;
                    }
                }
            }
        }
        /*for (int y = 0; y < newHeight; y++) {
            for (int x = 0; x < newWidth; x++) {
                float sourceX = x / static_cast<float>(enlarging);
                float sourceY = y / static_cast<float>(enlarging);

                int x1 = static_cast<int>(sourceX);
                int y1 = static_cast<int>(sourceY);

                int x2 = min(x1 + 1, image.cols - 1);
                int y2 = min(y1 + 1, image.rows - 1);

                float dx = sourceX - x1;
                float dy = sourceY - y1;

                Vec3b pixel1 = image.at<Vec3b>(y1, x1);
                Vec3b pixel2 = image.at<Vec3b>(y1, x2);
                Vec3b pixel3 = image.at<Vec3b>(y2, x1);
                Vec3b pixel4 = image.at<Vec3b>(y2, x2);

                for (int c = 0; c < 3; c++) {
                    float interpolatedValue = (1 - dx) * (1 - dy) * pixel1[c] +
                        dx * (1 - dy) * pixel2[c] +
                        (1 - dx) * dy * pixel3[c] +
                        dx * dy * pixel4[c];
                    increasedImage.at<Vec3b>(y, x)[c] = static_cast<uchar>(interpolatedValue);
                }
            }
        }*/
        imshow("Original Image", image);
        imshow("Increased Resolution Image", increasedImage);
        waitKey();
    }

    void decreaseResolution() {
        readImage();
        float reduction;
        cout << "Enter reducing value: ";
        cin >> reduction;

        if (reduction <= 0) {
            cout << "Invalid reducing value." << endl;
            return;
        }

        int width = image.cols / reduction;
        int height = image.rows / reduction;

        Mat decreasedImage(Size(width, height), CV_8UC3);
        Vec3b bgrVec;

        for (int x = 0; x < width; x++) {
            for (int y = 0; y < height; y++) {
                bgrVec = image.at<Vec3b>(Point(x * reduction, y * reduction));
                decreasedImage.at<Vec3b>(Point(x, y)) = bgrVec;
            }
        }
        imshow("Original Image", image);
        imshow("Decreased Resolution Image", decreasedImage);
        waitKey();
    }
    void blurImage() {
        readImage();
        int sideValue;
        cout << "Enter blurring value (odd number): ";
        cin >> sideValue;
        Mat bluringPicture = Mat::zeros(Size(image.cols, image.rows), CV_8UC3);

        for (int x = 0; x < image.cols; x++) {
            for (int y = 0; y < image.rows; y++) {
                int blue = 0, green = 0, red = 0;
                int count = 0;

                for (int sideBeginX = x - sideValue; sideBeginX < x + sideValue; sideBeginX++) {
                    for (int sideBeginY = y - sideValue; sideBeginY < y + sideValue; sideBeginY++) {
                        if (sideBeginX >= 0 && sideBeginY >= 0 && sideBeginX < image.cols && sideBeginY < image.rows) {
                            
                            blue += image.at<Vec3b>(Point(sideBeginX, sideBeginY))[0];
                            green += image.at<Vec3b>(Point(sideBeginX, sideBeginY))[1];
                            red += image.at<Vec3b>(Point(sideBeginX, sideBeginY))[2];

                            count++;
                        }
                    }
                }
                blue = blue / count;
                green = green / count;
                red = red / count;

                bluringPicture.at<Vec3b>(Point(x, y))[0] = blue;
                bluringPicture.at<Vec3b>(Point(x, y))[1] = green;
                bluringPicture.at<Vec3b>(Point(x, y))[2] = red;
            }
        }
        imshow("Original Image", image);
        imshow("Bluring Image", bluringPicture);
        waitKey();
    }
    void morphologicalOperations() {
        readImage();
        int morpType;
        cout << "Enter morphological type (1: ERODE, 2: DILATE, 3: OPEN, 4: CLOSE, 5: GRADIENT, 6: TOPHAT, 7: BLACKHAT): ";
        cin >> morpType;
        image.copyTo(modifiedImage);
        Mat targetPicture;
        Mat kernel = getStructuringElement(MORPH_RECT, Size(3, 3));
        if (morpType == 1)
            morphologyEx(modifiedImage, targetPicture, MORPH_ERODE, kernel, cv::Point(-1, -1), 2);
        else if (morpType == 2)
            morphologyEx(modifiedImage, targetPicture, MORPH_DILATE, kernel, cv::Point(-1, -1), 2);
        else if (morpType == 3)
            morphologyEx(modifiedImage, targetPicture, MORPH_OPEN, kernel, cv::Point(-1, -1), 2);
        else if (morpType == 4)
            morphologyEx(modifiedImage, targetPicture, MORPH_CLOSE, kernel, cv::Point(-1, -1), 2);
        else if (morpType == 5)
            morphologyEx(modifiedImage, targetPicture, MORPH_GRADIENT, kernel, cv::Point(-1, -1), 2);
        else if (morpType == 6)
            morphologyEx(modifiedImage, targetPicture, MORPH_TOPHAT, kernel, cv::Point(-1, -1), 2);
        else if (morpType == 7)
            morphologyEx(modifiedImage, targetPicture, MORPH_BLACKHAT, kernel, cv::Point(-1, -1), 2);
        imshow("Original Image", image);
        imshow("Morphological Operations", targetPicture);
        waitKey();
    }
};

int main()
{
    char choice;
    WindowProperties windowProperties;
    drawTrackbarStruct drawtrackbarStruct;
    webcamTrackbarStruct webcamTrackbarStruct;
    
    do {
        windowProperties.printMenu();
        cout << "Enter your choice: ";
        cin >> choice;
        if (choice == '1') {
                windowProperties.createWindow();                
                windowProperties.setMouseCallback();
                windowProperties.drawTrackbar(&drawtrackbarStruct);
                waitKey();                
                destroyAllWindows();
        }
        else if (choice == '2') {
            windowProperties.createWindow();
            while (true) {
                windowProperties.writeText();
                if (waitKey(1) == 27) break;
            }          
            destroyAllWindows();
        }
        else if (choice == '3') {
            imageProcessing processor;
            do {
                processor.imageMenu();
                cout << "Do you want to perform another image operation? (y/n): ";
                char choice;
                cin >> choice;
                if (choice == 'n' || choice == 'N') {
                    break;
                }
            } while (true);
            destroyAllWindows();
        }
        else if (choice == '4') {
            windowProperties.webcamTrackbar(&webcamTrackbarStruct);
            windowProperties.webcam(0, &webcamTrackbarStruct);
            waitKey();
            destroyAllWindows();
        }
        else if (choice == '5') {
            cout << "Exiting the PaintArt App." << endl;
        }
        else {
            cout << "Invalid choice. Please try again." << endl;
        }
    } while (choice != '5');
    

    return 0;
}
