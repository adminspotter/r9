#include <gtest/gtest.h>

#include "../client/ui/image.h"

TEST(ImageTest, CreateDelete)
{
    ui::image *img = new ui::image();
    ASSERT_EQ(img->width, 0);
    ASSERT_EQ(img->height, 0);
    ASSERT_EQ(img->per_pixel, 0);
    ASSERT_TRUE(img->data == NULL);
    delete img;
}

TEST(ImageTest, CopyConstructor)
{
    ui::image *img = new ui::image();
    img->width = 10;
    img->height = 10;
    img->per_pixel = 1;
    img->data = new unsigned char[img->width * img->height * img->per_pixel];

    ui::image *new_img = new ui::image(*img);
    ASSERT_EQ(new_img->width, 10);
    ASSERT_EQ(new_img->height, 10);
    ASSERT_EQ(new_img->per_pixel, 1);
    ASSERT_TRUE(img->data != NULL);
    delete new_img;
    delete img;
}

TEST(ImageTest, Assignment)
{
    ui::image *img = new ui::image();
    img->width = 10;
    img->height = 10;
    img->per_pixel = 1;
    img->data = new unsigned char[img->width * img->height * img->per_pixel];

    ui::image *new_img = new ui::image();
    new_img->width = 5;
    new_img->height = 5;
    new_img->per_pixel = 1;
    new_img->data = new unsigned char[new_img->width
                                      * new_img->height
                                      * new_img->per_pixel];

    *new_img = *img;

    ASSERT_EQ(new_img->width, 10);
    ASSERT_EQ(new_img->height, 10);
    ASSERT_EQ(new_img->per_pixel, 1);
    ASSERT_TRUE(img->data != NULL);
    delete new_img;
    delete img;
}

TEST(ImageTest, Reset)
{
    ui::image *img = new ui::image();
    img->width = 10;
    img->height = 10;
    img->per_pixel = 1;
    img->data = new unsigned char[img->width * img->height * img->per_pixel];
    img->reset();
    ASSERT_EQ(img->width, 0);
    ASSERT_EQ(img->height, 0);
    ASSERT_EQ(img->per_pixel, 0);
    ASSERT_TRUE(img->data == NULL);
    delete img;
}
