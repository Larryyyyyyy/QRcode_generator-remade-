#include "../include/Image.h"
#include "../include/Stb_image.h"
#define MAX_MODULES 57
#define MIN_SKIP 3
#define INTEGER_MATH_SHIFT 8
#define CENTER_QUORUM 2
imageHandler::imageHandler(const char* path) {
    loadImage(path);
    auto bitmap = adaptiveThresholding();
    auto *pF = new patternFinder(width, height, bitmap);
    auto patterns = vector<center>(pF->patterns);
    delete pF;
    auto *dR = new detector(patterns, width, height, bitmap);
    pixels = vector<vector<uint32_t>>(dR->bits);
    delete dR;
}
imageHandler::~imageHandler() {
    delete img;
}
void imageHandler::loadImage(const char* path) {
    img = stbi_load(path, &width, &height, &channels, 3);
    if (img == NULL) throw "Unable to load image.";
}
vector<uint8_t> imageHandler::adaptiveThresholding() {
    auto gray = getGrayImg();
    auto middle = getMiddleBrightness(gray);
    int sqrtNumArea = middle.size(), areaWidth = width / sqrtNumArea, areaHeight = height / sqrtNumArea;
    vector<uint8_t> bitmap;
    bitmap.resize(width * height);
    for (int ay = 0; ay < sqrtNumArea; ay++) {
        for (int ax = 0; ax < sqrtNumArea; ax++) {
            for (int dy = 0; dy < areaHeight; dy++) {
                for (int dx = 0; dx < areaWidth; dx++) {
                    bitmap[areaWidth * ax + dx + (areaHeight * ay + dy) * width] = 
                    (gray[areaWidth * ax + dx + (areaHeight * ay + dy) * width] < middle[ax][ay]) ? 1 : 0;
                }
            }
        }
    }
    return bitmap;
}
vector<int> imageHandler::getGrayImg() {
    vector<int> gray;
    for(int i = 0; i < height; ++i) {
        for(int j = 0; j < width; ++j) {
            int pos = (i * width + j) * 3;
            int Gray = (img[pos] * 33 + img[pos + 1] * 34 + img[pos + 2] * 33) / 100;
            // 以后可以考虑 Gray = 0.299R + 0.587G + 0.114B
            gray.push_back(Gray);
        }
    }
    return gray;
}
vector<vector<int>> imageHandler::getMiddleBrightness(const vector<int>& gray) {
    const int numSqrtArea = 4;
    int areaWidth = width / numSqrtArea;
    int areaHeight = height / numSqrtArea;
    vector<vector<int>> middle;
    middle.assign(numSqrtArea, vector<int>(numSqrtArea, 0));
    for(int i = 0; i < numSqrtArea; ++i) {
        for(int j = 0; j < numSqrtArea; ++j) {
            int mi = 0xff, ma = 0;
            for(int di = 0; di < areaHeight; ++di) {
                for(int dj = 0; dj < areaWidth; ++dj) {
                    int target = gray[areaWidth * j + dj + (areaHeight * i + di) * width];
                    mi = min(mi, target);
                    ma = max(ma, target);
                }
            }
            middle[j][i] = (mi + ma) / 2;
        }
    }
    return middle;
}

