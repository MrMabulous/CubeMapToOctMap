/*
 * Copyright(c) 2020 Matthias Bühlmann, Mabulous GmbH. http://www.mabulous.com
*/

#include <algorithm>
#include <cassert>
#include <cmath>
#include <execution>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <functional>
#include <filesystem>

#include "OpenEXR/IlmImf/ImfArray.h"
#include "OpenEXR/IlmImf/ImfChannelList.h"
#include "OpenEXR/IlmImf/ImfOutputFile.h"
#include "OpenEXR/IlmImf/ImfInputFile.h"
#include "IlmBase/Imath/ImathMatrix.h"
#include "OpenEXR/IlmImf/ImfNamespace.h"

#include "cubemaputil.h"
#include "octmaputil.h"
#include "filter.h"

#include "stringutils.h"

using namespace std;
using namespace std::placeholders;
namespace IMF = OPENEXR_IMF_NAMESPACE;
using namespace OPENEXR_IMF_NAMESPACE;
using namespace IMATH_NAMESPACE;


void displayHelp() {
    cout << "Arguments:.\n";
    cout << "-h --help\n";
    cout << "-i --input inputfile  : input cubemap exr file.\n";
    cout << "-o --output outputfile  : output cubemap exr file.\n";
    cout << "-c --compression [rle/piz/zip/pxr24/b44/b44a/dwaa/dwab]  : OpenEXR compression schemes. default is zip.\n";
    cout << "-t --transform transformationmatrix ... : 16 floats defining transformation matrix to transform input colors by.\n";
    cout << "-e --encode  : treats the (altready transformed) color as direction vector and encodes it as octmap uv coordinate and writes it to RG.\n";
    cout << "-m --mono  : write monochromatic output.\n";
    cout << "-r --resample [nearest/bilinear/gaussian/mitchell]  : resampling type. default is mitchell.\n";
}

void
writeRGB(const char fileName[],
    const float *rgbPixels,
    int width,
    int height,
    Compression compression = ZIP_COMPRESSION)
{

    Header header(width, height);
    header.channels().insert("R", Channel(IMF::FLOAT));
    header.channels().insert("G", Channel(IMF::FLOAT));
    header.channels().insert("B", Channel(IMF::FLOAT));

    header.compression() = compression;

    OutputFile file(fileName, header);

    FrameBuffer frameBuffer;

    frameBuffer.insert("R",					// name
        Slice(IMF::FLOAT,					// type
        (char *)rgbPixels,					// base
            sizeof(*rgbPixels) * 3,				// xStride
            sizeof(*rgbPixels) * 3 * width));	// yStride

    frameBuffer.insert("G",					// name
        Slice(IMF::FLOAT,					// type
        (char *)(rgbPixels + 1),				// base
            sizeof(*rgbPixels) * 3,				// xStride
            sizeof(*rgbPixels) * 3 * width));	// yStride

    frameBuffer.insert("B",					// name
        Slice(IMF::FLOAT,					// type
        (char *)(rgbPixels + 2),			// base
            sizeof(*rgbPixels) * 3,				// xStride
            sizeof(*rgbPixels) * 3 * width));	// yStride

    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}

void
writeZ(const char fileName[],
    const float *rgbPixels,
    int width,
    int height,
    Compression compression = ZIP_COMPRESSION)
{
    Header header(width, height);
    header.channels().insert("Z", Channel(IMF::FLOAT));

    header.compression() = compression;

    OutputFile file(fileName, header);

    FrameBuffer frameBuffer;

    frameBuffer.insert("Z",					// name
        Slice(IMF::FLOAT,					// type
        (char *)rgbPixels,					// base
            sizeof(*rgbPixels) * 3,				// xStride
            sizeof(*rgbPixels) * 3 * width));	// yStride

    file.setFrameBuffer(frameBuffer);
    file.writePixels(height);
}

