#include <iostream>
#include <fstream>
#include <cstdint>
#include <vector>
#include <iomanip>
#include <algorithm>

// this is standard file structure on BMP, check BMP site to see complete info
#pragma pack(push, 1)
struct BitmapFileHeader
{
    uint16_t file_type{0x4D42};
    uint32_t file_size{0};
    uint16_t reserved1{0};
    uint16_t reserved2{0};
    uint32_t pixelDataOffset{0};
};
struct BitmapInfoHeader
{
    uint32_t size{0};
    int32_t width{0};
    int32_t height{0};
    uint16_t planes{1};
    uint16_t bit_count{0};
    uint32_t compression{0};
    uint32_t size_image{0};
    int32_t x_pixels_per_meter{0};
    int32_t y_pixels_per_meter{0};
    uint32_t colors_used{0};
    uint32_t colors_important{0};
};
#pragma pack(pop)

class BMPReader 
{
public:
    std::vector<uint8_t> read(const char* filename = "baby_GT.bmp") 
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file) return {};
        // check to see if there no files so the code doesn't hang
        file.read(reinterpret_cast<char*>(&bfh), sizeof(bfh));
        file.read(reinterpret_cast<char*>(&bih), sizeof(bih));
        file.seekg(bfh.pixelDataOffset, file.beg);
        int row_stride = (bih.width * 3 + 3) & ~3;
        std::vector<uint8_t> padding(row_stride - bih.width * 3);

        pixels.resize(bih.width * std::abs(bih.height) * 3);
        // BGR values
        for (int y = 0; y < std::abs(bih.height); ++y) 
        {
            file.read(reinterpret_cast<char*>(pixels.data() + (y * bih.width * 3)), bih.width * 3);
            file.read(reinterpret_cast<char*>(padding.data()), padding.size());
        }
        
        std::cout << "Dimensions: " << bih.width << "x" << bih.height << std::endl;
        return pixels;
    }

    int32_t getWidth() const { return bih.width; }
    int32_t getHeight() const { return std::abs(bih.height); }

private:
    BitmapFileHeader bfh;
    BitmapInfoHeader bih;
    std::vector<uint8_t> pixels;
};
class gaussianfilter 
{
public:
    gaussianfilter(const std::vector<uint8_t>& data, int w, int h) 
        : image_data(data), width(w), height(h)
    {
        apply();
    }

    std::vector<uint8_t> getImageData() const { return image_data; }

private:
    void apply() 
    {
        std::vector<uint8_t> temp(image_data.size());
        // Horizontal pass
        convolve(image_data, temp, 1, 0);
        // Vertical pass
        convolve(temp, image_data, 0, 1);
    }

    void convolve(const std::vector<uint8_t>& src, std::vector<uint8_t>& dst, int dx, int dy) 
    {
        int radius = 2;
        for (int y = 0; y < height; ++y) 
        {
            for (int x = 0; x < width; ++x) 
            {
                for (int c = 0; c < 3; ++c) 
                {
                    double sum = 0.0;
                    for (int k = -radius; k <= radius; ++k) 
                    {
                        int nx = std::min(std::max(x + k * dx, 0), width - 1);
                        int ny = std::min(std::max(y + k * dy, 0), height - 1);
                        
                        sum += src[(ny * width + nx) * 3 + c] * kernel[k + radius];
                    }
                    dst[(y * width + x) * 3 + c] = static_cast<uint8_t>(std::min(255.0, std::max(0.0, sum)));
                }
            }
        }
    }

    double kernel[5] = {0.01, 0.18, 0.62, 0.18, 0.01};
    // low sigma for sharper downsample
    std::vector<uint8_t> image_data;
    int width, height;
};

class bilineardown
{
public:
    bilineardown(const std::vector<uint8_t>& src, int w, int h, int target_w, int target_h)
        : width(target_w), height(target_h)
    {
        resize(src, w, h);
    }

    std::vector<uint8_t> getResult() const { return data; }

private:
    void resize(const std::vector<uint8_t>& src, int src_w, int src_h) 
    {
        data.resize(width * height * 3);
        float x_scale = (float)src_w / width;
        float y_scale = (float)src_h / height;

        for (int y = 0; y < height; ++y) 
        {
            for (int x = 0; x < width; ++x) 
            {
                float src_x = x * x_scale;
                float src_y = y * y_scale;

                int x1 = (int)src_x;
                int y1 = (int)src_y;
                int x2 = std::min(x1 + 1, src_w - 1);
                int y2 = std::min(y1 + 1, src_h - 1);

                float dx = src_x - x1;
                float dy = src_y - y1;

                for (int c = 0; c < 3; ++c) 
                {
                    float val = (1 - dx) * (1 - dy) * src[(y1 * src_w + x1) * 3 + c] +
                                dx * (1 - dy) * src[(y1 * src_w + x2) * 3 + c] +
                                (1 - dx) * dy * src[(y2 * src_w + x1) * 3 + c] +
                                dx * dy * src[(y2 * src_w + x2) * 3 + c];
                    data[(y * width + x) * 3 + c] = static_cast<uint8_t>(val);
                }
            }
        }
    }
    std::vector<uint8_t> data;
    int width, height;
};
// save file
void writeBMP(const char* filename, int w, int h, const std::vector<uint8_t>& data) 
{
    std::ofstream file(filename, std::ios::binary);
    if (!file) return;

    BitmapFileHeader bfh;
    BitmapInfoHeader bih;

    int row_stride = (w * 3 + 3) & ~3;
    int padding_size = row_stride - (w * 3);

    bfh.file_type = 0x4D42; // 'BM'
    bfh.pixelDataOffset = sizeof(bfh) + sizeof(bih);
    bfh.file_size = bfh.pixelDataOffset + row_stride * h;

    bih.size = sizeof(bih);
    bih.width = w;
    bih.height = h;
    bih.planes = 1;
    bih.bit_count = 24;
    bih.size_image = row_stride * h;

    file.write(reinterpret_cast<char*>(&bfh), sizeof(bfh));
    file.write(reinterpret_cast<char*>(&bih), sizeof(bih));

    std::vector<uint8_t> padding(padding_size, 0);
    for (int y = 0; y < h; ++y)
    {
        file.write(reinterpret_cast<const char*>(data.data() + (y * w * 3)), w * 3);
        file.write(reinterpret_cast<char*>(padding.data()), padding_size);
    }
    std::cout << "Saved " << filename << " (" << w << "x" << h << ")" << std::endl;
}

int main() 
{
    // 1. Read BMP Image
    BMPReader reader;
    std::vector<uint8_t> pixels = reader.read();
    int width = reader.getWidth();
    int height = reader.getHeight();
    
    // 2. Apply Gaussian Filter
    gaussianfilter gf(pixels, width, height);
    
    // 3. Apply Bilinear Downsampling (half size)
    int new_w = width / 2;
    int new_h = height / 2;
    bilineardown bd(gf.getImageData(), width, height, new_w, new_h);

    // 4. Save Result
    writeBMP("output.bmp", new_w, new_h, bd.getResult());
    return 0;
}