patternFinder::patternFinder(int width, int height, vector<uint8_t> bitmap) {
    this->width = width;
    this->height = height;
    this->bitmap = bitmap;
    this->hasSkipped = 0;
    int iSkip = (3 * height) / (4 * MAX_MODULES);
    if (iSkip < MIN_SKIP) iSkip = MIN_SKIP;
    bool done = false;
	vector<int> state(5, 0);
    for (int i = iSkip - 1; i < height && !done; i += iSkip) {
        state[0] = state[1] = state[2] = state[3] = state[4] = 0;
        int current = 0;
        for (int j = 0; j < width; ++j) {
            if(bitmap[j + i * width]) {
                // Black pixel
                if (current & 1) {
                    // Counting white pixels
                    ++current;
                }
                ++state[current];
            }
            else {
                // White pixel
                if (!(current & 1)) {
                    // Counting black pixels
                    if (current == 4) {
                        // A winner?
                        if (foundPatternCross(state)) {
                            // Yes
                            auto confirmed = handlePossibleCenter(state, i, j);
                            if (confirmed) {
                                // Start examining every other line. Checking each line turned out to be too
                                // expensive and didn't improve performance.
                                iSkip = 2;
                                if (hasSkipped) {
                                    done = haveMultiplyConfirmedCenters();
                                }
                                else {
                                    auto rowSkip = findRowSkip();
                                    if (rowSkip > state[2]) {
                                        // Skip rows between row of lower confirmed center
                                        // and top of presumed third confirmed center
                                        // but back up a bit to get a full chance of detecting
                                        // it, entire width of center of finder pattern
                                        // Skip by rowSkip, but back off by stateCount[2] (size of last center
                                        // of pattern we saw) to be conservative, and also back off by iSkip which
                                        // is about to be re-added
                                        i += rowSkip - state[2] - iSkip;
                                        j = width - 1;
                                    }
                                }
                            }
                            else {
                                // Advance to next black pixel
                                do {
                                    ++j;
                                }
                                while (j < width && !bitmap[j + i * width]);
                                --j;
                                // back up to that last white pixel
                            }
                            // Clear state to start looking again
                            current = 0;
                            state[0] = state[1] = state[2] = state[3] = state[4] = 0;
                        }
                        else {
                            // No, shift counts back by two
                            state[0] = state[2];
                            state[1] = state[3];
                            state[2] = state[4];
                            state[3] = 1;
                            state[4] = 0;
                            current = 3;
                        }
                    }
                    else {
                        state[++current]++;
                    }
                }
                else {
                    // Counting white pixels
                    ++state[current];
                }
            }
        }
        if(foundPatternCross(state)) {
            auto confirmed = handlePossibleCenter(state, i, width);
            if (confirmed) {
                iSkip = state[0];
                if (hasSkipped) {
                    // Found a third one
                    done = haveMultiplyConfirmedCenters();
                }
            }
        }
    }
    patterns = selectBestPatterns();
    orderBestPatterns(); 
}
patternFinder::~patternFinder() {}
bool patternFinder::foundPatternCross(const vector<int>& state) {
    int tot = 0;
    for (int i = 0; i < 5; ++i) {
        if (!state[i]) return false;
        tot += state[i];
    }
    if (tot < 7) return false;
    int moduleSize = (tot << INTEGER_MATH_SHIFT) / 7;
    int maxVariance = moduleSize / 2;
	return abs(moduleSize - (state[0] << INTEGER_MATH_SHIFT)) < maxVariance && abs(moduleSize - (state[1] << INTEGER_MATH_SHIFT)) < maxVariance && abs(3 * moduleSize - (state[2] << INTEGER_MATH_SHIFT)) < 3 * maxVariance && abs(moduleSize - (state[3] << INTEGER_MATH_SHIFT)) < maxVariance && abs(moduleSize - (state[4] << INTEGER_MATH_SHIFT)) < maxVariance;
}
bool patternFinder::handlePossibleCenter(const vector<int>& state, const int & i, const int & j) {
    int tot = state[0] + state[1] + state[2] + state[3] + state[4];
    double centerJ = (double)(j - state[4] - state[3]) - (double)state[2] / 2.0;
    double centerI = crossCheckVertical(i, centerJ, state[2], tot);
    if (isnan(centerI)) return false;
    centerJ = crossCheckHorizontal(centerJ, centerI, state[2], tot);
    if (isnan(centerJ)) return false;
    double estimatedModuleSize = (double)tot / 7.0;
	bool found = false;
    for (auto &p : possibleCenters) {
        if (p.equal(estimatedModuleSize, centerI, centerJ)) {
            ++p.cnt;
            found = true;
            break;
        }
    }
    if (!found) {
        possibleCenters.push_back(center(centerJ, centerI, estimatedModuleSize));
    }
    return true;
}
double patternFinder::crossCheckVertical(const int& startI, const int& centerJ, const int& maxCount, const int& tot) {
    int i = startI;
    vector<int> cstate(5, 0);
    while (i >= 0 && bitmap[centerJ + i * width]) {
		++cstate[2];
		--i;
	}
    if (i < 0) return NAN;
    while (i >= 0 && !bitmap[centerJ + i * width] && cstate[1] <= maxCount) {
        ++cstate[1];
		--i;
	}
    if (i < 0 || cstate[1] > maxCount) return NAN;
    while (i >= 0 && bitmap[centerJ + i * width] && cstate[0] <= maxCount) {
        ++cstate[0];
		--i;
	}
	if (cstate[0] > maxCount) return NAN;
	// Now also count down from center
	i = startI + 1;
	while (i < height && bitmap[centerJ + i * width]) {
		++cstate[2];
		++i;
	}
	if (i == height) return NAN;
	while (i < height && !bitmap[centerJ + i * width] && cstate[3] < maxCount) {
        ++cstate[3];
        ++i;
	}
	if (i == height || cstate[3] >= maxCount) return NAN;
	while (i < height && bitmap[centerJ + i * width] && cstate[4] < maxCount) {
		++cstate[4];
		++i;
	}
	if (cstate[4] >= maxCount) return NAN;
    // If we found a finder-pattern-like section, but its size is more than 40% different than
    // the original, assume it's a false positive
    int tot_ = cstate[0] + cstate[1] + cstate[2] + cstate[3] + cstate[4];
    if (5 * abs(tot_ - tot) >= 2 * tot) return NAN;
    return foundPatternCross(cstate) ? (double)(i - cstate[4] - cstate[3]) - (double)cstate[2] / 2.0 : NAN;
}
double patternFinder::crossCheckHorizontal(const int& startJ,  const int& centerI, const int& maxCount, const int& tot) {
    int j = startJ;
    vector<int> cstate(5, 0);
    while (j >= 0 && bitmap[j + centerI * width]) {
        ++cstate[2];
        --j;
    }
    if (j < 0) return NAN;
	while (j >= 0 && !bitmap[j + centerI * width] && cstate[1] <= maxCount) {
		++cstate[1];
		--j;
	}
	if (j < 0 || cstate[1] > maxCount) return NAN;
    while (j >= 0 && bitmap[j + centerI * width] && cstate[0] <= maxCount) {
        ++cstate[0];
        --j;
    }
	if (cstate[0] > maxCount) return NAN;
	j = startJ + 1;
	while (j < width && bitmap[j + centerI * width]) {
		++cstate[2];
		++j;
	}
	if (j == width) return NAN;
	while (j < width && !bitmap[j + centerI * width] && cstate[3] < maxCount) {
        ++cstate[3];
		++j;
	}
    if (j == width || cstate[3] >= maxCount) return NAN;
    while (j < width && bitmap[j + centerI * width] && cstate[4] < maxCount) {
        ++cstate[4];
        ++j;
    }
	if (cstate[4] >= maxCount) return NAN;
    // If we found a finder-pattern-like section, but its size is significantly different than
    // the original, assume it's a false positive
	int tot_ = cstate[0] + cstate[1] + cstate[2] + cstate[3] + cstate[4];
	if (5 * abs(tot_ - tot) >= tot) return NAN;
    return foundPatternCross(cstate) ? (double)(j - cstate[4] - cstate[3]) - (double)cstate[2] / 2.0 : NAN;
}
bool patternFinder::haveMultiplyConfirmedCenters() {
    int confirmedCount = 0;
	double totalModuleSize = 0.0;
	for (auto p : possibleCenters) {
		if (p.cnt >= CENTER_QUORUM) {
			++confirmedCount;
			totalModuleSize += p.estimatedModuleSize;
		}
	}
	if (confirmedCount < 3) return false;
    // OK, we have at least 3 confirmed centers, but, it's possible that one is a "false positive"
    // and that we need to keep looking. We detect this by asking if the estimated module sizes
    // vary too much. We arbitrarily say that when the total deviation from average exceeds
    // 5% of the total module size estimates, it's too much.
    double average = totalModuleSize / possibleCenters.size();
	double totalDeviation = 0.0;
	for (auto p : possibleCenters) {
		totalDeviation += abs(p.estimatedModuleSize - average);
	}
	return totalDeviation <= 0.05 * totalModuleSize;
}
int patternFinder::findRowSkip() {
    if (possibleCenters.size() <= 1) return 0;
	center* firstConfirmedCenter = nullptr;
	for (auto &p : possibleCenters) {
        if (p.cnt >= CENTER_QUORUM) {
            if (firstConfirmedCenter == nullptr) {
                firstConfirmedCenter = &p;
            }
            else {
                // We have two confirmed centers
                // How far down can we skip before resuming looking for the next
                // pattern? In the worst case, only the difference between the
                // difference in the x / y coordinates of the two centers.
                // This is the case where you find top left last.
                hasSkipped = true;
                return (abs(firstConfirmedCenter->x - p.x) - abs(firstConfirmedCenter->y - p.y)) / 2;
            }
        }
    }
    return 0;
}
vector<center> patternFinder::selectBestPatterns() {
    vector<center> _;
    if (possibleCenters.size() < 3) {
        puts("-1");
        throw "Couldn't find enough finder patterns";
    }
	// Filter outlier possibilities whose module size is too different
	if (possibleCenters.size() > 3) {
		// But we can only afford to do so if we have at least 4 possibilities to choose from
		double totalModuleSize = 0.0, square = 0.0;
		for (auto p : possibleCenters) {
            totalModuleSize += p.estimatedModuleSize;
            square += (p.estimatedModuleSize * p.estimatedModuleSize);
		}
		double average = totalModuleSize / (double)possibleCenters.size();
        sort(possibleCenters.begin(), possibleCenters.end(), [&](center a, center b) {
            return abs(a.estimatedModuleSize - average) < abs(b.estimatedModuleSize - average);
        });
		double stdDev = sqrt(square / possibleCenters.size() - average * average);
		double limit = max(0.2 * average, stdDev);
		for (auto p : possibleCenters) {
			if (abs(p.estimatedModuleSize - average) <= limit) {
				_.push_back(p);
			}
		}
    }
    if (possibleCenters.size() == 3) {
        _ = possibleCenters;
    }
	if (_.size() > 3) {
		// Throw away all but those first size candidate points we found.
        sort(_.begin(), _.end(), [](center a, center b) {
            return a.cnt < b.cnt;
        });
    }
	return {_[0], _[1], _[2]};
}
void patternFinder::orderBestPatterns() {
    double zeroOneDistance = sqrt((patterns[0].x - patterns[1].x) * (patterns[0].x - patterns[1].x) +
                                  (patterns[0].y - patterns[1].y) * (patterns[0].y - patterns[1].y));
    double oneTwoDistance = sqrt((patterns[1].x - patterns[2].x) * (patterns[1].x - patterns[2].x) +
                                 (patterns[1].y - patterns[2].y) * (patterns[1].y - patterns[2].y));
    double zeroTwoDistance = sqrt((patterns[0].x - patterns[2].x) * (patterns[0].x - patterns[2].x) +
                                  (patterns[0].y - patterns[2].y) * (patterns[0].y - patterns[2].y));
    center *p1 = nullptr, *p2 = nullptr, *p3 = nullptr;
    if (oneTwoDistance >= zeroOneDistance && oneTwoDistance >= zeroTwoDistance) {
		p2 = &patterns[0];
		p1 = &patterns[1];
		p3 = &patterns[2];
	}
	else if (zeroTwoDistance >= oneTwoDistance && zeroTwoDistance >= zeroOneDistance) {
		p2 = &patterns[1];
		p1 = &patterns[0];
		p3 = &patterns[2];
    }
	else {
		p2 = &patterns[2];
		p1 = &patterns[0];
		p3 = &patterns[1];
	}
    // Use cross product to figure out whether A and C are correct or flipped.
    // This asks whether BC x BA has a positive z component, which is the arrangement
    // we want for A, B, C. If it's negative, then we've got it flipped around and
    // should swap A and C.
    // <summary> Returns the z component of the cross product between vectors BC and BA.</summary>
    if (((p3->x - p2->x) * (p1->y - p2->y)) - ((p3->y - p2->y) * (p1->x - p2->x)) < 0.0) {
        swap(*p1, *p3);
    }
    patterns = {*p1, *p2, *p3};
}