void
readRGB(const char fileName[],
    Array2D<float> &rgbPixels,
    int &width, int &height)
{
    InputFile file(fileName);

    Header header = file.header();
    Box2i dw = header.dataWindow();
    width = dw.max.x - dw.min.x + 1;
    height = dw.max.y - dw.min.y + 1;

    rgbPixels.resizeErase(height, width*3);

    FrameBuffer frameBuffer;

    frameBuffer.insert("R",							// name
        Slice(IMF::FLOAT,							// type
            (char *)(&rgbPixels[0][0] -				// base
            dw.min.x * 3 -
            dw.min.y * 3 * width),
            sizeof(rgbPixels[0][0]) * 3,			// xStride
            sizeof(rgbPixels[0][0]) * 3 * width));	// yStride

    frameBuffer.insert("G",							// name
        Slice(IMF::FLOAT,							// type
            (char *)(&rgbPixels[0][1] -				// base
            dw.min.x * 3 -
            dw.min.y * 3 * width),
            sizeof(rgbPixels[0][0]) * 3,			// xStride
            sizeof(rgbPixels[0][0]) * 3 * width));	// yStride

    frameBuffer.insert("B",							// name
        Slice(IMF::FLOAT,							// type
            (char *)(&rgbPixels[0][2] -				// base
            dw.min.x * 3 -
            dw.min.y * 3 * width),
            sizeof(rgbPixels[0][0]) * 3,			// xStride
            sizeof(rgbPixels[0][0]) * 3 * width));	// yStride

    file.setFrameBuffer(frameBuffer);
    file.readPixels(dw.min.y, dw.max.y);
}

enum ResampleType {
    NEAREST,
    BILINEAR,
    GAUSSIAN,
    MITCHELL
};

