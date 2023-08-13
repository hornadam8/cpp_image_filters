
#include <iostream>
#include <vector>
#include <fstream>
#include <cmath>
#include <tuple>
using namespace std;

// Pixel structure
struct Pixel
{
    // Red, green, blue color values
    int red;
    int green;
    int blue;
};

/**
 * Gets an integer from a binary stream.
 * Helper function for read_image()
 * @param stream the stream
 * @param offset the offset at which to read the integer
 * @param bytes  the number of bytes to read
 * @return the integer starting at the given offset
 */ 
int get_int(fstream& stream, int offset, int bytes)
{
    stream.seekg(offset);
    int result = 0;
    int base = 1;
    for (int i = 0; i < bytes; i++)
    {   
        result = result + stream.get() * base;
        base = base * 256;
    }
    return result;
}

/**
 * Reads the BMP image specified and returns the resulting image as a vector
 * @param filename BMP image filename
 * @return the image as a vector of vector of Pixels
 */
vector<vector<Pixel>> read_image(string filename)
{
    // Open the binary file
    fstream stream;
    stream.open(filename, ios::in | ios::binary);

    // Get the image properties
    int file_size = get_int(stream, 2, 4);
    int start = get_int(stream, 10, 4);
    int width = get_int(stream, 18, 4);
    int height = get_int(stream, 22, 4);
    int bits_per_pixel = get_int(stream, 28, 2);

    // Scan lines must occupy multiples of four bytes
    int scanline_size = width * (bits_per_pixel / 8);
    int padding = 0;
    if (scanline_size % 4 != 0)
    {
        padding = 4 - scanline_size % 4;
    }

    // Return empty vector if this is not a valid image
    if (file_size != start + (scanline_size + padding) * height)
    {
        return {};
    }

    // Create a vector the size of the input image
    vector<vector<Pixel>> image(height, vector<Pixel> (width));

    int pos = start;
    // For each row, starting from the last row to the first
    // Note: BMP files store pixels from bottom to top
    for (int i = height - 1; i >= 0; i--)
    {
        // For each column
        for (int j = 0; j < width; j++)
        {
            // Go to the pixel position
            stream.seekg(pos);

            // Save the pixel values to the image vector
            // Note: BMP files store pixels in blue, green, red order
            image[i][j].blue = stream.get();
            image[i][j].green = stream.get();
            image[i][j].red = stream.get();

            // We are ignoring the alpha channel if there is one

            // Advance the position to the next pixel
            pos = pos + (bits_per_pixel / 8);
        }

        // Skip the padding at the end of each row
        stream.seekg(padding, ios::cur);
        pos = pos + padding;
    }

    // Close the stream and return the image vector
    stream.close();
    return image;
}

/**
 * Sets a value to the char array starting at the offset using the size
 * specified by the bytes.
 * This is a helper function for write_image()
 * @param arr    Array to set values for
 * @param offset Starting index offset
 * @param bytes  Number of bytes to set
 * @param value  Value to set
 * @return nothing
 */
void set_bytes(unsigned char arr[], int offset, int bytes, int value)
{
    for (int i = 0; i < bytes; i++)
    {
        arr[offset+i] = (unsigned char)(value>>(i*8));
    }
}

/**
 * Write the input image to a BMP file name specified
 * @param filename The BMP file name to save the image to
 * @param image    The input image to save
 * @return True if successful and false otherwise
 */