alignmentPatternFinder::alignmentPatternFinder(int alignWidth, int alignHeight, int width, int height, int startX, int startY, double moduleSize, vector<uint8_t> bitmap) {
    this->alignWidth = alignWidth;
    this->alignHeight = alignHeight;
    this->width = width;
    this->height = height;
    this->startX = startX;
    this->startY = startY;
    this->moduleSize =moduleSize;
    this->bitmap = bitmap;
    this->pattern = nullptr;
    // We are looking for black/white/black modules in 1:1:1 ratio;
    // this tracks the number of black/white/black modules seen so far
    vector<int> state(3, 0);
    for (int iGen = 0; iGen < height; ++iGen) {
        // Search from middle outwards
        int i = startY + (alignHeight >> 1) + ((iGen & 0x01) == 0 ? ((iGen + 1) >> 1) : -((iGen + 1) >> 1));
		state[0] = state[1] = state[2] = 0;
        int j = startX;
        // Burn off leading white pixels before anything else; if we start in the middle of
        // a white run, it doesn't make sense to count its length, since we don't know if the
        // white run continued to the left of the start point
        while (j < startX + alignWidth && !bitmap[j + width * i]) ++j;
		int current = 0;
		while (j < startX + alignWidth) {
			if (bitmap[j + i * width]) {
                // Black pixel
                if (current == 1) {
                    // Counting black pixels
                    ++state[current];
                }
				else {
                    // Counting white pixels
                    if (current == 2) {
                        // A winner?
                        if (foundPatternCross(state)) {
                            // Yes
                            pattern = handlePossibleCenter(state, i, j);
                            if (pattern != nullptr) {
                                return;
                            }
                        }
						state[0] = state[2];
						state[1] = 1;
						state[2] = 0;
						current = 1;
                    }
                    else {
						state[++current]++;
					}
				}
			}
			else {
                // White pixel
                if (current == 1) {
                    // Counting black pixels
                    ++current;
                }
                ++state[current];
            }
            ++j;
        }
        if (foundPatternCross(state)) {
            pattern = handlePossibleCenter(state, i, startX + width);
            if (pattern != nullptr) return;
        }
    }
    // Hmm, nothing we saw was observed and confirmed twice. If we had
    // any guess at all, return it.
    if (!possibleCenters.empty()) {
        pattern = new center(possibleCenters[0]);
        return;
    }
    puts("0");
    throw "Couldn't find enough alignment patterns";
}
alignmentPatternFinder::~alignmentPatternFinder() {}
bool alignmentPatternFinder::foundPatternCross(const vector<int>& state) {
    for (int i = 0; i < 3; ++i) {
        if (fabs(moduleSize - state[i]) >= moduleSize / 2.0) {
            return false;
        }
    }
    return true;
}
center* alignmentPatternFinder::handlePossibleCenter(const vector<int>& state, const int & i, const int & j) {
    int tot = state[0] + state[1] + state[2];
    double centerJ = (double)(j - state[2]) - (double)state[1] / 2.0;
    double centerI = crossCheckVertical(i, centerJ, 2 * state[1], tot);
    if (isnan(centerI)) return nullptr;
    double estimatedModuleSize = (double)tot / 3.0;
    for (auto p : possibleCenters) {
        if (p.equal(estimatedModuleSize, centerI, centerJ)) {
            return new center(centerJ, centerI, estimatedModuleSize);
        }
    }
    // Hadn't found this before; save it
    possibleCenters.push_back(center(centerJ, centerI, estimatedModuleSize));
    return nullptr;
}
double alignmentPatternFinder::crossCheckVertical(const int& startI, const int& centerJ, const int& maxCount, const int& tot) {
    vector<int> cstate(3, 0);
    int i = startI;
    // Start counting up from center
    while (i >= 0 && bitmap[centerJ + i * width] && cstate[1] <= maxCount) {
		++cstate[1];
		--i;
	}
    // If already too many modules in this state or ran off the edge:
    if (i < 0 || cstate[1] > maxCount) return NAN;
    while (i >= 0 && !bitmap[centerJ + i * width] && cstate[0] <= maxCount) {
        ++cstate[0];
		--i;
	}
    if (cstate[0] > maxCount) return NAN;
    // Now also count down from center
	i = startI + 1;
	while (i < height && bitmap[centerJ + i * width] && cstate[1] <= maxCount) {
		++cstate[1];
		++i;
	}
	if (i == height || cstate[1] > maxCount) return NAN;
	while (i < height && !bitmap[centerJ + i * width] && cstate[2] <= maxCount) {
        ++cstate[2];
        ++i;
	}
	if (cstate[2] > maxCount) return NAN;
    int tot_ = cstate[0] + cstate[1] + cstate[2];
    if (5 * abs(tot_ - tot) >= 2 * tot) return NAN;
    return foundPatternCross(cstate) ? (double)(i - cstate[2]) - (double)cstate[1] / 2.0 : NAN;
}
detector::detector(vector<center> patterns, int width, int height, vector<uint8_t> bitmap) {
    this->bitmap = bitmap;
    this->width = width;
    this->height = height;
	auto bottomLeft = patterns[0];
    auto topLeft = patterns[1];
	auto topRight = patterns[2];
	auto moduleSize = (calculateModuleSizeOneWay(topLeft, topRight) + calculateModuleSizeOneWay(topLeft, bottomLeft)) / 2.0;
    if (moduleSize < 1.0) {
        puts("1");
        throw "Error";
    }
    int tltrCentersDimension = round(sqrt((topLeft.x - topRight.x) * (topLeft.x - topRight.x) + (topLeft.y - topRight.y) * (topLeft.y - topRight.y)) / moduleSize);
    int tlblCentersDimension = round(sqrt((topLeft.x - bottomLeft.x) * (topLeft.x - bottomLeft.x) + (topLeft.y - bottomLeft.y) * (topLeft.y - bottomLeft.y)) / moduleSize);
	int dimension = ((tltrCentersDimension + tlblCentersDimension) >> 1) + 7;
	if ((dimension & 0x03) == 3) {
        puts("2");
        throw "dimension error";
    }
    if ((dimension & 0x03) == 0 || (dimension & 0x03) == 2) dimension = 1;
    int version = (dimension - 17) >> 2;
    int modulesBetweenFPCenters = 4 * version + 10;
    center* alignmentPattern = nullptr;
    auto __ = center(0, 0, 0);
	// var alignmentPattern = null;
	// Anything above version 1 has an alignment pattern
	if (version > 1) {
        // Guess where a "bottom right" finder pattern would have been
        double bottomRightX = topRight.x - topLeft.x + bottomLeft.x;
        double bottomRightY = topRight.y - topLeft.y + bottomLeft.y;
        // Estimate that alignment pattern is closer by 3 modules
        // from "bottom right" to known top left location
		double correctionToTopLeft = 1.0 - 3.0 /  modulesBetweenFPCenters;
		int estAlignmentX = topLeft.x + correctionToTopLeft * (bottomRightX - topLeft.x);
		int estAlignmentY = topLeft.y + correctionToTopLeft * (bottomRightY - topLeft.y);
		// Kind of arbitrary -- expand search radius before giving up
        int allowance = floor (4 * moduleSize);
		int alignmentAreaLeftX = max(0, estAlignmentX - allowance);
		int alignmentAreaRightX = min(width - 1, estAlignmentX + allowance);
		if (alignmentAreaRightX - alignmentAreaLeftX < moduleSize * 3) {
            puts("3");
            throw "Error";
        }
		int alignmentAreaTopY = max(0, estAlignmentY - allowance);
		int alignmentAreaBottomY = min(height - 1, estAlignmentY + allowance);
		auto _ = new alignmentPatternFinder(alignmentAreaRightX - alignmentAreaLeftX, alignmentAreaBottomY - alignmentAreaTopY, width, height, alignmentAreaLeftX, alignmentAreaTopY, moduleSize, bitmap);
        __ = center(*(_->pattern));
        delete _;
		// If we didn't find alignment pattern... well try anyway without it
	}
    alignmentPattern = &__;
	auto transform = createTransform(topLeft, topRight, bottomLeft, alignmentPattern, dimension);
	sampleGrid(bitmap, transform, dimension);
}
detector::~detector() {}
double detector::sizeOfBlackWhiteBlackRun(int fromX, int fromY, int toX, int toY) {
    bool steep = abs(toY - fromY) > abs(toX - fromX);
	if (steep) {
		swap(fromX, fromY);
        swap(toX, toY);
	}
	int dx = abs(toX - fromX), dy = abs(toY - fromY);
	int error = - dx >> 1;
	int ystep = fromY < toY ? 1 : -1, xstep = fromX < toX ? 1 : -1;
	int state = 0; // In black pixels, looking for white, first or second time
	for (int x = fromX, y = fromY; x != toX; x += xstep) {
		int realX = steep ? y : x, realY = steep ? x : y;
		if (state == 1) {
            // In white pixels, looking for black
			if (bitmap[realX + realY * width]) {
				++state;
			}
		}
		else {
			if (!bitmap[realX + realY * width]) {
				++state;
			}
		}
		if (state == 3) {
            // Found black, white, black, and stumbled back onto white; done
			int diffX = x - fromX;
			int diffY = y - fromY;
			return  sqrt((diffX * diffX + diffY * diffY));
		}
		error += dy;
		if (error > 0) {
			if (y == toY) break;
			y += ystep;
			error -= dx;
		}
	}
	int diffX2 = toX - fromX;
	int diffY2 = toY - fromY;
	return  sqrt((diffX2 * diffX2 + diffY2 * diffY2));
}
double detector::sizeOfBlackWhiteBlackRunBothWays(int fromX, int fromY, int toX, int toY) {
    double res = sizeOfBlackWhiteBlackRun(fromX, fromY, toX, toY);
	// Now count other way -- don't run off image though of course
	double scale = 1.0;
	int otherToX = fromX - (toX - fromX);
	if (otherToX < 0) {
		scale =  fromX / (fromX - otherToX);
		otherToX = 0;
	}
	else if (otherToX >= width) {
		scale = (width - 1 - fromX) / (otherToX - fromX);
		otherToX = width - 1;
	}
	int otherToY = fromY - (toY - fromY) * scale;
	scale = 1.0;
	if (otherToY < 0) {
		scale = fromY / (fromY - otherToY);
		otherToY = 0;
	}
	else if (otherToY >= height) {
		scale = (height - 1 - fromY) / (otherToY - fromY);
		otherToY = height - 1;
	}
	otherToX = fromX + (otherToX - fromX) * scale;
	res += sizeOfBlackWhiteBlackRun(fromX, fromY, otherToX, otherToY);
	return res - 1.0; // -1 because we counted the middle pixel twice
}
double detector::calculateModuleSizeOneWay(center x, center y) {
    auto moduleSizeEst1 = sizeOfBlackWhiteBlackRunBothWays(x.x, x.y, y.x, y.y);
    auto moduleSizeEst2 = sizeOfBlackWhiteBlackRunBothWays(y.x, y.y, x.x, x.y);
	if (isnan(moduleSizeEst1)) {
        return moduleSizeEst2 / 7.0;
    }
	if (isnan(moduleSizeEst2)) {
        return moduleSizeEst1 / 7.0;
    }
    // Average them, and divide by 7 since we've counted the width of 3 black modules,
    // and 1 white and 1 black module on either side. Ergo, divide sum by 14.
    return (moduleSizeEst1 + moduleSizeEst2) / 14.0;
}
perspectiveTransform detector::squareToQuadrilateral(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3) {
    double dy2 = y3 - y2, dy3 = y0 - y1 + y2 - y3;
	if (fabs(dy2 - 0.0) < 1e-6 && fabs(dy3 - 0.0) < 1e-6) {
        return perspectiveTransform(x1 - x0, x2 - x1, x0, y1 - y0, y2 - y1, y0, 0.0, 0.0, 1.0);
    }
	else {
		double dx1 = x1 - x2, dx2 = x3 - x2, dx3 = x0 - x1 + x2 - x3, dy1 = y1 - y2;
		double denominator = dx1 * dy2 - dx2 * dy1;
		double a13 = (dx3 * dy2 - dx2 * dy3) / denominator, a23 = (dx1 * dy3 - dx3 * dy1) / denominator;
		return perspectiveTransform(x1 - x0 + a13 * x1, x3 - x0 + a23 * x3, x0, y1 - y0 + a13 * y1, y3 - y0 + a23 * y3, y0, a13, a23, 1.0);
	}
}
perspectiveTransform detector::quadrilateralToSquare(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3) {
    // Here, the adjoint serves as the inverse:
	return squareToQuadrilateral(x0, y0, x1, y1, x2, y2, x3, y3).buildAdjoint();
}
perspectiveTransform detector::quadrilateralToQuadrilateral(double x0, double y0, double x1, double y1, double x2, double y2, double x3, double y3, double x0p, double y0p, double x1p, double y1p, double x2p, double y2p, double x3p, double y3p) {
    auto qToS = quadrilateralToSquare(x0, y0, x1, y1, x2, y2, x3, y3);
	auto sToQ = squareToQuadrilateral(x0p, y0p, x1p, y1p, x2p, y2p, x3p, y3p);
	return sToQ.times(qToS);
}
perspectiveTransform detector::createTransform(center topLeft, center topRight, center bottomLeft, center* alignmentPattern, int dimension) {
    double dimMinusThree = (double)dimension - 3.5;
	double bottomRightX, bottomRightY, sourceBottomRightX, sourceBottomRightY;
	if (alignmentPattern != nullptr) {
		bottomRightX = alignmentPattern->x;
		bottomRightY = alignmentPattern->y;
        sourceBottomRightX = sourceBottomRightY = dimMinusThree - 3.0;
	}
    else {
        // Don't have an alignment pattern, just make up the bottom-right point
        bottomRightX = (topRight.x - topLeft.x) + bottomLeft.x;
        bottomRightY = (topRight.y - topLeft.y) + bottomLeft.y;
        sourceBottomRightX = sourceBottomRightY = dimMinusThree;
    }
    return quadrilateralToQuadrilateral(3.5, 3.5, dimMinusThree, 3.5, sourceBottomRightX, sourceBottomRightY, 3.5, dimMinusThree, topLeft.x, topLeft.y, topRight.x, topRight.y, bottomRightX, bottomRightY, bottomLeft.x, bottomLeft.y);
}
void detector::sampleGrid(vector<uint8_t> bitmap, perspectiveTransform transform, int dimension) {
    bits.assign(dimension + 2, vector<uint32_t>(dimension + 2, 1));
    vector<double> points(dimension << 1, 0.0);
    for (int y = 0; y < dimension; ++y) {
        double iValue = (double)y + 0.5;
		for (int x = 0; x < points.size(); x += 2) {
            points[x] = (double)(x >> 1) + 0.5;
            points[x + 1] = iValue;
        }
		transform.transformPoints(points);
        // Quick check to see if points transformed to something inside the image;
        // sufficient to check the endpoints
        checkAndNudgePoints(points);
        for (int x = 0; x < points.size(); x += 2) {
            if(bitmap[floor(points[x]) + width * floor(points[x + 1])]) bits[y + 1][(x >> 1) + 1] = 0;
        }
    }
}
void detector::checkAndNudgePoints(vector<double>& points) {
    // Check and nudge points from start until we see some that are OK:
    bool nudged = true;
	for (int offset = 0; offset < points.size() && nudged; offset += 2) {
        int x = floor(points[offset]);
		int y = floor(points[offset + 1]);
		if (x < - 1 || x > width || y < - 1 || y > height) {
            puts("4");
            throw "Error.checkAndNudgePoints";
        }
        nudged = false;
        if (x == -1) {
            points[offset] = 0.0;
            nudged = true;
        }
		else if (x == width) {
            points[offset] = width - 1;
            nudged = true;
        }
        if (y == -1) {
            points[offset + 1] = 0.0;
            nudged = true;
        }
        else if (y == height) {
            points[offset + 1] = height - 1;
            nudged = true;
        }
    }
    // Check and nudge points from end:
    nudged = true;
    for (int offset = points.size() - 2; offset >= 0 && nudged; offset -= 2) {
		int x = floor(points[offset]);
		int y = floor(points[offset + 1]);
		if (x < -1 || x > width || y < -1 || y > height) {
            puts("5");
            throw "Error.checkAndNudgePoints ";
        }
        nudged = false;
        if (x == -1) {
            points[offset] = 0.0;
            nudged = true;
        }
        else if (x == width) {
            points[offset] = width - 1;
            nudged = true;
        }
        if (y == - 1) {
            points[offset + 1] = 0.0;
            nudged = true;
        }
        else if (y == height) {
            points[offset + 1] = height - 1;
            nudged = true;
        }
    }
}