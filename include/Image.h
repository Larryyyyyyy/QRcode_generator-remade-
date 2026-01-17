#ifndef IMAGE_H
#define IMAGE_H
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>
using namespace std;
class imageHandler {
private:
    int width, height, channels;             // 图片参数
    unsigned char *img;                      // 图片每个像素亮度的指针
public:
    imageHandler(const char* path);
    ~imageHandler();
    void loadImage(const char* path);                                 // 读取图像, 这一步依赖外部库
    vector<uint8_t> adaptiveThresholding();                           // 自适应二值化, 每个像素都处理为黑(0)或白(1)
    vector<int> getGrayImg();                                         // 得到灰度图
    vector<vector<int>> getMiddleBrightness(const vector<int>& gray); // 生成一张亮度趋势图
    vector<vector<uint32_t>> pixels;
};
struct center {
    int cnt;
    double x, y, estimatedModuleSize;
    center(double x, double y, double estimatedModuleSize) {
        this->x = x;
        this->y = y;
        this->estimatedModuleSize = estimatedModuleSize;
        this->cnt = 1;
    }
    bool equal(double moduleSize, double i, double j) {
        if (fabs(i - this->y) <= moduleSize && fabs(j - this->x) <= moduleSize) {
            auto moduleSizeDiff = fabs(moduleSize - this->estimatedModuleSize);
            return moduleSizeDiff <= 1.0 || moduleSizeDiff / this->estimatedModuleSize <= 1.0;
        }
        return false;
    }
};
class patternFinder {
private:
    int width, height;
    vector<uint8_t> bitmap;
    vector<center> possibleCenters;
    bool hasSkipped;
public:
    patternFinder(int width, int height, vector<uint8_t> bitmap);
    ~patternFinder();
    bool foundPatternCross(const vector<int>& state);
    bool handlePossibleCenter(const vector<int>& state, const int & i, const int & j);
    double crossCheckVertical(const int& startI, const int& centerJ, const int& maxCount, const int& tot);
    double crossCheckHorizontal(const int& startJ,  const int& centerI, const int& maxCount, const int& tot);
    bool haveMultiplyConfirmedCenters();
    int findRowSkip();
    vector<center> selectBestPatterns();
    void orderBestPatterns();
    vector<center> patterns;
};
class alignmentPatternFinder {
private:
    int width, height;
    int alignWidth, alignHeight;
    vector<uint8_t> bitmap;
    vector<center> possibleCenters;
	int startX, startY;
	double moduleSize;
public:
    alignmentPatternFinder(int alignWidth, int alignHeight, int width, int height, int startX, int startY, double moduleSize, vector<uint8_t> bitmap);
    ~alignmentPatternFinder();
    bool foundPatternCross(const vector<int>& state);
    center* handlePossibleCenter(const vector<int>& state, const int & i, const int & j);
    double crossCheckVertical(const int& startI, const int& centerJ, const int& maxCount, const int& tot);
    center* pattern;
};
struct perspectiveTransform {
    double a11, a21, a31, a12, a22, a32, a13, a23, a33;
    perspectiveTransform(double _11, double _21, double _31, double _12, double _22, double _32, double _13, double _23, double _33) {
        a11 = _11, a21 = _21, a31 = _31, a12 = _12, a22 = _22, a32 = _32, a13 = _13, a23 = _23, a33 = _33;
    }
    void transformPoints(vector<double>& points) {
		for (int i = 0; i < points.size(); i += 2) {
            double x = points[i], y = points[i + 1];
            double denominator = a13 * points[i] + a23 * points[i + 1] + a33;
            points[i] = (a11 * x + a21 * y + a31) / denominator;
            points[i + 1] = (a12 * x + a22 * y + a32) / denominator;
		}
	}
    perspectiveTransform buildAdjoint() {
		return perspectiveTransform(a22 * a33 - a23 * a32, a23 * a31 - a21 * a33, a21 * a32 - a22 * a31, a13 * a32 - a12 * a33, a11 * a33 - a13 * a31, a12 * a31 - a11 * a32, a12 * a23 - a13 * a22, a13 * a21 - a11 * a23, a11 * a22 - a12 * a21);
	}
	perspectiveTransform times(perspectiveTransform other) {
		return perspectiveTransform(a11 * other.a11 + a21 * other.a12 + a31 * other.a13, a11 * other.a21 + a21 * other.a22 + a31 * other.a23, a11 * other.a31 + a21 * other.a32 + a31 * other.a33, a12 * other.a11 + a22 * other.a12 + a32 * other.a13, a12 * other.a21 + a22 * other.a22 + a32 * other.a23, a12 * other.a31 + a22 * other.a32 + a32 * other.a33, a13 * other.a11 + a23 * other.a12 + a33 * other.a13, a13 * other.a21 + a23 * other.a22 + a33 * other.a23, a13 * other.a31 + a23 * other.a32 + a33 * other.a33);
	}
};
class detector {
private:
    vector<uint8_t> bitmap;
    int width, height;
public:
    detector(vector<center> patterns, int width, int height, vector<uint8_t> bitmap);
    ~detector();
    double sizeOfBlackWhiteBlackRun(int fromX, int fromY, int toX, int toY);
    double sizeOfBlackWhiteBlackRunBothWays(int fromX, int fromY, int toX, int toY);
    double calculateModuleSizeOneWay(center x, center y);
    perspectiveTransform squareToQuadrilateral(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3);
    perspectiveTransform quadrilateralToSquare(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3);
    perspectiveTransform quadrilateralToQuadrilateral(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double x0p, double y0p, double x1p, double y1p, double x2p, double y2p, double x3p, double y3p);
    perspectiveTransform createTransform(center topLeft, center topRight, center  bottomLeft, center* alignmentPattern, int dimension);
    void sampleGrid(vector<uint8_t> bitmap, perspectiveTransform transform, int dimension);
    void checkAndNudgePoints(vector<double>& points);
    vector<vector<uint32_t>> bits;
};
#endif // !IMAGE_H