bool write_image(string filename, const vector<vector<Pixel>>& image)
{
    // Get the image width and height in pixels
    int width_pixels = image[0].size();
    int height_pixels = image.size();

    // Calculate the width in bytes incorporating padding (4 byte alignment)
    int width_bytes = width_pixels * 3;
    int padding_bytes = 0;
    padding_bytes = (4 - width_bytes % 4) % 4;
    width_bytes = width_bytes + padding_bytes;

    // Pixel array size in bytes, including padding
    int array_bytes = width_bytes * height_pixels;

    // Open a file stream for writing to a binary file
    fstream stream;
    stream.open(filename, ios::out | ios::binary);

    // If there was a problem opening the file, return false
    if (!stream.is_open())
    {
        return false;
    }

    // Create the BMP and DIB Headers
    const int BMP_HEADER_SIZE = 14;
    const int DIB_HEADER_SIZE = 40;
    unsigned char bmp_header[BMP_HEADER_SIZE] = {0};
    unsigned char dib_header[DIB_HEADER_SIZE] = {0};

    // BMP Header
    set_bytes(bmp_header,  0, 1, 'B');              // ID field
    set_bytes(bmp_header,  1, 1, 'M');              // ID field
    set_bytes(bmp_header,  2, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE+array_bytes); // Size of BMP file
    set_bytes(bmp_header,  6, 2, 0);                // Reserved
    set_bytes(bmp_header,  8, 2, 0);                // Reserved
    set_bytes(bmp_header, 10, 4, BMP_HEADER_SIZE+DIB_HEADER_SIZE); // Pixel array offset

    // DIB Header
    set_bytes(dib_header,  0, 4, DIB_HEADER_SIZE);  // DIB header size
    set_bytes(dib_header,  4, 4, width_pixels);     // Width of bitmap in pixels
    set_bytes(dib_header,  8, 4, height_pixels);    // Height of bitmap in pixels
    set_bytes(dib_header, 12, 2, 1);                // Number of color planes
    set_bytes(dib_header, 14, 2, 24);               // Number of bits per pixel
    set_bytes(dib_header, 16, 4, 0);                // Compression method (0=BI_RGB)
    set_bytes(dib_header, 20, 4, array_bytes);      // Size of raw bitmap data (including padding)                     
    set_bytes(dib_header, 24, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 28, 4, 2835);             // Print resolution of image (2835 pixels/meter)
    set_bytes(dib_header, 32, 4, 0);                // Number of colors in palette
    set_bytes(dib_header, 36, 4, 0);                // Number of important colors

    // Write the BMP and DIB Headers to the file
    stream.write((char*)bmp_header, sizeof(bmp_header));
    stream.write((char*)dib_header, sizeof(dib_header));

    // Initialize pixel and padding
    unsigned char pixel[3] = {0};
    unsigned char padding[3] = {0};

    // Pixel Array (Left to right, bottom to top, with padding)
    for (int h = height_pixels - 1; h >= 0; h--)
    {
        for (int w = 0; w < width_pixels; w++)
        {
            // Write the pixel (Blue, Green, Red)
            pixel[0] = image[h][w].blue;
            pixel[1] = image[h][w].green;
            pixel[2] = image[h][w].red;
            stream.write((char*)pixel, 3);
        }
        // Write the padding bytes
        stream.write((char *)padding, padding_bytes);
    }

    // Close the stream and return true
    stream.close();
    return true;
}

typedef vector<vector<Pixel>> Image;
typedef Image (*Process)(const Image&);

//**************************************************************************************************//
//                                       Processing Helpers                                         //
//**************************************************************************************************//

std::tuple<int, int> size_image(const Image& image) {
    int rows = image.size();
    int cols = image[0].size();
    return std::make_tuple(rows, cols);
}

std::tuple<int,int,int> rbg_pixel(Pixel p) {
    return std::make_tuple(p.red, p.blue, p.green);
}

Pixel new_pixel(int r, int b, int g) {
    Pixel new_pixel = Pixel();
    new_pixel.red = r;
    new_pixel.blue = b;
    new_pixel.green = g;
    return new_pixel;
}

double get_scale() {
    double scale = 0.0;
    while (scale <= 0.0 || scale > 1.0 || cin.fail()) {
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cout << "Enter scaling factor: ";
        cin >> scale;
        cout << endl;
        if (scale > 1.0 || scale <= 0.0 || cin.fail()) {
            cout << "Invalid input! Please enter an integer > 0" << endl;
        }
    }
    return scale;
}

Image rotate_90(const Image& image, int rotations) {
    // 4 is a 360 so true number of spins is num % 4
    rotations = rotations % 4;
    if (rotations == 0) {
        return image;
    }
    int rows, cols;
    tie(rows, cols) = size_image(image);
    Image new_image(cols, vector<Pixel> (rows));
    Image new_image_mirror(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Pixel p = image[row][col];
            new_image[(cols - 1) - col][row] = p;
        }
    }
    for (int i = 2; i <= rotations; i++) {
        for (int row = 0; row < rows; row++) {
            for (int col = 0; col < cols; col++) {
                if (i % 2 == 0) {
                    Pixel p = new_image[col][row];
                    new_image_mirror[(rows - 1) - row][col] = p;
                } else {
                    Pixel p = new_image_mirror[row][col];
                    new_image[(cols - 1) - col][row] = p;
                }
            }
        }
    }
    if (rotations % 2 == 0) {
        return new_image_mirror;
    } else {
        return new_image;
    }
}

//**************************************************************************************************//
//                               Image Processing functions                                         //
//**************************************************************************************************//

Image process_1(const Image& image) {
    int rows, cols;
    tie(rows,cols) = size_image(image);
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int red, blue, green;
            Pixel p = image[row][col];
            tie(red, blue, green) = rbg_pixel(p);
            // find distance to center
            double dist = sqrt(pow((col - (cols/2.0)), 2.0) + pow(row - (rows/2.0), 2.0));
            double scale_factor = (rows - dist)/ rows;

            int n_red, n_blue, n_green;
            n_red = red * scale_factor;
            n_blue = blue * scale_factor;
            n_green = green * scale_factor;

            new_image[row][col] = new_pixel(n_red, n_blue, n_green);
        }
    }
    return new_image;
}

