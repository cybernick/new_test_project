#include <iostream>
#include <vector>
#include <algorithm>
#include <iterator>
#include <string>
#include <curses.h>
#include <stdint.h>
#include <fstream>

using namespace std;

int PolyLine = 3, MultiPoint = 8;

class ByteConverter
{
    public:

    static int32_t big_endian_int_read(char *file_buf, int start_index)
    {
        return ((unsigned char)(file_buf[start_index + 0]  << 24) | ((unsigned char)file_buf[start_index + 1] << 16)| ((unsigned char)file_buf[start_index + 2] << 8) | ((unsigned char)file_buf[start_index + 3]));
    }
    static int32_t little_endian_int_read(char *file_buf, int start_index)
    {
        return (((unsigned char)file_buf[start_index + 3] << 24) | ((unsigned char)file_buf[start_index + 2] << 16) | ((unsigned char)file_buf[start_index + 1]  << 8) | (((unsigned char)file_buf[start_index + 0] )));
    }
    static double little_endian_double_read(char *file_buf, int start_index)
    {
        double result;
        char *arr;
        int j = -1;
        arr = new char();
        for (int i=start_index; i < start_index+8; i++)
        {
            j++;
            arr[j] = file_buf[i];
        }
        result = *reinterpret_cast<double *>(arr);
        return result;
    }
};

class HeaderShapefile
{
    public:

    static int32_t fileCode(char* file_buf, int start_index)
    {
        return ByteConverter::big_endian_int_read(file_buf, start_index);
    }

    static int32_t fileLength(char* file_buf, int start_index)
    {
       return ByteConverter::big_endian_int_read(file_buf, start_index);
    }

    static int32_t version(char* file_buf, int start_index)
    {
        return ByteConverter::little_endian_int_read(file_buf, start_index);
    }

    static int32_t shapeType(char* file_buf, int start_index)
    {
        return ByteConverter::little_endian_int_read(file_buf, start_index);
    }
};

class SizeOfFile
{
    public:

    static long size_of_files(FILE *file)
    {
            long l, e;
            l = ftell(file);
            fseek(file, 0, SEEK_END);
            e = ftell(file);
            fseek(file, l, SEEK_SET);
            return e;
    }
};

char chr_menu;

struct point
{
    double x;
    double y;
};

vector< vector<point> > lines;

class ReadFromFile
{
static char * copy_file_in_buffer(const char *file_path, long & size_of_file)
{
    char *file_buf;
    FILE *file = NULL;
    if ((file = fopen(file_path, "rb")) == NULL)
        cout << "Не могу открыть файл" << endl;
    else
        cout << "Файл успешно открыт!" << endl;
    size_of_file = SizeOfFile::size_of_files(file);

    file_buf = new char[size_of_file];

    fread(file_buf, size_of_file, 1, file);
    fclose(file);
    return file_buf;
}


static int get_record_from_file(char * file_buf, int start_index, int type)
{
    int rec_numb, cont_lenght,shp_type, num_points, num_parts;
    rec_numb = ByteConverter::big_endian_int_read(file_buf, start_index);
    cont_lenght = ByteConverter::big_endian_int_read(file_buf, start_index + 4);
    shp_type = ByteConverter::little_endian_int_read(file_buf, start_index + 8);
    if (type == PolyLine)
    {
        num_points = ByteConverter::little_endian_int_read(file_buf ,start_index + 48);
        num_parts = ByteConverter::little_endian_int_read(file_buf, start_index + 44);
    }
    else
    {
        num_points = ByteConverter::little_endian_int_read(file_buf,start_index+44);
    }
    vector<point> line;
    for (int i = 0; i < num_points; i++)
    {
        point p;
        if (type == PolyLine)
        {
            p.x = ByteConverter::little_endian_double_read(file_buf,start_index+52 + 4*num_parts + i*16);
            p.y = ByteConverter::little_endian_double_read(file_buf,start_index+52 + 4*num_parts + i*16 + 8);
        }
        else
        {
            p.x = ByteConverter::little_endian_double_read(file_buf,start_index+48 + i*16);
            p.y = ByteConverter::little_endian_double_read(file_buf,start_index+48 + i*16 + 8);
        }
        line.push_back(p);
    }
    lines.push_back(line);
    return start_index + cont_lenght*2 + 8;
}

public:
static void readInLines(string path)
{
    long size_of_file;
    int start_index = 100;
    char *file_buf = copy_file_in_buffer(path.c_str(),size_of_file);
    lines.clear();
    int shapeType = HeaderShapefile::shapeType(file_buf,32);
    while(size_of_file - start_index > 0)
    {
        start_index = get_record_from_file(file_buf, start_index, shapeType);
    }
}
};

