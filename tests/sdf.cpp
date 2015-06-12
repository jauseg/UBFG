/* http://www.codersnotes.com/algorithms/signed-distance-fields */

#include <QImage>
#include <QTime>
#include <QDebug>
#include <math.h>

#define MULTIPLER 8

struct Point
{
    short dx, dy;
    int f;
};

struct Grid
{
    int w, h;
    Point *grid;
    Grid(int width, int height) : w(width), h(height)
    {
        grid = new Point[(w + 2) * (h + 2)];
    }
    ~Grid()
    {
        delete[] grid;
    }
};
static const Point pointInside = { 0, 0, 0 };
static const Point pointEmpty = { SHRT_MAX, SHRT_MAX, INT_MAX/2 };

static inline Point Get(Grid &g, int x, int y)
{
    return g.grid[y * (g.w + 2) + x];
}

static inline void Put(Grid &g, int x, int y, const Point &p)
{
    g.grid[y * (g.w + 2) + x] = p;
}

static inline void Compare(Grid &g, int x, int y, int offsetx, int offsety)
{
    int add;
    Point other = Get(g, x + offsetx, y + offsety);
    if(offsety == 0) {
        add = 2 * other.dx + 1;
    }
    else if(offsetx == 0) {
        add = 2 * other.dy + 1;
    }
    else {
        add = 2 * (other.dy + other.dx + 1);
    }
    other.f += add;
    if (other.f < Get(g, x, y).f)
    {
        if(offsety == 0) {
            other.dx++;
        }
        else if(offsetx == 0) {
            other.dy++;
        }
        else {
            other.dy++;
            other.dx++;
        }
        Put(g, x, y, other);
    }
}

static void Generate8SED(Grid &g)
{
    /* forward scan */
    for(int y = 1; y <= g.h; y++)
    {
        for(int x = 1; x <= g.w; x++)
        {
            Compare(g, x, y,  0, -1);
            Compare(g, x, y, -1,  0);
            Compare(g, x, y, -1, -1);
        }
        for(int x = g.w; x >= 0; x--)
        {
            Compare(g, x, y,  1,  0);
            Compare(g, x, y,  1, -1);
        }
    }

    /* backward scan */
    for(int y = g.h - 1; y > 0; y--)
    {
        for(int x = 1; x <= g.w; x++)
        {
            Compare(g, x, y,  0,  1);
            Compare(g, x, y, -1,  0);
            Compare(g, x, y, -1,  1);
        }
        for(int x = g.w - 1; x > 0; x--)
        {
            Compare(g, x, y,  1,  0);
            Compare(g, x, y,  1,  1);
        }
    }
}
static void Generate4SED(Grid &g)
{
    /* forward scan */
    for(int y = 1; y <= g.h; y++)
    {
        for(int x = 1; x <= g.w; x++)
        {
            Compare(g, x, y,  0, -1);
            Compare(g, x, y, -1,  0);
        }
        for(int x = g.w - 1; x > 0; x--)
        {
            Compare(g, x, y,  1,  0);
        }
    }

    /* backward scan */
    for(int y = g.h - 1; y > 0; y--)
    {
        for(int x = 1; x <= g.w; x++)
        {
            Compare(g, x, y,  0,  1);
            Compare(g, x, y, -1,  0);
        }
        for(int x = g.w - 1; x > 0; x--)
        {
            Compare(g, x, y,  1,  0);
        }
    }
}



QImage dfcalculate(QImage &img)
{
    int w = img.width(), h = img.height();
    QImage result(w, h, QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        result.setColor(i, qRgb(i,i,i));
    }
    Grid grid(w, h);
    /* create 1-pixel gap */
    for(int x = 0; x < w + 2; x++)
    {
        Put(grid, x, 0, pointEmpty);
        Put(grid, x, h + 1, pointEmpty);
    }
    uchar *data = img.bits();
    uchar pixel = img.bytesPerLine() / w;
    #pragma omp parallel for
    for(int y = 1; y <= h; y++)
    {
        Put(grid, 0, y, pointEmpty);
        for(int x = 1; x <= w; x++)
        {
            if(data[((y - 1) * w + (x - 1)) * pixel] > 128)
            {
                Put(grid, x, y, pointInside);
            }
            else
            {
                Put(grid, x, y, pointEmpty);
            }
        }
        Put(grid, w + 1, y, pointEmpty);
    }
    Generate4SED(grid);
    data  = result.bits();
    #pragma omp parallel for
    for(int y = 1; y <= h; y++)
    {
        for(int x = 1; x <= w; x++)
        {
            float dist2 = sqrtf((float)Get(grid, x, y).f);
            quint8 c = dist2 * MULTIPLER;
            data[(y - 1) * w + (x - 1)] = c;
        }
    }
    return result;
}

#include <QPair>
#include <QVector>

QImage dfcalculate_bruteforce(QImage &img)
{
    int w = img.width(), h = img.height();
    QImage result(w, h, QImage::Format_Indexed8);
    for(int i = 0; i < 256; i++)
    {
        result.setColor(i, qRgb(i,i,i));
    }
    QVector< QPair<short, short> > cache;
    for(short y = 0; y < h; y++)
    {
        for(short x = 0; x < w; x++)
        {
            if(qGreen(img.pixel(x, y)) > 128)
                cache.append({x,y});
        }
    }
    uchar *data  = result.bits();
    #pragma omp parallel for
    for(short y = 0; y < h; y++)
    {
        for(short x = 0; x < w; x++)
        {
            int min = INT_MAX;
            for(int l = 0; l < cache.size(); l++)
            {
                int i = cache.at(l).first - x;
                int j = cache.at(l).second - y;
                int d = i * i + j * j;
                if(d < min)
                {
                    min = d;
                }
            }
            quint8 c = sqrtf(min) * MULTIPLER;
            data[y * w + x] = c;
        }
    }
    return result;
}