Image process_2(const Image& image) {
    int rows, cols;
    tie(rows, cols) = size_image(image);
    double scale_factor = get_scale();
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int red, green, blue;
            Pixel p = image[row][col];
            tie(red, blue, green) = rbg_pixel(p);
            int avg = (red + blue + green) / 3;
            int n_red, n_blue, n_green;
            // if pixel is light, make lighter
            if (avg >= 170) {
                n_red = (255 - (255 - red)*scale_factor);
                n_blue = (255 - (255 - blue)*scale_factor);
                n_green = (255 - (255 - green)*scale_factor);
            } else if (avg < 90) {
                n_red = red * scale_factor;
                n_blue = blue * scale_factor;
                n_green = green * scale_factor;
            } else {
                n_red = red;
                n_blue = blue;
                n_green = green;
            }
            new_image[row][col] = new_pixel(n_red, n_blue, n_green);
        }
    }
    return new_image;
}

Image process_3(const Image& image) {
    int rows, cols;
    tie(rows,cols) = size_image(image);
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int red, green, blue;
            Pixel p = image[row][col];
            tie(red, blue, green) = rbg_pixel(p);
            int avg = (red + blue + green) / 3;
            new_image[row][col] = new_pixel(avg, avg, avg);
        }
    }
    return new_image;
}

Image process_4(const Image& image) {
    return rotate_90(image, 1);
}

Image process_5(const Image& image) {
    int num = 0;
    while (num < 1 || cin.fail()) {
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cout << "Enter number of 90 degree rotations: ";
        cin >> num;
        if (num < 1 || cin.fail()) {
            cout << "Invalid input! Please enter an integer > 0" << endl;
        }
    }
    return rotate_90(image, num);
}

Image process_6(const Image& image) {
    int rows, cols;
    tie(rows,cols) = size_image(image);
    int x = 0;
    while (x < 1 || cin.fail()) {
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cout << "Enter X scale: ";
        cin >> x;
        cout << endl;
        if (x < 1 || cin.fail()) {
            cout << "Invalid input! Please enter an integer > 0" << endl;
        }
    }
    int y = 0;
    while (y < 1 || cin.fail()) {
        cin.clear();
        cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        cout << "Enter Y scale: ";
        cin >> y;
        cout << endl;
        if (y < 1 || cin.fail()) {
            cout << "Invalid input! Please enter an integer > 0" << endl;
        }
    }
    Image new_image(rows * y, vector<Pixel>(cols * x));
    for (int row = 0; row < y * rows; row++) {
        for (int col = 0; col < x * cols; col ++) {
            Pixel p = image[row/y][col/x];
            new_image[row][col] = p;
        }
    }
    return new_image;
}

Image process_7(const Image& image) {
    int rows, cols;
    tie(rows,cols) = size_image(image);
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Pixel p = image[row][col];
            int red, blue, green;
            tie(red, blue, green) = rbg_pixel(p);
            double avg = (red + blue + green) / 3.0;
            int n_red, n_blue, n_green;
            if (avg >= 127.5) {
                n_red = 255;
                n_blue = 255;
                n_green = 255;
            } else {
                n_red = 0;
                n_blue = 0;
                n_green = 0;
            }
            new_image[row][col] = new_pixel(n_red, n_blue, n_green);
        }
    }
    return new_image;
}

Image process_8(const Image& image) {
    double scale = get_scale();
    int rows, cols;
    tie(rows,cols) = size_image(image);
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Pixel p = image[row][col];
            int red, blue, green;
            tie(red,blue,green) = rbg_pixel(p);
            int n_red, n_blue, n_green;
            n_red = 255 - ((255 - red) * scale);
            n_blue = 255 - ((255 - blue) * scale);
            n_green = 255 - ((255 - green) * scale);
            new_image[row][col] = new_pixel(n_red, n_blue, n_green);
        }
    }
    return new_image;
}

Image process_9(const Image& image) {
    double scale = get_scale();
    int rows, cols;
    tie(rows,cols) = size_image(image);
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Pixel p = image[row][col];
            int red, blue, green;
            tie(red,blue,green) = rbg_pixel(p);
            int n_red, n_blue, n_green;
            n_red = red * scale;
            n_blue = blue * scale;
            n_green = green * scale;
            new_image[row][col] = new_pixel(n_red, n_blue, n_green);
        }
    }
    return new_image;
}