class SaveInPolyLineAndMultiPoint
{
static int content_length(int type,int vert)
{
    if (type == MultiPoint)
    {
        return (40 + 2*vert*8);
    }
    else if (type == PolyLine)
    {
        return (48 + 2*vert*8);
    }
}

static int lines_size_to_shp_file_length(int type)
{
    int count=100;
    for(int i = 0; i < lines.size(); i++)
    {
        count+= 8 + content_length(type, lines.at(i).size());
    }
    return count;
}

static double max_x_in_lines(vector< vector<point> > lines)
{
    double max;
    vector<double> x_vect;
    for(int i = 0; i < lines.size(); i++)
    {
        for(int j = 0;j < lines.at(i).size(); j++)
        {
            x_vect.push_back(lines.at(i).at(j).x);
        }
    }
    max = *max_element(x_vect.begin(), x_vect.end());
    return max;
}
static double max_y_in_lines(vector< vector<point> > lines)
{
    double max;
    vector<double> y_vect;
    for(int i = 0;i < lines.size(); i++)
    {
        for(int j = 0; j < lines.at(i).size(); j++)
        {
            y_vect.push_back(lines.at(i).at(j).y);
        }
    }
    max = *max_element(y_vect.begin(), y_vect.end());
    return max;
}
static double min_x_in_lines(vector< vector<point> > lines)
{
    double min;
    vector<double> x_vect;
    for(int i = 0; i < lines.size(); i++)
    {
        for(int j=0; j < lines.at(i).size(); j++)
        {
            x_vect.push_back(lines.at(i).at(j).x);
        }
    }
    min = *min_element(x_vect.begin(),x_vect.end());
    return min;
}
static double min_y_in_lines(vector< vector<point> > lines)
{
    double min;
    vector<double> y_vect;
    for(int i = 0; i < lines.size(); i++)
    {
        for(int j=0;j<lines.at(i).size();j++)
        {
            y_vect.push_back(lines.at(i).at(j).y);
        }
    }
    min = *min_element(y_vect.begin(),y_vect.end());
    return min;
}

static double max_x_in_line( vector<point>  line)
{
    double max;
    vector<double> x_vect;
    for(int i = 0; i < line.size(); i++)
    {
            x_vect.push_back(line.at(i).x);
    }
    max = *max_element(x_vect.begin(), x_vect.end());
    return max;
}
static double max_y_in_line( vector<point>  line)
{
    double max;
    vector<double> y_vect;
    for(int i = 0; i < line.size(); i++)
    {
            y_vect.push_back(line.at(i).y);
    }
    max = *max_element(y_vect.begin(), y_vect.end());
    return max;
}
static double min_x_in_line( vector<point>  line)
{
    double min;
    vector<double> x_vect;
    for(int i = 0; i < line.size(); i++)
    {
            x_vect.push_back(line.at(i).x);
    }
    min = *min_element(x_vect.begin(), x_vect.end());
    return min;
}
static double min_y_in_line( vector<point>  line)
{
    double min;
    vector<double> y_vect;
    for(int i = 0; i < line.size(); i++)
    {
            y_vect.push_back(line.at(i).y);
    }
    min = *min_element(y_vect.begin(), y_vect.end());
    return min;
}



static void big_endian_int_write(int32_t number, ofstream &file)
{
    char * data = new char[4];
    data[0] = static_cast<char>((number >> 24) & 0xFF);
    data[1] = static_cast<char>((number >> 16) & 0xFF);
    data[2] = static_cast<char>((number >> 8) & 0xFF);
    data[3] = static_cast<char>(number & 0xFF);
    file.write(data,4);
    delete[] data;
}

static void little_endian_int_write(int32_t number, ofstream &file)
{

    char * data = new char[4];
    data[0] = static_cast<char>(number & 0xFF);
    data[1] = static_cast<char>((number >> 8) & 0xFF);
    data[2] = static_cast<char>((number >> 16) & 0xFF);
    data[3] = static_cast<char>((number >> 24) & 0xFF);
    file.write(data,4);
    delete[] data;
}

static void little_endian_double_write(double number, ofstream &file)
{
    char * data = new char[8];
    char *pDouble = (char*)(double*)(&number);
        for (int i = 0; i < 8; ++i) {
          data[i] = pDouble[i];
        }
    file.write(data,8);
    delete[] data;
}
public:
static void save_lines_in_polyline_file(string path,int type)
{
    ofstream file(path.c_str(),ios::binary);
    //HEADER
    big_endian_int_write(9994, file);

    for (int i = 0; i < 5; i++)
    {
        big_endian_int_write(0, file);
    }
    int len = lines_size_to_shp_file_length(type);
    big_endian_int_write(len, file);
    little_endian_int_write(1000, file);
    little_endian_int_write(type, file);
    little_endian_double_write(min_x_in_lines(lines), file);
    little_endian_double_write(min_y_in_lines(lines), file);
    little_endian_double_write(max_x_in_lines(lines), file);
    little_endian_double_write(max_y_in_lines(lines), file);
    for(int i = 0; i < 4; i++)
    {
        little_endian_double_write(0, file);
    }
    // DATA
    for(int i = 0;i < lines.size(); i++)
    {
        // RECORD HEADER
        big_endian_int_write(i+1, file);
        big_endian_int_write(content_length(type, lines.at(i).size())/2, file);

        // RECORD CONTENT
        little_endian_int_write(type, file);
        //Bound BOX
        little_endian_double_write(min_x_in_line(lines.at(i)), file);
        little_endian_double_write(min_y_in_line(lines.at(i)), file);
        little_endian_double_write(max_x_in_line(lines.at(i)), file);
        little_endian_double_write(max_y_in_line(lines.at(i)), file);

        if(type == PolyLine)
        {
            little_endian_int_write(1, file);
        }
        little_endian_int_write(lines.at(i).size(), file);
        if(type == PolyLine)
        {
            little_endian_int_write(0, file);
        }
        for(int j = 0; j < lines.at(i).size(); j++)
        {
            little_endian_double_write(lines.at(i).at(j).x, file);
            little_endian_double_write(lines.at(i).at(j).y, file);
        }
    }

}
};

