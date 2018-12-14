// ==============================
// By Jimmy
//
// 2018/12/14
//
// 1. sort out code
// ==============================

#include "image.h"
#include "mjpeg_streaming.h"
#include "socket_server.h"

#include <iostream>
#include <vector>
#include <map>

#include <fstream> // read tag file

//static void * cap;
static cv::Mat m;
static image im;
static image **alphabet;
static std::vector<unsigned char> frame;
static std::vector<person_box> boxes;

static std::map<std::string, image> tag_pics;

static std::map<int, std::vector<unsigned char>> frame_buffer;
static std::vector<int> frame_buffer_remove;

void load_tag_pic();
void draw_tag_pic(image im, std::string tag_name, int &all_tag_count, int &legal_tag_count);

int main()
{
    alphabet = load_alphabet();
    int red = 0, green = 255, blue = 0;
    float rgb[3] = {red, green, blue};

    int frame_stamp = 0;
    frame_buffer.clear();

    while(1)
    {
        frame_stamp = receive_frame_with_stamp(frame, 8091, 200, 95);

        if(frame_stamp && !frame.empty())
        {
            m = cv::imdecode(frame, 1);
            im = mat_to_image(m);

            char frame_stamp_label_str[4096] = {0};
            sprintf(frame_stamp_label_str, "%d - %d", frame_stamp, frame_buffer.size());
            image frame_stamp_label = get_label(alphabet, frame_stamp_label_str, (im.h*.03));
            draw_label(im, 0, 0, frame_stamp_label, rgb);

            m = image_to_mat(im);
            send_mjpeg(m, 8090, 200, 95);
        }
    }

    return 0;
}



void load_tag_pic()
{
    tag_pics.clear();

    std::ifstream in("tag_pic/tag_pic.txt");

    if(!in)
    {
        std::cout<<"Cannot open input file."<<std::endl;
        exit(EXIT_FAILURE);
    }

    std::string str, tag_str;

    while (std::getline(in, str))
    {
        // output the line
        tag_str = "tag_pic/" + str;
        str = str.substr(0, str.size() - 4); // remove .jpg
        std::cout<<str<<std::endl;
        std::cout<<tag_str<<std::endl;
        tag_pics.insert(std::pair<std::string, image>(str, resize_image(load_image_color((char *)tag_str.c_str(), 100, 100), 100, 100)));

        /*cv::Mat m = image_to_mat(tag_pics);
        imshow(str, m);
        waitKey(1);*/
    }

    in.close();
}

void draw_tag_pic(image im, std::string tag_name, int &all_tag_count, int &legal_tag_count)
{
    int tag_pic_width = 220;
    int is_legal_tag = 0;

    int dx = im.w - tag_pic_width + 60;
    int dy;

    if (tag_name != "unknown")
    {
        legal_tag_count++;
        dy = 40 + (int)100 * (legal_tag_count - 1); // legal tag position
        is_legal_tag = 1;
    }
    else
    {
        dy = 40 + (int)160 * (all_tag_count - legal_tag_count); // unknown tag position
    }

    all_tag_count++;

    dy += (is_legal_tag) ? 30 : (int)im.h / 2;

    embed_image(tag_pics[tag_name], im, dx, dy);

    // draw label on tag
    float rgb[3];
    if(is_legal_tag)
    {
        image label = get_label(alphabet, (char *)tag_name.c_str(), (im.h*.03)/10);
        
        rgb[0] = 0.0;
        rgb[1] = 1.0;
        rgb[2] = 0.0;

        //draw_label(im, dy, dx - 40, label, rgb);
        draw_label(im, dy, dx, label, rgb);
        free_image(label);
    }
    else
    {
        image label = get_label(alphabet, (char *)tag_name.c_str(), (im.h*.03)/16);

        rgb[0] = 1.0;
        rgb[1] = 0.0;
        rgb[2] = 0.0;

        draw_label(im, dy, dx, label, rgb);
        free_image(label);
    }

}