Image process_10(const Image& image) {
    int rows, cols;
    tie(rows,cols) = size_image(image);
    Image new_image(rows, vector<Pixel> (cols));
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            Pixel p = image[row][col];
            int red, blue, green;
            tie(red, blue, green) = rbg_pixel(p);
            int mx = max(red, blue);
            mx = max(mx, green);
            int sum = red + blue + green;
            int n_red, n_blue, n_green;
            if (sum >= 550) {
                n_red = 255;
                n_blue = 255;
                n_green = 255;
            } else if (sum <= 150) {
                n_red = 0;
                n_blue = 0;
                n_green = 0;
            } else if (mx == red) {
                n_red = 255;
                n_blue = 0;
                n_green = 0;
            } else if (mx == green) {
                n_red = 0;
                n_blue = 0;
                n_green = 255;
            } else {
                n_red = 0;
                n_blue = 255;
                n_green = 0;
            }
            new_image[row][col] = new_pixel(n_red, n_blue, n_green);
        }
    }
    return new_image;
}

//**************************************************************************************************//
//                                       UI functions                                               //
//**************************************************************************************************//

string get_input_filename() {
    cout << "Enter input BMP filename: ";
    string input;
    cin >> input;
    return input;
}

string get_output_filename() {
    cout << "Enter output BMP filename: ";
    string input;
    cin >> input;
    return input;
}


void print_menu(string& current_file) {
    cout << endl;
    cout << "IMAGE PROCESSING MENU" << endl;
    cout << " 1) Vignette" << endl;
    cout << " 2) Clarendon" << endl;
    cout << " 3) Grayscale" << endl;
    cout << " 4) Rotate 90 degrees" << endl;
    cout << " 5) Rotate multiple 90 degrees" << endl;
    cout << " 6) Enlarge" << endl;
    cout << " 7) High contrast" << endl;
    cout << " 8) Lighten" << endl;
    cout << " 9) Darken" << endl;
    cout << "10) Black, white, red, green, blue" << endl;
    cout << "11) Change image (current: " << current_file << ")" << endl;
    cout << endl;
    cout << "Enter menu selection (Q/q to quit): ";
}

//**************************************************************************************************//
//                                       Handler Helpers                                            //
//**************************************************************************************************//

Image get_image(string filename, string filter_name) {
    cout << endl;
    cout << filter_name <<" selected" << endl;
    return read_image(filename);
}

void respond(string filter_name, Image& new_image) {
    string output_file = get_output_filename();
    write_image(output_file, new_image);
    cout << "Successfully applied " << filter_name << "!" << endl;
}

//**************************************************************************************************//
//                                          Handler                                                 //
//**************************************************************************************************//


/**
 * Perform the input process on the input filename and use the filter name in the output
 * @param filename The BMP file name to save the image to
 * @param filter_name The common name for the output of the process, such as 'clarendon'
 * @param *process A pointer to a function for processing the image and returning the new image
 * @void
 */
void execute(string filename, string filter_name, Process process) {
    string upper = filter_name;
    upper[0] = std::toupper(filter_name[0]);
    Image image = get_image(filename, upper);
    Image new_image = process(image);
    respond(filter_name, new_image);
}

//**************************************************************************************************//
//                                         "Router"                                                 //
//**************************************************************************************************//

const Process proc_arr[10] = {
        &process_1,
        &process_2,
        &process_3,
        &process_4,
        &process_5,
        &process_6,
        &process_7,
        &process_8,
        &process_9,
        &process_10
};
const string filter_arr[10] = {
        "vignette",
        "clarendon",
        "greyscale",
        "rotate 90 degrees",
        "rotate multiple 90 degrees",
        "enlarge",
        "high contrast",
        "lighten",
        "darken",
        "black, white, red, green, blue"
};

string map_selection(int s, string current_file) {
    if (s == 11) {
        cout << "Change image selected" << endl;
        return get_input_filename();
    }
    execute(current_file, filter_arr[s-1], proc_arr[s-1]);
    return current_file;
}

//**************************************************************************************************//
//                                       Main                                                       //
//**************************************************************************************************//

int main()
{
    cout << "CSPB 1300 Image Processing Application" << endl;
    string filename = get_input_filename();

    string input;

    while (input != "Q" && input != "q") {
        int sel = 0;
        while (sel == 0) {
            print_menu(filename);
            cin >> input;
            if (input == "Q" || input == "q") {
                break;
            }
            sel = atoi(input.c_str());
        }
        if (input == "Q" || input == "q") {
            break;
        }
        if (sel > 11) {
            continue;
        }
        filename = map_selection(sel, filename);
    }


    return 0;
}