void menu()
{
    cout<<"Для сохранения в файл нажмите S, для считывания из файла нажмите R, для вывода на консоль вектора W"<<endl;
    cout<<"для выхода нажмите E."<<endl;
    cin>>chr_menu;
    return;
}

int main()
{
    cout<<"Добро пожаловать в приложение по работе с shapefile!"<<endl;
    cout<<"Производиться инициализация вектора линий..."<<endl;

    for (int i = 0; i < 10; i++)
    {
        vector<point> line;
        for (int j = 0; j < 2+i; j++)
        {
            point p;
            p.x = j+1000;
            p.y = j+1000;
            line.push_back(p);
        }
        lines.push_back(line);
        line.clear();
    }

    cout<<"Вектор инициализирован..."<<endl;
    cout<<"                         "<<endl;
    cout<<"Для сохранения в файл нажмите S, для считывания из файла нажмите R,";
    cout<<"для вывода на консоль вектора W, для выхода нажмите E."<<endl;
    cin>>chr_menu;
    while (chr_menu != 'E' && chr_menu != 'e')
    {
        if (chr_menu == 'S' || chr_menu == 's')
        {
            string  polyline_path;
            string  multi_point_path;

            cout<<"Введите путь и имя создаваемого файла для полилайнов";
            cout<<"(например: /home/username/polyline.shp)"<<endl;
            getline (cin, polyline_path);
            getline (cin, polyline_path);

            cout<<"Введите путь и имя создаваемого файла для мультиточек";
            cout<<"(например: /home/username/polyline.shp)"<<endl;
            getline (cin, multi_point_path);


            SaveInPolyLineAndMultiPoint::save_lines_in_polyline_file(polyline_path, PolyLine);
            SaveInPolyLineAndMultiPoint::save_lines_in_polyline_file(multi_point_path, MultiPoint);

        }
        else if (chr_menu == 'R' || chr_menu == 'r')
        {
            string  path;
            cout<<"Введите путь и имя файла для чтения файлов полилайнов или мультиточек";
            cout<<"(например: /home/username/polyline.shp)"<<endl;
            getline (cin, path);
            getline (cin, path);
            ReadFromFile::readInLines(path);
        }
        else if (chr_menu == 'W' || chr_menu == 'w')
        {
            for (int i = 0; i < lines.size(); i++)
            {
                cout<<"Линия номер-"<<i<<endl;
                for (int j = 0; j < lines.at(i).size(); j++)
                {
                    cout<<"x-"<<j<<":"<<lines.at(i).at(j).x<<" "<<"y-"<<j<<":"<<lines.at(i).at(j).y<<endl;
                }
            }
        }
        menu();
    }

    return 0;
}