int main( int argc, char *argv[], char *envp[] ) {

    if(argc < 2) {
        displayHelp();
        return 0;
    }

    vector<string> args(argv + 1, argv + argc);
    string inputFile = "";
    string outputFile = "";
    Compression compression = ZIP_COMPRESSION;
    bool writeMono = false;

    bool transform = false;
    Imath::Matrix44<float> transformMatrix;

    bool encodeColor = false;

    ResampleType resample = MITCHELL;
    MitchellFilter mitchellFilter;
    GaussianFilter gaussianFilter;
    Filter* filter = &mitchellFilter;

    // Loop over remaining command-line args
    for (vector<string>::iterator i = args.begin(); i != args.end(); ++i) {
        if (*i == "-h" || *i == "--help") {
            displayHelp();
            return 0;
        } else if (*i == "-i" || *i == "--input") {
            inputFile = *++i;
        }
        else if (*i == "-o" || *i == "--output") {
            outputFile = *++i;
        }
        else if (*i == "-r" || *i == "--resample") {
            string resampleName = toLower(*++i);
            if (resampleName == "nearest")
                resample = NEAREST;
            else if (resampleName == "bilinear")
                resample = BILINEAR;
            else if (resampleName == "gaussian") {
                filter = &gaussianFilter;
                resample = GAUSSIAN;
            }
            else if (resampleName == "mitchell") {
                resample = MITCHELL;
                filter = &mitchellFilter;
            }
            else {
                cout << "unknown resampling method: " << *i << "\n";
                displayHelp();
                return 1;
            }
        }
        else if (*i == "-m" || *i == "--mono") {
            writeMono = true;
        }
        else if (*i == "-t" || *i == "--transform") {
            transform = true;
            for (int y = 0; y < 4; y++) {
                for (int x = 0; x < 4; x++) {
                    transformMatrix[y][x] = stof(*++i);
                }
            }
            // matrix vector multiplication is implemented as row-vector multiplication, hence transpose the matrix.
            transformMatrix.transpose();
        }
        else if (*i == "-e" || *i == "--encode") {
            encodeColor = true;
        }
        else if (*i == "-c" || *i == "--compression") {
            string compressionString = toLower(*++i);
            if (compressionString == "no") {
                compression = NO_COMPRESSION;
            }
            else if (compressionString == "rle") {
                compression = RLE_COMPRESSION;
            }
            else if (compressionString == "zip_single") {
                compression = ZIPS_COMPRESSION;
            }
            else if (compressionString == "zip") {
                compression = ZIP_COMPRESSION;
            }
            else if (compressionString == "piz") {
                compression = PIZ_COMPRESSION;
            }
            else if (compressionString == "pxr24") {
                compression = PXR24_COMPRESSION;
            }
            else if (compressionString == "b44") {
                compression = B44_COMPRESSION;
            }
            else if (compressionString == "b44a") {
                compression = B44A_COMPRESSION;
            }
            else if (compressionString == "dwaa") {
                compression = DWAA_COMPRESSION;
            }
            else if (compressionString == "dwab") {
                compression = DWAB_COMPRESSION;
            }
            else {
                cout << "unknown compression method: " << *i << "\n";
                displayHelp();
                return 1;
            }
        } else {
            cout << "unknown argument " <<  *i << "\n";
            displayHelp();
            return 1;
        }
    }

    if (encodeColor && writeMono) {
        cout << "-e and -m cannot be used together\n";
        displayHelp();
        return 1;
    }

    cout << "input file: " << inputFile << " output file: " << outputFile << "\n";

    set<std::string> patches;
    filesystem::path filePath(inputFile);
    filesystem::path folderPath = filePath.parent_path();
    filesystem::path fileName = filePath.filename();
    string fileNameString = fileName.string();
    size_t numWildcards = std::count(fileNameString.begin(), fileNameString.end(), '#');
    if (numWildcards > 1) {
        cout << "error: multiple # in " << filePath.string() << "\n";
        cout << "use maximally one # wildcard per filename.\n";
        return 1;
    }
    else if (numWildcards == 1) {
        if (std::count(outputFile.begin(), outputFile.end(), '#') != 1) {
            cout << "error: if using a # wildcard in the input file name, there must be a # in the output file name as well.\n";
            return 1;
        }
        vector<string> nameSplit = split(toLower(fileNameString), "#");
        assert(nameSplit.size() == 2);
        for (const auto & entry : filesystem::directory_iterator(folderPath)) {
            std::string otherFileName = toLower(entry.path().filename().string());
            size_t prefixPos = otherFileName.find(nameSplit[0]);
            size_t postfixPos = otherFileName.find(nameSplit[1]);
            if (prefixPos == 0 && postfixPos != string::npos) {
                string patch = otherFileName.substr(nameSplit[0].size(),
                    otherFileName.size() - nameSplit[0].size() - nameSplit[1].size());
                patches.insert(patch);
            }
        }
    }
    else {
        if (!filesystem::exists(filePath)) {
            cout << "error: " << filePath.string() << " does not exist.\n";
            return 1;
        }
        patches.insert("");
    }

    std::for_each(
        std::execution::par_unseq,
        patches.begin(),
        patches.end(),
        [&](const string& patch)
    {
        int width, height;
        Array2D<float> inputImage;
        string actualInputFilePath = inputFile;
        size_t hashPos = actualInputFilePath.find("#");
        if (hashPos != string::npos)
            actualInputFilePath.replace(hashPos, 1, patch);
        cout << "reading " << actualInputFilePath << "\n";
        readRGB(actualInputFilePath.c_str(), inputImage, width, height);

        Array2D<float> outputImage;
        outputImage.resizeErase(height, height * 3);
        static float debug_colors[6][3] = { {1,0,0},{0,1,0},{0,0,1},{1,0.5f,0.5f},{0.5f,1,0.5f},{0.5f,0.5f,1} };
        for (int y = 0; y < height; y++) {
            for (int x = 0; x < height; x++) {
                Imath::V2f octMapCoord(((x+0.5f) / height) * 2.0f - 1.0f, 1.0f - ((y+0.5f) / height) * 2.0f);
                int face;
                Imath::V2f cubeMapCoord = cubeEncode(octDecode(octMapCoord), &face);
                Imath::V3f col(0,0,0);
                if (resample == NEAREST) {
                    int inputPixX = std::min(height-1, int(cubeMapCoord.x * height));
                    inputPixX += height * face;
                    int inputPixY = std::min(height-1, int((1.0f - cubeMapCoord.y) * height));
                    col = Imath::V3f(inputImage[inputPixY][inputPixX * 3 + 0],
                                     inputImage[inputPixY][inputPixX * 3 + 1],
                                     inputImage[inputPixY][inputPixX * 3 + 2]);
                }
                else if (resample == BILINEAR) { // BILINEAR
                    float xCoord = cubeMapCoord.x * height;
                    float yCoord = (1.0f - cubeMapCoord.y) * height;
                    int lowX = std::max(0, int(xCoord - 0.5f));
                    int lowY = std::max(0, int(yCoord - 0.5f));
                    int highX = std::min(height-1, lowX + 1);
                    int highY = std::min(height-1, lowY + 1);
                    float hFrac = xCoord - (lowX + 0.5f);
                    float vFrac = yCoord - (lowY + 0.5f);
                    lowX += height * face;
                    highX += height * face;
                    for (int c = 0; c < 3; c++) {
                        float topleft = inputImage[lowY][lowX * 3 + c];
                        float topright = inputImage[lowY][highX * 3 + c];
                        float bottomleft = inputImage[highY][lowX * 3 + c];
                        float bottomright = inputImage[highY][highX * 3 + c];
                        col[c] = (topleft * (1 - hFrac) + topright * hFrac) * (1 - vFrac) + (bottomleft * (1 - hFrac) + bottomright * hFrac) * vFrac;
                    }
                } else {
                    float radius = filter->GetRadius();
                    float tmp;
                    const int supportExtent = 3;
                    const int sampleCount = (2 * supportExtent + 1) * (2 * supportExtent + 1);
                    float weight = 0.0;
                    for(float xOfst = -supportExtent; xOfst <= supportExtent; xOfst++) {
                        for(float yOfst = -supportExtent; yOfst <= supportExtent; yOfst++) {
                            Imath::V2f pixelOfst = Imath::V2f(xOfst,yOfst) * radius / (supportExtent + 1);
                            Imath::V2f octCoordOfst = pixelOfst * (2.0f / height);
                            Imath::V2f octMapSampleCoord = octMapCoord + octCoordOfst;
                            // compute wrapover
                            if(abs(octMapSampleCoord.x) > 1.0f) {
                              float overlap = abs(octMapSampleCoord.x) - 1.0f;
                              octMapSampleCoord.x = sign(octMapSampleCoord.x) - overlap;
                              octMapSampleCoord.y = -octMapSampleCoord.y;
                            }
                            if(abs(octMapSampleCoord.y) > 1.0f) {
                              float overlap = abs(octMapSampleCoord.y) - 1.0f;
                              octMapSampleCoord.y = sign(octMapSampleCoord.y) - overlap;
                              octMapSampleCoord.x = -octMapSampleCoord.x;
                            }
                            int sampleFace;
                            Imath::V2f cubeMapSampleCoord = cubeEncode(octDecode(octMapSampleCoord), &sampleFace);
                            int inputPixX = std::min(height-1, int(cubeMapSampleCoord.x * height));
                            inputPixX += height * sampleFace;
                            int inputPixY = std::min(height-1, int((1.0f - cubeMapSampleCoord.y) * height));
                            Imath::V3f sampleCol = Imath::V3f(inputImage[inputPixY][inputPixX * 3 + 0],
                                                              inputImage[inputPixY][inputPixX * 3 + 1],
                                                              inputImage[inputPixY][inputPixX * 3 + 2]);
                            float filterWeight = filter->Eval(pixelOfst);
                            weight += filterWeight;
                            col = col + sampleCol * filterWeight;
                        }                        
                    }
                    col /= weight;
                }
                if (transform) {
                    Imath::V3f transformedCol;
                    transformMatrix.multVecMatrix(col, transformedCol);
                    col = transformedCol;
                }
                if (encodeColor) {
                    col = col * 2 - Imath::V3f(1, 1, 1);
                    Imath::V2f uv = octEncode(col);
                    col[0] = (uv[0] + 1) * 0.5f;
                    col[1] = (uv[1] + 1) * 0.5f;
                    col[2] = 0;
                }
                for (int c = 0; c < 3; c++) {
                    outputImage[y][x * 3 + c] = col[c];
                }
            }
        }

        string actualOutputFilePath = outputFile;
        hashPos = actualOutputFilePath.find("#");

        if (hashPos != string::npos)
            actualOutputFilePath.replace(hashPos, 1, patch);
        cout << "writing file: " << actualOutputFilePath << "\n";
        if (writeMono) {
            writeZ(actualOutputFilePath.c_str(), outputImage[0], height, height, compression);
        }
        else {
            writeRGB(actualOutputFilePath.c_str(), outputImage[0], height, height, compression);
        }
    });
}